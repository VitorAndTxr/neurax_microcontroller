#!/usr/bin/env python3
"""
Bluetooth sEMG Streaming Capture and Visualization
Captures 10 seconds of 215 Hz sEMG data via Bluetooth binary protocol
and generates time-domain and frequency-domain plots.

Requirements:
    pip install pyserial matplotlib numpy scipy

Usage:
    python capture_bluetooth_stream.py [COM_PORT]

Example:
    python capture_bluetooth_stream.py COM6
"""

import serial
import struct
import time
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
from scipy import signal
import sys
import os

# ============================================================================
# PROTOCOL CONSTANTS (matching ESP32 firmware)
# ============================================================================
PACKET_MAGIC_BYTE = 0xAA
PACKET_MESSAGE_CODE_STREAM_DATA = 13
PACKET_HEADER_SIZE = 8  # magic(1) + code(1) + timestamp(4) + count(2)
SAMPLE_SIZE = 2  # int16_t
MAX_SAMPLES_PER_PACKET = 50

# Streaming configuration
SAMPLING_RATE = 215  # Hz
CAPTURE_DURATION = 10  # seconds
EXPECTED_SAMPLES = SAMPLING_RATE * CAPTURE_DURATION

# Serial configuration
BAUD_RATE = 9600  # HC-05/HC-06 default baud rate
TIMEOUT = 2.0  # seconds

# ============================================================================
# BLUETOOTH PACKET PARSER
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
        timestamp = struct.unpack('<I', data[2:6])[0]  # Little-endian uint32_t
        sample_count = struct.unpack('<H', data[6:8])[0]  # Little-endian uint16_t

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

        # Parse samples (int16_t array)
        payload_size = header['sample_count'] * SAMPLE_SIZE
        expected_packet_size = PACKET_HEADER_SIZE + payload_size

        if len(data) < expected_packet_size:
            return None

        samples = []
        for i in range(header['sample_count']):
            offset = PACKET_HEADER_SIZE + (i * SAMPLE_SIZE)
            sample = struct.unpack('<h', data[offset:offset+2])[0]  # Little-endian int16_t
            samples.append(sample)

        self.packets_received += 1
        self.samples_received += len(samples)

        return {
            'timestamp': header['timestamp'],
            'samples': samples
        }

    def find_sync(self):
        """Find magic byte in buffer for packet synchronization"""
        try:
            idx = self.buffer.index(PACKET_MAGIC_BYTE)
            if idx > 0:
                print(f"[SYNC] Skipped {idx} bytes to find magic byte")
                self.buffer = self.buffer[idx:]
            return True
        except ValueError:
            self.buffer.clear()
            return False

    def process_buffer(self):
        """Process buffered data and extract complete packets"""
        packets = []

        while len(self.buffer) >= PACKET_HEADER_SIZE:
            # Check if buffer starts with magic byte
            if self.buffer[0] != PACKET_MAGIC_BYTE:
                if not self.find_sync():
                    break

            # Try to parse header to get packet size
            header = self.parse_header(self.buffer)
            if header is None:
                self.buffer = self.buffer[1:]  # Skip one byte and retry
                continue

            packet_size = PACKET_HEADER_SIZE + (header['sample_count'] * SAMPLE_SIZE)

            # Wait for complete packet
            if len(self.buffer) < packet_size:
                break

            # Parse complete packet
            packet_data = bytes(self.buffer[:packet_size])
            packet = self.parse_packet(packet_data)

            if packet is not None:
                packets.append(packet)

            # Remove processed packet from buffer
            self.buffer = self.buffer[packet_size:]

        return packets

# ============================================================================
# DATA CAPTURE
# ============================================================================

def send_start_command(ser):
    """Send start streaming command via JSON protocol"""
    # Command: {"cd":11,"mt":"x"}
    command = b'{"cd":11,"mt":"x"}\0'
    ser.write(command)
    print("[CMD] Sent START_STREAM command")
    time.sleep(0.5)  # Wait for ESP32 to start streaming

def send_stop_command(ser):
    """Send stop streaming command"""
    # Command: {"cd":12,"mt":"x"}
    command = b'{"cd":12,"mt":"x"}\0'
    ser.write(command)
    print("[CMD] Sent STOP_STREAM command")

