#!/usr/bin/env python3
"""
Native Bluetooth sEMG Streaming Test
Connects directly via Bluetooth SPP, captures 10 seconds of data, and generates analysis report.

Requirements:
    Windows: pip install pybluez-win10 matplotlib numpy scipy
    Linux:   pip install pybluez matplotlib numpy scipy

Usage:
    python test_bluetooth_native.py
    python test_bluetooth_native.py --device "NeuroEstimulator"
    python test_bluetooth_native.py --address "00:11:22:33:44:55"
"""

import struct
import time
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
from scipy import signal
import argparse
import sys
import platform

# Try to import Bluetooth library
try:
    import bluetooth
    BLUETOOTH_AVAILABLE = True
except ImportError:
    BLUETOOTH_AVAILABLE = False
    print("\n⚠️  WARNING: PyBluez not installed!")
    print("Install with:")
    if platform.system() == "Windows":
        print("  pip install pybluez-win10")
    else:
        print("  pip install pybluez")
    print("\nFalling back to manual connection mode...\n")

# ============================================================================
# PROTOCOL CONSTANTS
# ============================================================================
PACKET_MAGIC_BYTE = 0xAA
PACKET_MESSAGE_CODE_STREAM_DATA = 13
PACKET_HEADER_SIZE = 8
SAMPLE_SIZE = 2
MAX_SAMPLES_PER_PACKET = 50

# Streaming configuration
SAMPLING_RATE = 215  # Hz
CAPTURE_DURATION = 10  # seconds
EXPECTED_SAMPLES = SAMPLING_RATE * CAPTURE_DURATION

# Bluetooth SPP UUID (Serial Port Profile)
SPP_UUID = "00001101-0000-1000-8000-00805F9B34FB"

# ============================================================================
# BLUETOOTH CONNECTION
# ============================================================================

def scan_for_device(device_name="NeuroEstimulator", timeout=10):
    """Scan for Bluetooth device by name"""
    if not BLUETOOTH_AVAILABLE:
        return None, None

    print(f"🔍 Scanning for Bluetooth devices (timeout: {timeout}s)...")
    print(f"   Looking for: '{device_name}'")

    try:
        nearby_devices = bluetooth.discover_devices(duration=timeout, lookup_names=True)

        print(f"\n📡 Found {len(nearby_devices)} device(s):")
        for addr, name in nearby_devices:
            status = "✅" if device_name.lower() in name.lower() else "  "
            print(f"   {status} {name} [{addr}]")

        # Find target device
        for addr, name in nearby_devices:
            if device_name.lower() in name.lower():
                print(f"\n✅ Target device found!")
                print(f"   Name: {name}")
                print(f"   Address: {addr}")
                return addr, name

        print(f"\n❌ Device '{device_name}' not found")
        return None, None

    except Exception as e:
        print(f"❌ Bluetooth scan failed: {e}")
        return None, None

def connect_bluetooth(address):
    """Connect to device via Bluetooth SPP"""
    if not BLUETOOTH_AVAILABLE:
        return None

    print(f"\n🔗 Connecting to {address}...")

    try:
        # Create Bluetooth socket (RFCOMM)
        sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)

        # Connect to SPP service (channel 1)
        sock.connect((address, 1))

        print(f"✅ Connected successfully!")
        return sock

    except bluetooth.BluetoothError as e:
        print(f"❌ Connection failed: {e}")
        print("\n💡 Troubleshooting:")
        print("   1. Make sure device is powered on")
        print("   2. Pair device in system Bluetooth settings first")
        print("   3. Check if device is not connected to other apps")
        return None
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        return None

# ============================================================================
# PACKET PARSER
# ============================================================================

