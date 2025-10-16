#!/usr/bin/env python3
"""
Bluetooth Connection Diagnostics
Helps identify why no data is being received from ESP32.

Usage:
    python diagnose_bluetooth.py COM7
"""

import serial
import time
import sys

def diagnose_connection(port, baud=9600):
    """Diagnose Bluetooth connection issues"""

    print(f"\n{'='*60}")
    print(f"  BLUETOOTH CONNECTION DIAGNOSTICS")
    print(f"{'='*60}\n")

    # Open port
    try:
        ser = serial.Serial(port, baud, timeout=2)
        print(f"✅ Port opened: {port} @ {baud} baud")
    except Exception as e:
        print(f"❌ Failed to open port: {e}")
        return

    # Flush buffers
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    time.sleep(1)

    print(f"\n{'─'*60}")
    print("TEST 1: Check if any data is being received")
    print(f"{'─'*60}\n")

    print("Waiting 3 seconds for any incoming data...")
    time.sleep(3)

    if ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        print(f"✅ Received {len(data)} bytes:")
        print(f"   Raw: {data[:100]}")  # First 100 bytes
        try:
            print(f"   ASCII: {data.decode('ascii', errors='ignore')[:200]}")
        except:
            pass
    else:
        print("❌ No data received spontaneously")

    print(f"\n{'─'*60}")
    print("TEST 2: Send START_STREAM command and check ACK + binary stream")
    print(f"{'─'*60}\n")

    # Send start command
    cmd = b'{"cd":11,"mt":"x"}\0'
    print(f"📤 Sending: {cmd}")
    ser.write(cmd)
    ser.flush()

    # STEP 1: Read JSON ACK response
    print("⏳ Waiting for JSON ACK response...")
    time.sleep(0.5)

    ack_received = False
    if ser.in_waiting > 0:
        ack_data = ser.read(ser.in_waiting)
        print(f"✅ Received ACK ({len(ack_data)} bytes):")

        try:
            ack_str = ack_data.decode('ascii', errors='ignore').strip()
            print(f"   JSON ACK: {ack_str}")

            # Check for expected ACK
            if '{"cd":11,"mt":"a"}' in ack_str:
                print("   ✅ Valid ACK confirmed - streaming should start now")
                ack_received = True
            else:
                print("   ⚠️  Unexpected ACK format")
        except:
            print(f"   ⚠️  Non-ASCII ACK: {ack_data[:50]}")
    else:
        print("❌ No ACK received after START command")

    # STEP 2: Read binary streaming data
    if ack_received:
        print("\n⏳ Waiting 2 seconds for binary streaming packets...")
        time.sleep(2)

        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            print(f"✅ Received {len(data)} bytes of streaming data:")

            # Show first bytes as hex
            hex_str = ' '.join(f'{b:02X}' for b in data[:32])
            print(f"   Hex: {hex_str}")

            # Check for magic byte (binary packets)
            if 0xAA in data:
                idx = data.index(0xAA)
                print(f"\n✅ Found magic byte (0xAA) at position {idx}")
                print(f"   This looks like binary streaming data!")

                # Show packet header
                if len(data) >= idx + 8:
                    magic = data[idx]
                    code = data[idx+1]
                    print(f"   Magic: 0x{magic:02X}")
                    print(f"   Code:  0x{code:02X} (should be 0x0D for stream data)")
            else:
                print("\n⚠️  No magic byte (0xAA) found in streaming data")
                print("   Device may not be sending binary packets")
        else:
            print("❌ No binary data received after ACK")

    print(f"\n{'─'*60}")
    print("TEST 3: Try simple commands")
    print(f"{'─'*60}\n")

    # Test gyroscope command
    ser.reset_input_buffer()
    cmd = b'{"cd":1,"mt":"x"}\0'
    print(f"📤 Sending gyroscope test: {cmd}")
    ser.write(cmd)
    ser.flush()

    time.sleep(2)

    if ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        print(f"✅ Received {len(data)} bytes")
        try:
            print(f"   Response: {data.decode('ascii', errors='ignore')}")
        except:
            print(f"   Raw: {data}")
    else:
        print("❌ No response to gyroscope command")

    print(f"\n{'─'*60}")
    print("TEST 4: Monitor raw data for 10 seconds")
    print(f"{'─'*60}\n")

    # Send start again
    ser.reset_input_buffer()
    ser.write(b'{"cd":11,"mt":"x"}\0')
    ser.flush()

    # Wait for ACK and discard it
    print("⏳ Waiting for ACK...")
    time.sleep(0.5)
    if ser.in_waiting > 0:
        ack_data = ser.read(ser.in_waiting)
        try:
            ack_str = ack_data.decode('ascii', errors='ignore').strip()
            print(f"✅ ACK received: {ack_str}")
        except:
            print(f"⚠️  Non-ASCII ACK received")

    print("\n📊 Monitoring for 10 seconds...")
    print("   (Press Ctrl+C to stop early)\n")

    start = time.time()
    total_bytes = 0
    packet_count = 0
    last_print = 0

    try:
        while time.time() - start < 10:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                total_bytes += len(data)

                # Count magic bytes (packets)
                packet_count += data.count(0xAA)

                # Show periodic updates (every 2 seconds)
                elapsed = time.time() - start
                if elapsed - last_print >= 2.0:
                    print(f"   {elapsed:.1f}s: {total_bytes} bytes, ~{packet_count} packets")
                    last_print = elapsed

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\n   Interrupted by user")

    elapsed = time.time() - start
    print(f"\n📊 Summary:")
    print(f"   Total bytes:  {total_bytes}")
    print(f"   Packets (0xAA count): {packet_count}")
    print(f"   Bytes/sec:    {total_bytes/elapsed:.1f}")

    if total_bytes > 0:
        print(f"\n✅ Device is sending data!")
        print(f"   Expected: ~460 bytes/sec (215 Hz, 50 samples/packet)")

        if total_bytes < 100:
            print(f"\n⚠️  Very low data rate - possible issues:")
            print(f"   • Device not streaming continuously")
            print(f"   • Wrong baud rate")
            print(f"   • Buffer issues on ESP32")
    else:
        print(f"\n❌ No data received!")
        print(f"\n💡 Possible causes:")
        print(f"   • ESP32 not running streaming firmware")
        print(f"   • Wrong baud rate (try 9600)")
        print(f"   • Device not paired/connected properly")
        print(f"   • Bluetooth module not responding")

    # Send stop
    print(f"\n📤 Sending STOP command...")
    ser.write(b'{"cd":12,"mt":"x"}\0')
    ser.flush()
    time.sleep(0.5)

    # Close
    ser.close()
    print(f"\n{'='*60}")
    print("  DIAGNOSTICS COMPLETE")
    print(f"{'='*60}\n")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python diagnose_bluetooth.py <PORT>")
        print("Example: python diagnose_bluetooth.py COM7")
        sys.exit(1)

    port = sys.argv[1]
    diagnose_connection(port)
