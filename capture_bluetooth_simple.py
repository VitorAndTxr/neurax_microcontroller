#!/usr/bin/env python3
"""
Simple Bluetooth sEMG Capture (Serial Port Auto-Detection)
Auto-detects Bluetooth COM port and captures 10 seconds of data.

This script works on Windows/Linux/Mac without requiring PyBluez.
It uses pyserial to connect to the Bluetooth device via its virtual COM port.

Requirements:
    pip install pyserial matplotlib numpy scipy

Usage:
    python capture_bluetooth_simple.py
    python capture_bluetooth_simple.py --duration 20
    python capture_bluetooth_simple.py --port COM6
"""

import serial
import serial.tools.list_ports
import struct
import time
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
from scipy import signal as scipy_signal
import argparse
import sys

# ============================================================================
# CONFIGURATION
# ============================================================================
DEVICE_NAME = "NeuroEstimulator"
SAMPLING_RATE = 215  # Hz
CAPTURE_DURATION = 10  # seconds
BAUD_RATE = 9600  # HC-05/HC-06 default baud rate

# Protocol constants
MAGIC_BYTE = 0xAA
STREAM_CODE = 0x0D
HEADER_SIZE = 8
SAMPLE_SIZE = 2

# ============================================================================
# PORT DETECTION
# ============================================================================

def find_bluetooth_port(device_name=DEVICE_NAME):
    """Auto-detect Bluetooth COM port for device"""
    print(f"🔍 Searching for '{device_name}' on serial ports...")

    ports = serial.tools.list_ports.comports()

    if not ports:
        print("   No serial ports found")
        return None

    print(f"\n📡 Found {len(ports)} serial port(s):")

    target_port = None
    for port in ports:
        # Check if device name matches
        is_match = (device_name.lower() in port.description.lower() or
                   device_name.lower() in str(port.manufacturer).lower() if port.manufacturer else False)

        marker = "✅" if is_match else "   "
        print(f"   {marker} {port.device}")
        print(f"       Description: {port.description}")
        if port.manufacturer:
            print(f"       Manufacturer: {port.manufacturer}")

        if is_match and target_port is None:
            target_port = port.device

    if target_port:
        print(f"\n✅ Target port: {target_port}")
    else:
        print(f"\n⚠️  '{device_name}' not found")
        print("   Available ports:")
        for port in ports:
            print(f"   • {port.device}: {port.description}")

    return target_port

# ============================================================================
# PACKET PARSER
# ============================================================================

class PacketParser:
    """Parse binary streaming packets"""

    def __init__(self):
        self.buffer = bytearray()
        self.packets_received = 0
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

        if len(self.buffer) < HEADER_SIZE:
            return None

        # Parse header
        magic = self.buffer[0]
        code = self.buffer[1]
        timestamp = struct.unpack('<I', self.buffer[2:6])[0]
        count = struct.unpack('<H', self.buffer[6:8])[0]

        # Validate
        if code != STREAM_CODE or count > 50:
            self.errors += 1
            self.buffer = self.buffer[1:]
            return None

        # Check if complete packet available
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
        self.packets_received += 1

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

def send_command(ser, cmd_json):
    """Send JSON command"""
    cmd_bytes = (cmd_json + '\0').encode('ascii')
    ser.write(cmd_bytes)
    ser.flush()

def capture_data(port, duration=CAPTURE_DURATION):
    """Capture streaming data via serial port"""

    print(f"\n{'='*60}")
    print(f"  DATA CAPTURE - {duration}s @ {SAMPLING_RATE} Hz")
    print(f"{'='*60}\n")

    # Open serial port
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        print(f"✅ Connected to {port} @ {BAUD_RATE} baud")
    except serial.SerialException as e:
        print(f"❌ Failed to open {port}: {e}")
        return None, None, 0

    # Flush buffers
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    time.sleep(0.5)

    # Send start command
    print(f"📤 START_STREAM → {{\"cd\":11,\"mt\":\"x\"}}")
    send_command(ser, '{"cd":11,"mt":"x"}')

    # Wait for JSON ACK response
    time.sleep(0.5)
    if ser.in_waiting > 0:
        ack_data = ser.read(ser.in_waiting)
        try:
            ack_str = ack_data.decode('ascii', errors='ignore').strip()
            if '{"cd":11,"mt":"a"}' in ack_str:
                print(f"✅ ACK received - streaming started")
            else:
                print(f"⚠️  Unexpected ACK: {ack_str}")
        except:
            print(f"⚠️  Non-ASCII ACK received")

    # Capture loop
    parser = PacketParser()
    all_samples = []
    all_timestamps = []

    start_time = time.time()
    last_print = 0

    print("📊 Capturing...\n")

    try:
        while time.time() - start_time < duration:
            # Read data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
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
                print(f"   {pct:3d}% | Samples: {len(all_samples):4d} | Packets: {parser.packets_received:3d}")
                last_print = elapsed

            time.sleep(0.001)

    except KeyboardInterrupt:
        print("\n⚠️  Interrupted")

    # Stop streaming
    print(f"\n📤 STOP_STREAM → {{\"cd\":12,\"mt\":\"x\"}}")
    send_command(ser, '{"cd":12,"mt":"x"}')
    time.sleep(0.2)

    # Close port
    ser.close()
    print("🔌 Port closed")

    # Stats
    elapsed = time.time() - start_time
    expected = SAMPLING_RATE * duration
    actual_rate = len(all_samples) / elapsed if elapsed > 0 else 0

    print(f"\n{'='*60}")
    print(f"  RESULTS")
    print(f"{'='*60}")
    print(f"  Duration:     {elapsed:.2f} s")
    print(f"  Samples:      {len(all_samples)} / {expected} expected")
    print(f"  Packets:      {parser.packets_received}")
    print(f"  Rate:         {actual_rate:.1f} Hz")
    print(f"  Errors:       {parser.errors}")
    print(f"  Complete:     {len(all_samples)/expected*100:.1f}%")
    print(f"{'='*60}\n")

    return np.array(all_samples), np.array(all_timestamps), parser.packets_received