class BinaryPacketParser:
    """Parse binary streaming packets from ESP32"""

    def __init__(self):
        self.buffer = bytearray()
        self.packets_received = 0
        self.samples_received = 0
        self.errors = 0

    def parse_header(self, data):
        """Parse 8-byte packet header"""
        if len(data) < PACKET_HEADER_SIZE:
            return None

        magic = data[0]
        code = data[1]
        timestamp = struct.unpack('<I', data[2:6])[0]
        sample_count = struct.unpack('<H', data[6:8])[0]

        return {
            'magic': magic,
            'code': code,
            'timestamp': timestamp,
            'sample_count': sample_count
        }

    def parse_packet(self, data):
        """Parse complete binary packet"""
        header = self.parse_header(data)

        if header is None:
            return None

        # Validate header
        if header['magic'] != PACKET_MAGIC_BYTE:
            self.errors += 1
            return None

        if header['code'] != PACKET_MESSAGE_CODE_STREAM_DATA:
            return None

        if header['sample_count'] > MAX_SAMPLES_PER_PACKET:
            self.errors += 1
            return None

        # Parse samples
        payload_size = header['sample_count'] * SAMPLE_SIZE
        expected_packet_size = PACKET_HEADER_SIZE + payload_size

        if len(data) < expected_packet_size:
            return None

        samples = []
        for i in range(header['sample_count']):
            offset = PACKET_HEADER_SIZE + (i * SAMPLE_SIZE)
            sample = struct.unpack('<h', data[offset:offset+2])[0]
            samples.append(sample)

        self.packets_received += 1
        self.samples_received += len(samples)

        return {
            'timestamp': header['timestamp'],
            'samples': samples
        }

    def find_sync(self):
        """Find magic byte in buffer"""
        try:
            idx = self.buffer.index(PACKET_MAGIC_BYTE)
            if idx > 0:
                self.buffer = self.buffer[idx:]
            return True
        except ValueError:
            self.buffer.clear()
            return False

    def process_buffer(self):
        """Process buffered data and extract packets"""
        packets = []

        while len(self.buffer) >= PACKET_HEADER_SIZE:
            if self.buffer[0] != PACKET_MAGIC_BYTE:
                if not self.find_sync():
                    break

            header = self.parse_header(self.buffer)
            if header is None:
                self.buffer = self.buffer[1:]
                continue

            packet_size = PACKET_HEADER_SIZE + (header['sample_count'] * SAMPLE_SIZE)

            if len(self.buffer) < packet_size:
                break

            packet_data = bytes(self.buffer[:packet_size])
            packet = self.parse_packet(packet_data)

            if packet is not None:
                packets.append(packet)

            self.buffer = self.buffer[packet_size:]

        return packets

# ============================================================================
# DATA CAPTURE
# ============================================================================

def send_command(sock, command_json):
    """Send JSON command via Bluetooth"""
    try:
        cmd_bytes = (command_json + '\0').encode('ascii')
        sock.send(cmd_bytes)
        return True
    except Exception as e:
        print(f"❌ Failed to send command: {e}")
        return False

