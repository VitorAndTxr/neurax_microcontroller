#!/usr/bin/env python3
"""
Serial Monitor sEMG Data Capture and Visualization
Captures 10 seconds of filtered sEMG values from Serial.println() output
and generates time-domain and frequency-domain plots.

Requirements:
    pip install pyserial matplotlib numpy scipy

Usage:
    python capture_serial_stream.py [COM_PORT]

Example:
    python capture_serial_stream.py COM6
"""

import serial
import time
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
from scipy import signal
import sys
import os

# ============================================================================
# CONFIGURATION
# ============================================================================
SAMPLING_RATE = 215  # Hz (expected from streaming task)
CAPTURE_DURATION = 10  # seconds
EXPECTED_SAMPLES = SAMPLING_RATE * CAPTURE_DURATION
BAUD_RATE = 115200
TIMEOUT = 1.0

# ============================================================================
# DATA CAPTURE
# ============================================================================

def capture_serial_data(port, duration=CAPTURE_DURATION):
    """Capture filtered values from Serial.println() output"""

    print(f"\n{'='*60}")
    print(f"  SERIAL sEMG CAPTURE - {duration}s @ {SAMPLING_RATE} Hz")
    print(f"{'='*60}\n")

    # Open serial port
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=TIMEOUT)
        print(f"[SERIAL] Connected to {port} @ {BAUD_RATE} baud")
        time.sleep(2)  # Wait for connection to stabilize
    except serial.SerialException as e:
        print(f"[ERROR] Failed to open {port}: {e}")
        print(f"[TIP] Check if port is correct and not in use by PlatformIO")
        return None, None

    # Flush buffers
    ser.reset_input_buffer()

    samples = []
    start_time = time.time()
    last_progress = 0

    print(f"[CAPTURE] Recording for {duration} seconds...")
    print(f"[CAPTURE] Expected samples: {EXPECTED_SAMPLES}")
    print(f"[CAPTURE] Waiting for numeric data...\n")

    try:
        while time.time() - start_time < duration:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()

                    # Skip empty lines and debug messages
                    if not line or line.startswith('['):
                        continue

                    # Try to parse as float
                    try:
                        value = float(line)
                        samples.append(value)
                    except ValueError:
                        # Not a number, skip
                        continue

                except UnicodeDecodeError:
                    continue

            # Progress indicator
            elapsed = time.time() - start_time
            progress = int(elapsed / duration * 100)
            if progress >= last_progress + 10:
                print(f"[PROGRESS] {progress}% - Samples: {len(samples)}, Rate: {len(samples)/elapsed:.1f} Hz")
                last_progress = progress

            time.sleep(0.001)  # Small delay

    except KeyboardInterrupt:
        print("\n[ABORT] Capture interrupted by user")

    finally:
        ser.close()

    # Calculate statistics
    elapsed = time.time() - start_time
    actual_rate = len(samples) / elapsed if elapsed > 0 else 0

    print(f"\n{'='*60}")
    print(f"  CAPTURE STATISTICS")
    print(f"{'='*60}")
    print(f"  Duration:        {elapsed:.2f} seconds")
    print(f"  Samples:         {len(samples)} / {EXPECTED_SAMPLES} expected")
    print(f"  Actual rate:     {actual_rate:.1f} Hz")
    print(f"  Completeness:    {len(samples)/EXPECTED_SAMPLES*100:.1f}%")
    print(f"{'='*60}\n")

    if len(samples) == 0:
        print("[ERROR] No numeric data captured!")
        print("[TIP] Make sure streaming is active (send command via Bluetooth app)")
        return None, None

    # Generate timestamps
    timestamps = np.arange(len(samples)) / actual_rate

    return np.array(samples), timestamps

# ============================================================================
# DATA ANALYSIS
# ============================================================================