def capture_stream(port, duration=CAPTURE_DURATION):
    """Capture streaming data from Bluetooth serial port"""

    print(f"\n{'='*60}")
    print(f"  sEMG STREAMING CAPTURE - {duration}s @ {SAMPLING_RATE} Hz")
    print(f"{'='*60}\n")

    # Open serial port
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=TIMEOUT)
        print(f"[SERIAL] Connected to {port} @ {BAUD_RATE} baud")
    except serial.SerialException as e:
        print(f"[ERROR] Failed to open {port}: {e}")
        return None, None

    # Flush buffers
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    # Send start command
    send_start_command(ser)

    # Wait for JSON ACK response
    time.sleep(0.5)
    if ser.in_waiting > 0:
        ack_data = ser.read(ser.in_waiting)
        try:
            ack_str = ack_data.decode('ascii', errors='ignore').strip()
            if '{"cd":11,"mt":"a"}' in ack_str:
                print("[ACK] Streaming started")
            else:
                print(f"[WARN] Unexpected ACK: {ack_str}")
        except:
            print("[WARN] Non-ASCII ACK received")

    # Initialize parser
    parser = BinaryPacketParser()
    samples = []
    timestamps = []

    start_time = time.time()
    last_progress = 0

    print(f"[CAPTURE] Recording for {duration} seconds...")
    print(f"[CAPTURE] Expected samples: {EXPECTED_SAMPLES}")

    try:
        while time.time() - start_time < duration:
            # Read available data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                parser.buffer.extend(data)

                # Process complete packets
                packets = parser.process_buffer()

                for packet in packets:
                    samples.extend(packet['samples'])
                    # Generate timestamps (relative time in seconds)
                    for i in range(len(packet['samples'])):
                        t = len(timestamps) / SAMPLING_RATE
                        timestamps.append(t)

            # Progress indicator
            progress = int((time.time() - start_time) / duration * 100)
            if progress >= last_progress + 10:
                print(f"[PROGRESS] {progress}% - Samples: {len(samples)}, Packets: {parser.packets_received}")
                last_progress = progress

            time.sleep(0.001)  # Small delay to avoid busy-waiting

    except KeyboardInterrupt:
        print("\n[ABORT] Capture interrupted by user")

    finally:
        # Send stop command
        send_stop_command(ser)
        ser.close()

    # Print statistics
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
# DATA ANALYSIS & VISUALIZATION
# ============================================================================

def analyze_signal(samples, timestamps, sampling_rate=SAMPLING_RATE):
    """Analyze signal in time and frequency domains"""

    if len(samples) == 0:
        print("[ERROR] No samples to analyze!")
        return

    # Convert raw ADC values to voltage (ADS1115: LSB = 0.1875 mV at ±6.144V range)
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

    # FFT for frequency analysis
    N = len(samples)
    fft_values = np.fft.fft(samples)
    fft_freq = np.fft.fftfreq(N, 1/sampling_rate)

    # Only positive frequencies
    positive_freq_idx = fft_freq > 0
    fft_magnitude = np.abs(fft_values[positive_freq_idx])
    fft_freq_positive = fft_freq[positive_freq_idx]

    # Find dominant frequency
    peak_idx = np.argmax(fft_magnitude)
    dominant_freq = fft_freq_positive[peak_idx]

    print(f"  Dominant frequency: {dominant_freq:.2f} Hz")
    print(f"  FFT peak magnitude: {fft_magnitude[peak_idx]:.2f}\n")

    return voltages, fft_freq_positive, fft_magnitude