# ============================================================================
# ANALYSIS
# ============================================================================

def analyze_and_plot(samples, timestamps):
    """Analyze and visualize data"""

    if len(samples) == 0:
        print("❌ No data to analyze")
        return

    # Convert to voltage (ADS1115 LSB = 0.1875 mV)
    voltages = samples * 0.0001875

    # Statistics
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
    fft_freq_pos = fft_freq[pos_idx]

    peak_idx = np.argmax(fft_mag)
    print(f"  Peak freq:    {fft_freq_pos[peak_idx]:.2f} Hz")
    print(f"  Peak mag:     {fft_mag[peak_idx]:.2f}\n")

    # Plot
    fig, ax = plt.subplots(3, 1, figsize=(12, 9))
    fig.suptitle(f'Bluetooth sEMG Capture - {SAMPLING_RATE} Hz', fontsize=14, fontweight='bold')

    # Raw ADC
    ax[0].plot(timestamps, samples, 'b-', lw=0.5)
    ax[0].set_ylabel('ADC Value')
    ax[0].set_title('Raw Signal (Filtered: 10-50 Hz + Notch 60 Hz)')
    ax[0].grid(alpha=0.3)

    # Voltage
    ax[1].plot(timestamps, voltages*1000, 'g-', lw=0.5)
    rms = np.sqrt(np.mean(voltages**2))*1000
    ax[1].axhline(rms, color='r', ls='--', lw=2, label=f'RMS: {rms:.2f} mV')
    ax[1].set_ylabel('Voltage (mV)')
    ax[1].set_title('Voltage Signal')
    ax[1].grid(alpha=0.3)
    ax[1].legend()

    # FFT
    ax[2].plot(fft_freq_pos, fft_mag, 'r-', lw=1.5)
    ax[2].axvspan(10, 50, alpha=0.2, color='green', label='Passband')
    ax[2].axvline(60, color='orange', ls='--', lw=2, label='Notch')
    ax[2].set_xlabel('Frequency (Hz)')
    ax[2].set_ylabel('Magnitude')
    ax[2].set_title('Frequency Spectrum')
    ax[2].set_xlim([0, min(100, SAMPLING_RATE/2)])
    ax[2].grid(alpha=0.3)
    ax[2].legend()

    plt.tight_layout(rect=[0, 0, 1, 0.97])

    # Save files
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    plot_file = f"semg_capture_{ts}.png"
    csv_file = f"semg_capture_{ts}.csv"

    plt.savefig(plot_file, dpi=300)
    print(f"📊 Plot saved: {plot_file}")

    data = np.column_stack((timestamps, samples, voltages))
    np.savetxt(csv_file, data, delimiter=',', header="Time(s),ADC,Voltage(V)", comments='')
    print(f"💾 CSV saved:  {csv_file}\n")

    plt.show()

# ============================================================================
# MAIN
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description='Simple Bluetooth sEMG Capture')
    parser.add_argument('--port', help='Serial port (auto-detect if not specified)')
    parser.add_argument('--device', default=DEVICE_NAME, help='Device name for auto-detection')
    parser.add_argument('--duration', type=int, default=CAPTURE_DURATION, help='Capture duration (s)')
    args = parser.parse_args()

    print(f"\n{'='*60}")
    print(f"  BLUETOOTH sEMG CAPTURE (Serial Auto-Detect)")
    print(f"{'='*60}\n")

    # Determine port
    if args.port:
        port = args.port
        print(f"✅ Using specified port: {port}")
    else:
        port = find_bluetooth_port(args.device)
        if not port:
            print("\n❌ Auto-detection failed")
            print("\n💡 Solutions:")
            print("   1. Pair device in Windows Bluetooth settings")
            print("   2. Specify port manually: --port COM6")
            print("   3. Check device is powered on")
            return 1

    # Capture data
    samples, timestamps, packets = capture_data(port, args.duration)

    if samples is None or len(samples) == 0:
        print("❌ No data captured")
        return 1

    # Analyze
    analyze_and_plot(samples, timestamps)

    print("✅ Test completed successfully!\n")
    return 0

if __name__ == "__main__":
    exit(main())