def analyze_signal(samples, timestamps):
    """Analyze signal statistics and frequency content"""

    if len(samples) == 0:
        print("[ERROR] No samples to analyze!")
        return None, None, None

    # Statistics
    print(f"\n{'='*60}")
    print(f"  SIGNAL ANALYSIS")
    print(f"{'='*60}")
    print(f"  Value range:     {samples.min():.2f} to {samples.max():.2f}")
    print(f"  Mean:            {samples.mean():.2f}")
    print(f"  Std deviation:   {samples.std():.2f}")
    print(f"  RMS:             {np.sqrt(np.mean(samples**2)):.2f}")
    print(f"{'='*60}\n")

    # FFT for frequency analysis
    N = len(samples)
    actual_rate = len(samples) / (timestamps[-1] - timestamps[0]) if len(timestamps) > 1 else SAMPLING_RATE

    fft_values = np.fft.fft(samples)
    fft_freq = np.fft.fftfreq(N, 1/actual_rate)

    # Only positive frequencies
    positive_freq_idx = fft_freq > 0
    fft_magnitude = np.abs(fft_values[positive_freq_idx])
    fft_freq_positive = fft_freq[positive_freq_idx]

    # Find dominant frequency
    if len(fft_magnitude) > 0:
        peak_idx = np.argmax(fft_magnitude)
        dominant_freq = fft_freq_positive[peak_idx]
        print(f"  Dominant frequency: {dominant_freq:.2f} Hz")
        print(f"  FFT peak magnitude: {fft_magnitude[peak_idx]:.2f}\n")

    return samples, fft_freq_positive, fft_magnitude

# ============================================================================
# VISUALIZATION
# ============================================================================

def plot_results(timestamps, samples, fft_freq, fft_magnitude):
    """Generate comprehensive visualization"""

    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    fig.suptitle(f'sEMG Filtered Signal Analysis - Serial Capture @ {CAPTURE_DURATION}s',
                 fontsize=16, fontweight='bold')

    # ========================================================================
    # PLOT 1: Filtered Signal (Time Domain - Full)
    # ========================================================================
    axes[0].plot(timestamps, samples, 'b-', linewidth=0.8, alpha=0.8)
    axes[0].set_xlabel('Time (s)', fontsize=12)
    axes[0].set_ylabel('Filtered Value', fontsize=12)
    axes[0].set_title('Filtered sEMG Signal (10-50 Hz + Notch 60 Hz)', fontsize=14)
    axes[0].grid(True, alpha=0.3)
    axes[0].set_xlim([timestamps[0], timestamps[-1]])

    # Add statistics box
    stats_text = f"Range: [{samples.min():.2f}, {samples.max():.2f}]\n"
    stats_text += f"Mean: {samples.mean():.2f}\n"
    stats_text += f"Std: {samples.std():.2f}\n"
    stats_text += f"RMS: {np.sqrt(np.mean(samples**2)):.2f}"
    axes[0].text(0.02, 0.98, stats_text, transform=axes[0].transAxes,
                 fontsize=10, verticalalignment='top',
                 bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    # Add RMS line
    rms = np.sqrt(np.mean(samples**2))
    axes[0].axhline(y=rms, color='r', linestyle='--', linewidth=2, alpha=0.5, label=f'RMS: {rms:.2f}')
    axes[0].axhline(y=-rms, color='r', linestyle='--', linewidth=2, alpha=0.5)
    axes[0].legend(loc='upper right')

    # ========================================================================
    # PLOT 2: Zoomed View (First 2 seconds)
    # ========================================================================
    zoom_duration = min(2.0, timestamps[-1])
    zoom_idx = timestamps <= zoom_duration

    axes[1].plot(timestamps[zoom_idx], samples[zoom_idx], 'g-', linewidth=1.2, alpha=0.8)
    axes[1].set_xlabel('Time (s)', fontsize=12)
    axes[1].set_ylabel('Filtered Value', fontsize=12)
    axes[1].set_title('Zoomed View (First 2 seconds)', fontsize=14)
    axes[1].grid(True, alpha=0.3)
    axes[1].set_xlim([0, zoom_duration])

    # ========================================================================
    # PLOT 3: Frequency Spectrum (FFT)
    # ========================================================================
    axes[2].semilogy(fft_freq, fft_magnitude, 'r-', linewidth=1.5, alpha=0.8)
    axes[2].set_xlabel('Frequency (Hz)', fontsize=12)
    axes[2].set_ylabel('Magnitude (log scale)', fontsize=12)
    axes[2].set_title('Frequency Spectrum (FFT)', fontsize=14)
    axes[2].grid(True, alpha=0.3, which='both')
    axes[2].set_xlim([0, min(100, len(fft_freq))])

    # Highlight filter bands
    axes[2].axvspan(10, 50, alpha=0.2, color='green', label='Passband (10-50 Hz)')
    axes[2].axvline(x=60, color='orange', linestyle='--', linewidth=2, label='Notch @ 60 Hz')

    # Mark peak frequency
    if len(fft_magnitude) > 0:
        peak_idx = np.argmax(fft_magnitude)
        peak_freq = fft_freq[peak_idx]
        peak_mag = fft_magnitude[peak_idx]
        axes[2].plot(peak_freq, peak_mag, 'bo', markersize=10)
        axes[2].annotate(f'{peak_freq:.1f} Hz', xy=(peak_freq, peak_mag),
                         xytext=(10, 10), textcoords='offset points',
                         bbox=dict(boxstyle='round', facecolor='yellow', alpha=0.7),
                         arrowprops=dict(arrowstyle='->', connectionstyle='arc3,rad=0'))

    axes[2].legend(loc='upper right')

    plt.tight_layout(rect=[0, 0, 1, 0.97])

    # Save figure
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"semg_serial_analysis_{timestamp}.png"
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f"[PLOT] Saved to: {filename}")

    plt.show()