def capture_stream_bluetooth(sock, duration=CAPTURE_DURATION):
    """Capture streaming data via Bluetooth"""

    print(f"\n{'='*60}")
    print(f"  sEMG BLUETOOTH STREAMING TEST")
    print(f"  Duration: {duration}s @ {SAMPLING_RATE} Hz")
    print(f"{'='*60}\n")

    # Send start command
    print("📤 Sending START_STREAM command: {\"cd\":11,\"mt\":\"x\"}")
    if not send_command(sock, '{"cd":11,"mt":"x"}'):
        return None, None

    time.sleep(0.5)  # Wait for ESP32 to start

    # Initialize parser
    parser = BinaryPacketParser()
    samples = []
    timestamps = []

    start_time = time.time()
    last_progress = 0

    print(f"📊 Capturing data...")
    print(f"   Expected samples: {EXPECTED_SAMPLES}")
    print()

    try:
        while time.time() - start_time < duration:
            # Read data (non-blocking with timeout)
            try:
                sock.settimeout(0.1)
                data = sock.recv(1024)
                if data:
                    parser.buffer.extend(data)

                    # Process packets
                    packets = parser.process_buffer()

                    for packet in packets:
                        samples.extend(packet['samples'])
                        for i in range(len(packet['samples'])):
                            t = len(timestamps) / SAMPLING_RATE
                            timestamps.append(t)

            except bluetooth.BluetoothError:
                pass  # Timeout, continue
            except Exception as e:
                print(f"⚠️  Read error: {e}")

            # Progress
            progress = int((time.time() - start_time) / duration * 100)
            if progress >= last_progress + 20:
                print(f"   {progress}% - Samples: {len(samples):4d}, Packets: {parser.packets_received}")
                last_progress = progress

            time.sleep(0.001)

    except KeyboardInterrupt:
        print("\n⚠️  Capture interrupted by user")

    # Send stop command
    print(f"\n📤 Sending STOP_STREAM command: {{\"cd\":12,\"mt\":\"x\"}}")
    send_command(sock, '{"cd":12,"mt":"x"}')
    time.sleep(0.2)

    # Statistics
    elapsed = time.time() - start_time
    actual_rate = len(samples) / elapsed if elapsed > 0 else 0

    print(f"\n{'='*60}")
    print(f"  CAPTURE STATISTICS")
    print(f"{'='*60}")
    print(f"  Duration:        {elapsed:.2f} seconds")
    print(f"  Samples:         {len(samples)} / {EXPECTED_SAMPLES} expected")
    print(f"  Packets:         {parser.packets_received}")
    print(f"  Actual rate:     {actual_rate:.1f} Hz")
    print(f"  Packet errors:   {parser.errors}")
    print(f"  Completeness:    {len(samples)/EXPECTED_SAMPLES*100:.1f}%")
    print(f"{'='*60}\n")

    return np.array(samples), np.array(timestamps)

# ============================================================================
# ANALYSIS & VISUALIZATION
# ============================================================================

