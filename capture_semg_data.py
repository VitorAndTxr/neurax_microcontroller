#!/usr/bin/env python3
"""
Script to capture 5 seconds of sEMG data from ESP32 via serial port.

Output format: timestamp,raw_adc,filtered_value
Saves to: semg_data_YYYYMMDD_HHMMSS.csv

Usage:
    python capture_semg_data.py
    python capture_semg_data.py --port COM6 --duration 5 --output data.csv
"""

import serial
import time
import argparse
from datetime import datetime
import sys


def capture_semg_data(port='COM6', baudrate=115200, duration=5, output_file=None):
    """
    Capture sEMG data from serial port for specified duration.

    Args:
        port (str): Serial port name (e.g., 'COM6', '/dev/ttyUSB0')
        baudrate (int): Serial baudrate (default: 115200)
        duration (int): Capture duration in seconds (default: 5)
        output_file (str): Output CSV filename (default: auto-generated)

    Returns:
        str: Path to saved CSV file
    """
    # Generate output filename if not provided
    if output_file is None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_file = f"semg_data_{timestamp}.csv"

    print(f"[INFO] Opening serial port {port} @ {baudrate} baud...")

    try:
        # Open serial connection
        ser = serial.Serial(port, baudrate, timeout=1)
        time.sleep(2)  # Wait for connection to stabilize

        # Flush any existing data in buffer
        ser.reset_input_buffer()

        print(f"[INFO] Capturing data for {duration} seconds...")
        print(f"[INFO] Output file: {output_file}")
        print("-" * 60)

        # Open output file
        with open(output_file, 'w') as f:
            # Write CSV header
            f.write("timestamp,raw_adc,filtered_value\n")

            start_time = time.time()
            sample_count = 0

            # Capture data for specified duration
            while (time.time() - start_time) < duration:
                if ser.in_waiting > 0:
                    try:
                        # Read line from serial
                        line = ser.readline().decode('utf-8', errors='ignore').strip()

                        # Skip empty lines and log messages
                        if not line or line.startswith('['):
                            continue

                        # Check if line is valid CSV (has commas)
                        if ',' in line:
                            # Parse and validate data
                            parts = line.split(',')
                            if len(parts) == 3:
                                # Write to file
                                f.write(line + '\n')
                                sample_count += 1

                                # Print progress every 50 samples
                                if sample_count % 50 == 0:
                                    elapsed = time.time() - start_time
                                    rate = sample_count / elapsed if elapsed > 0 else 0
                                    print(f"[PROGRESS] Samples: {sample_count}, "
                                          f"Rate: {rate:.1f} Hz, "
                                          f"Time: {elapsed:.1f}s")

                    except UnicodeDecodeError:
                        # Skip lines that can't be decoded
                        continue

            # Calculate statistics
            elapsed = time.time() - start_time
            avg_rate = sample_count / elapsed if elapsed > 0 else 0

            print("-" * 60)
            print(f"[SUCCESS] Capture complete!")
            print(f"  • Total samples: {sample_count}")
            print(f"  • Duration: {elapsed:.2f} seconds")
            print(f"  • Average rate: {avg_rate:.1f} Hz")
            print(f"  • File saved: {output_file}")

        # Close serial port
        ser.close()

        return output_file

    except serial.SerialException as e:
        print(f"[ERROR] Failed to open serial port {port}: {e}")
        print(f"[HINT] Make sure the port is not in use by another program")
        print(f"[HINT] Close PlatformIO monitor before running this script")
        sys.exit(1)

    except KeyboardInterrupt:
        print("\n[INFO] Capture interrupted by user")
        ser.close()
        sys.exit(0)

    except Exception as e:
        print(f"[ERROR] Unexpected error: {e}")
        ser.close()
        sys.exit(1)


def plot_data(csv_file):
    """
    Optional: Plot the captured data using matplotlib.

    Args:
        csv_file (str): Path to CSV file
    """
    try:
        import pandas as pd
        import matplotlib.pyplot as plt

        print(f"\n[INFO] Plotting data from {csv_file}...")

        # Read CSV
        df = pd.read_csv(csv_file)

        # Create figure with subplots
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

        # Convert timestamp to relative time (seconds)
        df['time_s'] = (df['timestamp'] - df['timestamp'].iloc[0]) / 1000.0

        # Plot raw ADC
        ax1.plot(df['time_s'], df['raw_adc'], 'b-', linewidth=0.5, label='Raw ADC')
        ax1.set_ylabel('Raw ADC Value')
        ax1.set_title('sEMG Signal - Raw ADC (215 Hz downsampled from 860 Hz)')
        ax1.grid(True, alpha=0.3)
        ax1.legend()

        # Plot filtered signal
        ax2.plot(df['time_s'], df['filtered_value'], 'r-', linewidth=0.8, label='Filtered (10-50 Hz + 60 Hz notch)')
        ax2.set_xlabel('Time (seconds)')
        ax2.set_ylabel('Filtered Value')
        ax2.set_title('sEMG Signal - Butterworth Filtered')
        ax2.grid(True, alpha=0.3)
        ax2.legend()

        plt.tight_layout()

        # Save plot
        plot_file = csv_file.replace('.csv', '_plot.png')
        plt.savefig(plot_file, dpi=300, bbox_inches='tight')
        print(f"[SUCCESS] Plot saved to {plot_file}")

        # Show plot
        plt.show()

    except ImportError:
        print("[INFO] matplotlib/pandas not installed. Skipping plot.")
        print("[HINT] Install with: pip install matplotlib pandas")
    except Exception as e:
        print(f"[ERROR] Failed to plot data: {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Capture sEMG data from ESP32 via serial port',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Capture 5 seconds on COM6 (default)
  python capture_semg_data.py

  # Capture 10 seconds on COM3
  python capture_semg_data.py --port COM3 --duration 10

  # Custom output file
  python capture_semg_data.py --output my_data.csv

  # Plot after capture
  python capture_semg_data.py --plot
        """
    )

    parser.add_argument('--port', '-p', type=str, default='COM6',
                        help='Serial port (default: COM6)')
    parser.add_argument('--baudrate', '-b', type=int, default=115200,
                        help='Baudrate (default: 115200)')
    parser.add_argument('--duration', '-d', type=int, default=5,
                        help='Capture duration in seconds (default: 5)')
    parser.add_argument('--output', '-o', type=str, default=None,
                        help='Output CSV filename (default: auto-generated)')
    parser.add_argument('--plot', action='store_true',
                        help='Plot data after capture (requires matplotlib)')

    args = parser.parse_args()

    # Capture data
    csv_file = capture_semg_data(
        port=args.port,
        baudrate=args.baudrate,
        duration=args.duration,
        output_file=args.output
    )

    # Plot if requested
    if args.plot:
        plot_data(csv_file)