def save_data(timestamps, samples):
    """Save captured data to CSV"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"semg_serial_data_{timestamp}.csv"

    data = np.column_stack((timestamps, samples))
    header = "Time(s),Filtered_Value"
    np.savetxt(filename, data, delimiter=',', header=header, comments='', fmt='%.6f')

    print(f"[DATA] Saved to: {filename}")
    return filename

# ============================================================================
# MAIN
# ============================================================================

def main():
    print("\n" + "="*60)
    print("  Serial sEMG Data Capture Tool")
    print("="*60)

    # Parse command line arguments
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        # Try to auto-detect
        if os.name == 'nt':  # Windows
            port = 'COM6'
        else:  # Linux/Mac
            port = '/dev/ttyUSB0'

    print(f"\nUsing serial port: {port}")
    print(f"(Change with: python {sys.argv[0]} <PORT>)\n")

    print("IMPORTANT:")
    print("1. Make sure streaming is active on ESP32")
    print("2. Close PlatformIO serial monitor if open")
    print("3. Send start command via Bluetooth app: {\"cd\":11,\"mt\":\"x\"}")
    print("\nPress Ctrl+C to abort capture\n")

    time.sleep(2)

    # Capture data
    samples, timestamps = capture_serial_data(port, duration=CAPTURE_DURATION)

    if samples is None or len(samples) < 100:
        print("\n[ERROR] Insufficient data captured.")
        print("[TIP] Check:")
        print("  1. Streaming is active (Bluetooth command sent)")
        print("  2. Serial monitor shows numeric values")
        print("  3. Correct COM port selected")
        return 1

    # Analyze
    samples, fft_freq, fft_magnitude = analyze_signal(samples, timestamps)

    if samples is None:
        return 1

    # Save
    save_data(timestamps, samples)

    # Plot
    plot_results(timestamps, samples, fft_freq, fft_magnitude)

    return 0

if __name__ == "__main__":
    exit(main())
