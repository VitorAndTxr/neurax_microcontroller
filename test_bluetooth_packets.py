#!/usr/bin/env python3
"""
Simple Bluetooth Packet Tester
Displays raw packet structure and validates protocol compliance
"""

import serial
import struct
import time
import sys

# Configuration
BAUD_RATE = 115200
MAGIC_BYTE = 0xAA
STREAM_DATA_CODE = 0x0D

def hex_dump(data, label="Data"):
    """Display data as hex dump"""
    print(f"\n{label} ({len(data)} bytes):")
    hex_str = ' '.join(f'{b:02X}' for b in data[:32])  # First 32 bytes
    if len(data) > 32:
        hex_str += " ..."
    print(f"  {hex_str}")

def parse_packet_header(data):
    """Parse and display packet header"""
    if len(data) < 8:
        print("  [ERROR] Insufficient data for header")
        return None

    magic = data[0]
    code = data[1]
    timestamp = struct.unpack('<I', data[2:6])[0]
    count = struct.unpack('<H', data[6:8])[0]

    print(f"\n📦 PACKET HEADER:")
    print(f"  Magic Byte:    0x{magic:02X} {'✅' if magic == MAGIC_BYTE else '❌ INVALID'}")
    print(f"  Message Code:  0x{code:02X} (decimal: {code}) {'✅' if code == STREAM_DATA_CODE else '❌'}")
    print(f"  Timestamp:     {timestamp} ms ({timestamp/1000:.2f} seconds)")
    print(f"  Sample Count:  {count} samples")
    print(f"  Expected Size: {8 + count*2} bytes")

    return {'magic': magic, 'code': code, 'timestamp': timestamp, 'count': count}

def parse_samples(data, offset=8, count=5):
    """Parse and display first few samples"""
    print(f"\n📊 SAMPLE DATA (first {count}):")
    samples = []
    for i in range(min(count, (len(data)-offset)//2)):
        pos = offset + i*2
        sample = struct.unpack('<h', data[pos:pos+2])[0]  # int16_t
        voltage_mv = sample * 0.1875  # ADS1115 LSB = 0.1875 mV
        samples.append(sample)
        print(f"  Sample[{i}]: {sample:6d} (raw) → {voltage_mv:8.2f} mV")

    return samples

def test_bluetooth_streaming(port):
    """Main test function"""
    print(f"{'='*60}")
    print(f"  BLUETOOTH PACKET TESTER")
    print(f"{'='*60}\n")

    # Open serial port
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=2)
        print(f"✅ Connected to {port} @ {BAUD_RATE} baud\n")
    except Exception as e:
        print(f"❌ Failed to open {port}: {e}")
        return 1

    # Flush buffers
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    time.sleep(0.5)

    # Send start streaming command
    start_cmd = b'{"cd":11,"mt":"x"}\0'
    print("📤 Sending START_STREAM command:")
    print(f"   {start_cmd.decode('ascii', errors='ignore')}")
    ser.write(start_cmd)
    time.sleep(1)  # Wait for response

    # Check for ACK response
    print("\n📥 Waiting for ACK response...")
    ack_data = ser.read(100)
    if ack_data:
        try:
            ack_str = ack_data.decode('ascii', errors='ignore')
            print(f"   Received: {ack_str}")
            if '{"cd":11,"mt":"a"}' in ack_str:
                print("   ✅ ACK confirmed!")
        except:
            print("   ⚠️  Non-ASCII data (might be binary packets starting)")

    # Capture and analyze first 3 packets
    print(f"\n{'='*60}")
    print(f"  CAPTURING BINARY PACKETS")
    print(f"{'='*60}")

    buffer = bytearray()
    packets_analyzed = 0

    try:
        start_time = time.time()
        while packets_analyzed < 3 and (time.time() - start_time) < 10:
            # Read available data
            if ser.in_waiting > 0:
                buffer.extend(ser.read(ser.in_waiting))

            # Try to find and parse packet
            if len(buffer) >= 8:
                # Look for magic byte
                try:
                    idx = buffer.index(MAGIC_BYTE)
                    if idx > 0:
                        print(f"\n⚠️  Skipped {idx} bytes to find magic byte")
                        buffer = buffer[idx:]
                except ValueError:
                    buffer.clear()
                    continue

                # Parse header to get packet size
                if buffer[0] == MAGIC_BYTE:
                    print(f"\n{'─'*60}")
                    print(f"PACKET #{packets_analyzed + 1}")
                    print(f"{'─'*60}")

                    hex_dump(buffer, "Raw Data")
                    header = parse_packet_header(buffer)

                    if header:
                        packet_size = 8 + header['count'] * 2

                        # Wait for complete packet
                        if len(buffer) >= packet_size:
                            parse_samples(buffer, offset=8, count=5)

                            # Validate packet
                            print(f"\n✅ PACKET VALID ({packet_size} bytes)")

                            # Remove packet from buffer
                            buffer = buffer[packet_size:]
                            packets_analyzed += 1
                        else:
                            print(f"\n⏳ Waiting for complete packet ({len(buffer)}/{packet_size} bytes)")
                    else:
                        buffer = buffer[1:]  # Skip bad byte

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\n\n⚠️  Interrupted by user")

    # Send stop command
    print(f"\n{'='*60}")
    stop_cmd = b'{"cd":12,"mt":"x"}\0'
    print(f"📤 Sending STOP_STREAM command:")
    print(f"   {stop_cmd.decode('ascii', errors='ignore')}")
    ser.write(stop_cmd)
    time.sleep(0.5)

    ser.close()

    # Summary
    print(f"\n{'='*60}")
    print(f"  TEST SUMMARY")
    print(f"{'='*60}")
    print(f"  Packets analyzed: {packets_analyzed}")
    print(f"  Expected: 3 packets @ 215 Hz")
    print(f"  Status: {'✅ PASS' if packets_analyzed >= 3 else '⚠️ INCOMPLETE'}")
    print(f"{'='*60}\n")

    return 0

if __name__ == "__main__":
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = "COM6"  # Default Windows port
        print(f"Usage: python {sys.argv[0]} <PORT>")
        print(f"Using default port: {port}\n")

    exit(test_bluetooth_streaming(port))