def analyze_and_plot(samples, timestamps):
    """Generate comprehensive analysis report"""

    if len(samples) == 0:
        print("❌ No samples to analyze!")
        return

    # Convert to voltage (ADS1115: LSB = 0.1875 mV)
    ADC_LSB = 0.0001875  # V
    voltages = samples * ADC_LSB

    # Statistics
    print(f"\n{'='*60}")
    print(f"  SIGNAL ANALYSIS")
    print(f"{'='*60}")
    print(f"  Raw ADC range:   {samples.min()} to {samples.max()}")
    print(f"  Voltage range:   {voltages.min():.4f} to {voltages.max():.4f} V")
    print(f"  Mean voltage:    {voltages.mean():.4f} V")
    print(f"  Std deviation:   {voltages.std():.4f} V")
    print(f"  RMS voltage:     {np.sqrt(np.mean(voltages**2)):.4f} V")
    print(f"{'='*60}\n")

    # FFT
    N = len(samples)
    fft_values = np.fft.fft(samples)
    fft_freq = np.fft.fftfreq(N, 1/SAMPLING_RATE)

    positive_freq_idx = fft_freq > 0
    fft_magnitude = np.abs(fft_values[positive_freq_idx])
    fft_freq_positive = fft_freq[positive_freq_idx]

    peak_idx = np.argmax(fft_magnitude)
    dominant_freq = fft_freq_positive[peak_idx]

    print(f"  Dominant frequency: {dominant_freq:.2f} Hz")
    print(f"  FFT peak magnitude: {fft_magnitude[peak_idx]:.2f}\n")

    # Generate plots
    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    fig.suptitle(f'Bluetooth sEMG Analysis - {SAMPLING_RATE} Hz @ {CAPTURE_DURATION}s',
                 fontsize=16, fontweight='bold')

    # Plot 1: Raw ADC
    axes[0].plot(timestamps, samples, 'b-', linewidth=0.5, alpha=0.8)
    axes[0].set_xlabel('Time (s)')
    axes[0].set_ylabel('ADC Value (int16)')
    axes[0].set_title('Raw ADC Signal (Filtered: 10-50 Hz + Notch 60 Hz)')
    axes[0].grid(True, alpha=0.3)

    stats_text = f"Range: [{samples.min()}, {samples.max()}]\nMean: {samples.mean():.1f}\nStd: {samples.std():.1f}"
    axes[0].text(0.02, 0.98, stats_text, transform=axes[0].transAxes,
                 fontsize=10, verticalalignment='top',
                 bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    # Plot 2: Voltage
    axes[1].plot(timestamps, voltages * 1000, 'g-', linewidth=0.5, alpha=0.8)
    axes[1].set_xlabel('Time (s)')
    axes[1].set_ylabel('Voltage (mV)')
    axes[1].set_title('Voltage Signal (mV)')
    axes[1].grid(True, alpha=0.3)

    rms = np.sqrt(np.mean(voltages**2)) * 1000
    axes[1].axhline(y=rms, color='r', linestyle='--', linewidth=2, label=f'RMS: {rms:.2f} mV')
    axes[1].legend()

    # Plot 3: FFT
    axes[2].plot(fft_freq_positive, fft_magnitude, 'r-', linewidth=1.5, alpha=0.8)
    axes[2].set_xlabel('Frequency (Hz)')
    axes[2].set_ylabel('Magnitude')
    axes[2].set_title('Frequency Spectrum (FFT)')
    axes[2].grid(True, alpha=0.3)
    axes[2].set_xlim([0, min(100, SAMPLING_RATE/2)])

    axes[2].axvspan(10, 50, alpha=0.2, color='green', label='Passband (10-50 Hz)')
    axes[2].axvline(x=60, color='orange', linestyle='--', linewidth=2, label='Notch @ 60 Hz')
    axes[2].plot(dominant_freq, fft_magnitude[peak_idx], 'bo', markersize=10)
    axes[2].legend()

    plt.tight_layout(rect=[0, 0, 1, 0.97])

    # Save
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    plot_file = f"bluetooth_analysis_{timestamp}.png"
    csv_file = f"bluetooth_data_{timestamp}.csv"

    plt.savefig(plot_file, dpi=300, bbox_inches='tight')
    print(f"📊 Plot saved: {plot_file}")

    # Save CSV
    data = np.column_stack((timestamps, samples, voltages))
    header = "Time(s),ADC_Value,Voltage(V)"
    np.savetxt(csv_file, data, delimiter=',', header=header, comments='')
    print(f"💾 Data saved: {csv_file}")

    plt.show()

# ============================================================================
# MAIN
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description='Bluetooth sEMG Streaming Test')
    parser.add_argument('--device', default='NeuroEstimulator', help='Device name to search for')
    parser.add_argument('--address', help='Device MAC address (skip scan)')
    parser.add_argument('--duration', type=int, default=10, help='Capture duration in seconds')
    args = parser.parse_args()

    print(f"\n{'='*60}")
    print(f"  BLUETOOTH sEMG STREAMING TEST")
    print(f"{'='*60}\n")

    if not BLUETOOTH_AVAILABLE:
        print("❌ PyBluez not available. Cannot proceed.")
        return 1

    # Get device address
    if args.address:
        address = args.address
        print(f"✅ Using provided address: {address}")
    else:
        address, name = scan_for_device(args.device)
        if not address:
            print("\n❌ Device not found. Try specifying address with --address")
            return 1

    # Connect
    sock = connect_bluetooth(address)
    if not sock:
        return 1

    try:
        # Capture data
        samples, timestamps = capture_stream_bluetooth(sock, duration=args.duration)

        if samples is None or len(samples) == 0:
            print("❌ No data captured")
            return 1

        # Analyze and plot
        analyze_and_plot(samples, timestamps)

        print("\n✅ Test completed successfully!\n")
        return 0

    finally:
        # Close connection
        print("🔌 Closing Bluetooth connection...")
        sock.close()
        print("✅ Connection closed\n")

if __name__ == "__main__":
    exit(main())