def plot_results(timestamps, samples, voltages, fft_freq, fft_magnitude):
    """Generate comprehensive visualization"""

    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    fig.suptitle(f'sEMG Signal Analysis - {SAMPLING_RATE} Hz @ {CAPTURE_DURATION}s',
                 fontsize=16, fontweight='bold')

    # ========================================================================
    # PLOT 1: Raw ADC Values (Time Domain)
    # ========================================================================
    axes[0].plot(timestamps, samples, 'b-', linewidth=0.5, alpha=0.8)
    axes[0].set_xlabel('Time (s)', fontsize=12)
    axes[0].set_ylabel('ADC Value (int16)', fontsize=12)
    axes[0].set_title('Raw ADC Signal (Filtered: 10-50 Hz + Notch 60 Hz)', fontsize=14)
    axes[0].grid(True, alpha=0.3)
    axes[0].set_xlim([0, timestamps[-1]])

    # Add statistics box
    stats_text = f"Range: [{samples.min()}, {samples.max()}]\n"
    stats_text += f"Mean: {samples.mean():.1f}\n"
    stats_text += f"Std: {samples.std():.1f}"
    axes[0].text(0.02, 0.98, stats_text, transform=axes[0].transAxes,
                 fontsize=10, verticalalignment='top',
                 bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    # ========================================================================
    # PLOT 2: Voltage (Time Domain)
    # ========================================================================
    axes[1].plot(timestamps, voltages * 1000, 'g-', linewidth=0.5, alpha=0.8)  # Convert to mV
    axes[1].set_xlabel('Time (s)', fontsize=12)
    axes[1].set_ylabel('Voltage (mV)', fontsize=12)
    axes[1].set_title('Voltage Signal (mV)', fontsize=14)
    axes[1].grid(True, alpha=0.3)
    axes[1].set_xlim([0, timestamps[-1]])

    # Add RMS line
    rms = np.sqrt(np.mean(voltages**2)) * 1000
    axes[1].axhline(y=rms, color='r', linestyle='--', linewidth=2, label=f'RMS: {rms:.2f} mV')
    axes[1].legend(loc='upper right')

    # ========================================================================
    # PLOT 3: Frequency Spectrum (FFT)
    # ========================================================================
    axes[2].plot(fft_freq, fft_magnitude, 'r-', linewidth=1.5, alpha=0.8)
    axes[2].set_xlabel('Frequency (Hz)', fontsize=12)
    axes[2].set_ylabel('Magnitude', fontsize=12)
    axes[2].set_title('Frequency Spectrum (FFT)', fontsize=14)
    axes[2].grid(True, alpha=0.3)
    axes[2].set_xlim([0, min(100, SAMPLING_RATE/2)])  # Show up to 100 Hz

    # Highlight filter bands
    axes[2].axvspan(10, 50, alpha=0.2, color='green', label='Passband (10-50 Hz)')
    axes[2].axvline(x=60, color='orange', linestyle='--', linewidth=2, label='Notch @ 60 Hz')
    axes[2].legend(loc='upper right')

    # Mark peak frequency
    peak_idx = np.argmax(fft_magnitude)
    peak_freq = fft_freq[peak_idx]
    peak_mag = fft_magnitude[peak_idx]
    axes[2].plot(peak_freq, peak_mag, 'bo', markersize=10, label=f'Peak: {peak_freq:.2f} Hz')
    axes[2].annotate(f'{peak_freq:.1f} Hz', xy=(peak_freq, peak_mag),
                     xytext=(10, 10), textcoords='offset points',
                     bbox=dict(boxstyle='round', facecolor='yellow', alpha=0.7),
                     arrowprops=dict(arrowstyle='->', connectionstyle='arc3,rad=0'))

    plt.tight_layout(rect=[0, 0, 1, 0.97])

    # Save figure
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"semg_analysis_{timestamp}.png"
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f"[PLOT] Saved to: {filename}")

    plt.show()

def save_data(timestamps, samples, voltages):
    """Save captured data to CSV"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"semg_stream_{timestamp}.csv"

    data = np.column_stack((timestamps, samples, voltages))
    header = "Time(s),ADC_Value,Voltage(V)"
    np.savetxt(filename, data, delimiter=',', header=header, comments='')

    print(f"[DATA] Saved to: {filename}")
    return filename

# ============================================================================
# MAIN
# ============================================================================

def main():
    # Parse command line arguments
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        # Try to auto-detect common ports
        if os.name == 'nt':  # Windows
            port = 'COM6'  # Change if needed
        else:  # Linux/Mac
            port = '/dev/ttyUSB0'

    print(f"\nUsing serial port: {port}")
    print(f"(Change with: python {sys.argv[0]} <PORT>)\n")

    # Capture streaming data
    samples, timestamps = capture_stream(port, duration=CAPTURE_DURATION)

    if samples is None or len(samples) == 0:
        print("[ERROR] No data captured. Check Bluetooth connection and streaming commands.")
        return 1

    # Analyze signal
    voltages, fft_freq, fft_magnitude = analyze_signal(samples, timestamps)

    # Save data
    save_data(timestamps, samples, voltages)

    # Plot results
    plot_results(timestamps, samples, voltages, fft_freq, fft_magnitude)

    return 0

if __name__ == "__main__":
    exit(main())
