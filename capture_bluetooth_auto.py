#!/usr/bin/env python3
"""
Automatic Bluetooth sEMG Capture
Automatically scans, connects, captures, and disconnects.

Features:
- Auto-discovery of "NeuroEstimulator" device
- Native Bluetooth SPP connection
- 10-second data capture @ 215 Hz
- Automatic connection cleanup
- CSV export and visualization

Requirements:
    Windows: pip install pybluez-win10 matplotlib numpy scipy
    Linux:   pip install pybluez matplotlib numpy scipy

Usage:
    python capture_bluetooth_auto.py
    python capture_bluetooth_auto.py --duration 20
    python capture_bluetooth_auto.py --device "MyDevice"
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

# Bluetooth library
try:
    import bluetooth
    BT_AVAILABLE = True
except ImportError:
    BT_AVAILABLE = False
    print("\n" + "="*60)
    print("  ⚠️  PyBluez Not Installed")
    print("="*60)
    print("\nInstall with:")
    if platform.system() == "Windows":
        print("  pip install pybluez-win10")
    else:
        print("  pip install pybluez")
    print("\n" + "="*60 + "\n")
    sys.exit(1)

# ============================================================================
# CONFIGURATION
# ============================================================================
DEVICE_NAME = "NeuroEstimulator"
SAMPLING_RATE = 215  # Hz
CAPTURE_DURATION = 10  # seconds
SPP_CHANNEL = 1  # Bluetooth RFCOMM channel

# Protocol constants
MAGIC_BYTE = 0xAA
STREAM_CODE = 0x0D
HEADER_SIZE = 8
SAMPLE_SIZE = 2
MAX_SAMPLES = 50

# ============================================================================
# BLUETOOTH OPERATIONS
# ============================================================================

class BluetoothConnection:
    """Manages Bluetooth connection lifecycle"""

    def __init__(self, device_name=DEVICE_NAME):
        self.device_name = device_name
        self.address = None
        self.sock = None

    def scan(self, timeout=10):
        """Scan for device"""
        print(f"🔍 Scanning for '{self.device_name}' (timeout: {timeout}s)...")

        try:
            devices = bluetooth.discover_devices(duration=timeout, lookup_names=True)

            if not devices:
                print("   No devices found")
                return False

            print(f"\n📡 Found {len(devices)} device(s):")
            for addr, name in devices:
                match = "✅" if self.device_name.lower() in name.lower() else "   "
                print(f"   {match} {name} [{addr}]")

            # Find target
            for addr, name in devices:
                if self.device_name.lower() in name.lower():
                    self.address = addr
                    print(f"\n✅ Target found: {name} [{addr}]")
                    return True

            print(f"\n❌ '{self.device_name}' not found")
            return False

        except Exception as e:
            print(f"❌ Scan error: {e}")
            return False

    def connect(self):
        """Connect to device"""
        if not self.address:
            print("❌ No address to connect to")
            return False

        print(f"\n🔗 Connecting to {self.address}...")

        try:
            self.sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
            self.sock.connect((self.address, SPP_CHANNEL))
            print("✅ Connected!")
            return True

        except bluetooth.BluetoothError as e:
            print(f"❌ Connection failed: {e}")
            print("\n💡 Tips:")
            print("   • Ensure device is powered on")
            print("   • Pair in system settings first")
            print("   • Close other Bluetooth apps")
            return False

    def disconnect(self):
        """Disconnect and cleanup"""
        if self.sock:
            try:
                self.sock.close()
                print("🔌 Disconnected")
            except:
                pass
        self.sock = None

    def send(self, data):
        """Send data"""
        if self.sock:
            self.sock.send(data)

    def recv(self, size=1024, timeout=0.1):
        """Receive data with timeout"""
        if self.sock:
            self.sock.settimeout(timeout)
            try:
                return self.sock.recv(size)
            except bluetooth.BluetoothError:
                return b''
        return b''

# ============================================================================
# PACKET PARSER
# ============================================================================

class PacketParser:
    """Binary packet parser"""

    def __init__(self):
        self.buffer = bytearray()
        self.packets = 0
        self.errors = 0

    def feed(self, data):
        """Add data to buffer"""
        self.buffer.extend(data)

    def parse_one(self):
        """Parse one packet from buffer"""
        if len(self.buffer) < HEADER_SIZE:
            return None

        # Find magic byte
        if self.buffer[0] != MAGIC_BYTE:
            try:
                idx = self.buffer.index(MAGIC_BYTE)
                self.buffer = self.buffer[idx:]
            except ValueError:
                self.buffer.clear()
                return None

        # Parse header
        if len(self.buffer) < HEADER_SIZE:
            return None

        magic = self.buffer[0]
        code = self.buffer[1]
        timestamp = struct.unpack('<I', self.buffer[2:6])[0]
        count = struct.unpack('<H', self.buffer[6:8])[0]

        # Validate
        if code != STREAM_CODE or count > MAX_SAMPLES:
            self.errors += 1
            self.buffer = self.buffer[1:]
            return None

        # Wait for complete packet
        packet_size = HEADER_SIZE + count * SAMPLE_SIZE
        if len(self.buffer) < packet_size:
            return None

        # Parse samples
        samples = []
        for i in range(count):
            offset = HEADER_SIZE + i * SAMPLE_SIZE
            sample = struct.unpack('<h', self.buffer[offset:offset+2])[0]
            samples.append(sample)

        # Remove from buffer
        self.buffer = self.buffer[packet_size:]
        self.packets += 1

        return {'timestamp': timestamp, 'samples': samples}

    def parse_all(self):
        """Parse all available packets"""
        packets = []
        while True:
            packet = self.parse_one()
            if packet is None:
                break
            packets.append(packet)
        return packets

# ============================================================================
# DATA CAPTURE
# ============================================================================

def capture_data(conn, duration=CAPTURE_DURATION):
    """Capture streaming data"""

    print(f"\n{'='*60}")
    print(f"  DATA CAPTURE - {duration}s @ {SAMPLING_RATE} Hz")
    print(f"{'='*60}\n")

    # Start streaming
    start_cmd = '{"cd":11,"mt":"x"}\0'.encode('ascii')
    print("📤 START_STREAM → {\"cd\":11,\"mt\":\"x\"}")
    conn.send(start_cmd)
    time.sleep(0.5)

    # Capture loop
    parser = PacketParser()
    all_samples = []
    all_timestamps = []

    start_time = time.time()
    last_print = 0

    print("📊 Capturing...")
    print()

    try:
        while time.time() - start_time < duration:
            # Read data
            data = conn.recv(1024, timeout=0.1)
            if data:
                parser.feed(data)
                packets = parser.parse_all()

                for packet in packets:
                    all_samples.extend(packet['samples'])
                    for _ in packet['samples']:
                        all_timestamps.append(len(all_timestamps) / SAMPLING_RATE)

            # Progress
            elapsed = time.time() - start_time
            if elapsed - last_print >= 2.0:
                pct = int(elapsed / duration * 100)
                print(f"   {pct:3d}% | Samples: {len(all_samples):4d} | Packets: {parser.packets:3d}")
                last_print = elapsed

    except KeyboardInterrupt:
        print("\n⚠️  Interrupted")

    # Stop streaming
    stop_cmd = '{"cd":12,"mt":"x"}\0'.encode('ascii')
    print(f"\n📤 STOP_STREAM → {{\"cd\":12,\"mt\":\"x\"}}")
    conn.send(stop_cmd)
    time.sleep(0.2)

    # Stats
    elapsed = time.time() - start_time
    expected = SAMPLING_RATE * duration
    actual_rate = len(all_samples) / elapsed if elapsed > 0 else 0

    print(f"\n{'='*60}")
    print(f"  RESULTS")
    print(f"{'='*60}")
    print(f"  Duration:     {elapsed:.2f} s")
    print(f"  Samples:      {len(all_samples)} / {expected} expected")
    print(f"  Packets:      {parser.packets}")
    print(f"  Rate:         {actual_rate:.1f} Hz")
    print(f"  Errors:       {parser.errors}")
    print(f"  Complete:     {len(all_samples)/expected*100:.1f}%")
    print(f"{'='*60}\n")

    return np.array(all_samples), np.array(all_timestamps), parser.packets

# ============================================================================
# ANALYSIS
# ============================================================================

def analyze(samples, timestamps):
    """Analyze and visualize"""

    if len(samples) == 0:
        print("❌ No data to analyze")
        return

    # Convert to voltage
    voltages = samples * 0.0001875  # ADS1115 LSB

    # Stats
    print(f"{'='*60}")
    print(f"  SIGNAL ANALYSIS")
    print(f"{'='*60}")
    print(f"  ADC range:    {samples.min()} to {samples.max()}")
    print(f"  Voltage:      {voltages.min():.4f} to {voltages.max():.4f} V")
    print(f"  Mean:         {voltages.mean():.4f} V")
    print(f"  Std:          {voltages.std():.4f} V")
    print(f"  RMS:          {np.sqrt(np.mean(voltages**2)):.4f} V")
    print(f"{'='*60}\n")

    # FFT
    fft_vals = np.fft.fft(samples)
    fft_freq = np.fft.fftfreq(len(samples), 1/SAMPLING_RATE)
    pos_idx = fft_freq > 0
    fft_mag = np.abs(fft_vals[pos_idx])
    fft_freq = fft_freq[pos_idx]

    peak_idx = np.argmax(fft_mag)
    print(f"  Peak freq:    {fft_freq[peak_idx]:.2f} Hz")
    print(f"  Peak mag:     {fft_mag[peak_idx]:.2f}\n")

    # Plot
    fig, ax = plt.subplots(3, 1, figsize=(12, 9))
    fig.suptitle(f'Bluetooth sEMG Capture - {SAMPLING_RATE} Hz', fontsize=14, fontweight='bold')

    # Raw
    ax[0].plot(timestamps, samples, 'b-', lw=0.5)
    ax[0].set_ylabel('ADC Value')
    ax[0].set_title('Raw Signal (Filtered: 10-50 Hz + Notch 60 Hz)')
    ax[0].grid(alpha=0.3)

    # Voltage
    ax[1].plot(timestamps, voltages*1000, 'g-', lw=0.5)
    ax[1].axhline(np.sqrt(np.mean(voltages**2))*1000, color='r', ls='--', label='RMS')
    ax[1].set_ylabel('Voltage (mV)')
    ax[1].set_title('Voltage')
    ax[1].grid(alpha=0.3)
    ax[1].legend()

    # FFT
    ax[2].plot(fft_freq, fft_mag, 'r-', lw=1.5)
    ax[2].axvspan(10, 50, alpha=0.2, color='green', label='Passband')
    ax[2].axvline(60, color='orange', ls='--', label='Notch')
    ax[2].set_xlabel('Frequency (Hz)')
    ax[2].set_ylabel('Magnitude')
    ax[2].set_title('Frequency Spectrum')
    ax[2].set_xlim([0, min(100, SAMPLING_RATE/2)])
    ax[2].grid(alpha=0.3)
    ax[2].legend()

    plt.tight_layout(rect=[0, 0, 1, 0.97])

    # Save
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    plot_file = f"bluetooth_capture_{ts}.png"
    csv_file = f"bluetooth_capture_{ts}.csv"

    plt.savefig(plot_file, dpi=300)
    print(f"📊 Plot: {plot_file}")

    data = np.column_stack((timestamps, samples, voltages))
    np.savetxt(csv_file, data, delimiter=',', header="Time(s),ADC,Voltage(V)", comments='')
    print(f"💾 CSV:  {csv_file}\n")

    plt.show()

# ============================================================================
# MAIN
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description='Auto Bluetooth sEMG Capture')
    parser.add_argument('--device', default=DEVICE_NAME, help='Device name')
    parser.add_argument('--duration', type=int, default=CAPTURE_DURATION, help='Duration (s)')
    args = parser.parse_args()

    print(f"\n{'='*60}")
    print(f"  AUTOMATIC BLUETOOTH sEMG CAPTURE")
    print(f"{'='*60}\n")

    # Create connection
    conn = BluetoothConnection(args.device)

    try:
        # Scan
        if not conn.scan(timeout=10):
            print("\n❌ Scan failed")
            return 1

        # Connect
        if not conn.connect():
            print("\n❌ Connection failed")
            return 1

        # Capture
        samples, timestamps, packets = capture_data(conn, args.duration)

        if len(samples) == 0:
            print("❌ No data captured")
            return 1

        # Analyze
        analyze(samples, timestamps)

        print("✅ Test completed successfully!\n")
        return 0

    except KeyboardInterrupt:
        print("\n⚠️  Interrupted by user\n")
        return 1

    except Exception as e:
        print(f"\n❌ Error: {e}\n")
        return 1

    finally:
        # Always disconnect
        conn.disconnect()

if __name__ == "__main__":
    exit(main())
