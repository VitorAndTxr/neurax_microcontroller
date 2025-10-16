# Binary Streaming Protocol - Implementation Guide

**Version**: 1.1
**Date**: 2025-10-16
**Target Rate**: 215 Hz (fixed, hardware-optimized)
**Packet Size**: 108 bytes
**Bandwidth**: 464 bytes/s @ 9600 baud (48% utilization)

---

## Table of Contents

1. [Overview](#overview)
2. [Protocol Specification](#protocol-specification)
3. [Packet Structure](#packet-structure)
4. [Decoding Implementation](#decoding-implementation)
5. [Example Code](#example-code)
6. [Migration from JSON](#migration-from-json)
7. [Testing & Validation](#testing--validation)

---

## Overview

The **Binary Streaming Protocol** replaces the previous JSON-based streaming to achieve higher sampling rates with lower bandwidth consumption. This protocol is optimized for real-time sEMG data transmission over Bluetooth SPP (Serial Port Profile).

### Key Benefits

| Metric | JSON Protocol | Binary Protocol | Improvement |
|--------|---------------|-----------------|-------------|
| Packet Size | 282 bytes | **108 bytes** | **-62%** |
| Bandwidth @ 215 Hz | ~1800 bytes/s (187%) | **464 bytes/s (48%)** | **-74%** |
| Max Rate @ 9600 baud | ~100 Hz | **215 Hz** | **+115%** |
| Latency | 100ms | 230ms | Acceptable for data logging |

### Use Cases

- Real-time sEMG signal visualization
- Data logging for offline analysis
- Trigger detection monitoring
- Signal quality assessment

---

## Protocol Specification

### Communication Flow

```
Mobile App → ESP32: {"cd":11,"mt":"x"}                                   (Start)
ESP32 → App:        {"cd":11,"mt":"a"}                                   (ACK)

ESP32 → App:        [Binary Packet 1] (108 bytes)
ESP32 → App:        [Binary Packet 2] (108 bytes)
ESP32 → App:        [Binary Packet 3] (108 bytes)
...                 (~4.3 packets/second @ 215 Hz)

Mobile App → ESP32: {"cd":12,"mt":"x"}                                   (Stop)
ESP32 → App:        {"cd":12,"mt":"a"}                                   (ACK)
```

### Fixed Configuration

**Firmware v3.0+** uses a **fixed 215 Hz configuration** for optimal performance:

- **Sampling Rate**: 215 Hz (860 Hz ADC ÷ 4 downsample)
- **Data Type**: Filtered (Butterworth 10-50 Hz bandpass + 60 Hz notch)
- **Samples per Packet**: 50
- **Bandwidth**: 464 bytes/s (48% of 9600 baud)

**Configuration message (code 14) is no longer supported** - the system automatically uses optimal settings.

**Note**: For firmware v2.x (configurable streaming), see [CHANGELOG.md](../../CHANGELOG.md) for migration guide

---

## Packet Structure

### Binary Packet Layout

```
┌──────────────────────────────────────────────────────────────┐
│ HEADER (8 bytes)                                             │
├──────────────────────────────────────────────────────────────┤
│ Offset │ Field         │ Type    │ Value │ Description       │
├────────┼───────────────┼─────────┼───────┼───────────────────┤
│ 0      │ magic         │ uint8_t │ 0xAA  │ Packet start mark │
│ 1      │ message_code  │ uint8_t │ 0x0D  │ Stream data (13)  │
│ 2-5    │ timestamp     │ uint32_t│ (var) │ millis() at send  │
│ 6-7    │ sample_count  │ uint16_t│ 50    │ Number of samples │
├──────────────────────────────────────────────────────────────┤
│ DATA PAYLOAD (100 bytes)                                     │
├──────────────────────────────────────────────────────────────┤
│ 8-9    │ samples[0]    │ int16_t │ (var) │ Sample value      │
│ 10-11  │ samples[1]    │ int16_t │ (var) │ Sample value      │
│ ...    │ ...           │ ...     │ ...   │ ...               │
│ 106-107│ samples[49]   │ int16_t │ (var) │ Sample value      │
└──────────────────────────────────────────────────────────────┘

Total Size: 108 bytes
```

### Data Type Specifications

- **Endianness**: Little-endian (LSB first)
- **Alignment**: Packed structure (no padding)
- **Integer Format**: Two's complement signed 16-bit

### Value Range

```
int16_t range:  -32768 to +32767
sEMG range:     -4096 to +4096
Precision:      1 LSB = 1 millivolt
Conversion:     volts = int16_value / 1000.0
```

**Example Values**:
- `0` → 0.000 V (baseline)
- `2490` → 2.490 V (typical AD8232 baseline = VCC/2)
- `4096` → 4.096 V (max ADC range)
- `-1500` → -1.500 V (filtered signal can be negative)

---

## Decoding Implementation

### Step-by-Step Decoder

#### 1. Packet Synchronization

**Problem**: Bluetooth stream is continuous bytes - how to find packet boundaries?

**Solution**: Search for magic byte `0xAA` followed by message code `0x0D`.

```python
def find_packet_start(stream: bytes, offset: int = 0) -> int:
    """Find next packet start in byte stream."""
    while offset < len(stream) - 1:
        if stream[offset] == 0xAA and stream[offset + 1] == 0x0D:
            return offset
        offset += 1
    return -1  # Not found
```

**Important**: Always validate magic byte to avoid desynchronization!

#### 2. Header Parsing

```python
import struct

def parse_header(packet: bytes) -> dict:
    """Parse 8-byte binary packet header."""
    if len(packet) < 8:
        raise ValueError("Incomplete header")

    # Unpack header: '<' = little-endian
    # 'B' = uint8, 'I' = uint32, 'H' = uint16
    magic, code, timestamp, count = struct.unpack('<BBIH', packet[:8])

    # Validate
    if magic != 0xAA:
        raise ValueError(f"Invalid magic byte: 0x{magic:02X}")
    if code != 0x0D:
        raise ValueError(f"Invalid message code: 0x{code:02X}")

    return {
        'magic': magic,
        'code': code,
        'timestamp': timestamp,
        'sample_count': count
    }
```

#### 3. Data Payload Extraction

```python
def parse_samples(packet: bytes, offset: int = 8, count: int = 50) -> list[int]:
    """Parse int16_t array from packet data payload."""
    data_size = count * 2  # 2 bytes per int16

    if len(packet) < offset + data_size:
        raise ValueError("Incomplete data payload")

    # Unpack array of signed 16-bit integers
    # '<' = little-endian, 'h' = signed short (int16)
    format_str = f'<{count}h'
    samples = struct.unpack(format_str, packet[offset:offset + data_size])

    return list(samples)
```

#### 4. Value Conversion

```python
def int16_to_volts(sample: int) -> float:
    """Convert int16 sample to voltage in volts."""
    return sample / 1000.0

def process_packet(packet: bytes) -> dict:
    """Complete packet processing pipeline."""
    header = parse_header(packet)
    samples_int16 = parse_samples(packet, count=header['sample_count'])
    samples_volts = [int16_to_volts(s) for s in samples_int16]

    return {
        'timestamp_ms': header['timestamp'],
        'sample_count': header['sample_count'],
        'values_int16': samples_int16,
        'values_volts': samples_volts
    }
```

---

## Example Code

### Python Implementation (Complete)

```python
import struct
from typing import Optional, Generator
from dataclasses import dataclass

@dataclass
class StreamingPacket:
    """Represents a decoded sEMG streaming packet."""
    timestamp_ms: int
    sample_count: int
    values_volts: list[float]
    values_raw: list[int]

class BinaryStreamDecoder:
    """Decoder for ESP32 binary streaming protocol."""

    MAGIC_BYTE = 0xAA
    MESSAGE_CODE = 0x0D
    HEADER_SIZE = 8
    PACKET_SIZE = 108

    def __init__(self):
        self.buffer = bytearray()

    def feed(self, data: bytes) -> Generator[StreamingPacket, None, None]:
        """
        Feed incoming bytes and yield complete packets.

        Usage:
            decoder = BinaryStreamDecoder()
            for chunk in bluetooth_stream:
                for packet in decoder.feed(chunk):
                    print(f"Received {packet.sample_count} samples")
        """
        self.buffer.extend(data)

        while len(self.buffer) >= self.PACKET_SIZE:
            packet = self._try_decode_packet()
            if packet:
                yield packet
            else:
                # Synchronization lost - search for next magic byte
                self._resync()

    def _try_decode_packet(self) -> Optional[StreamingPacket]:
        """Try to decode packet from buffer start."""
        if len(self.buffer) < self.PACKET_SIZE:
            return None

        # Validate magic byte
        if self.buffer[0] != self.MAGIC_BYTE:
            return None

        # Validate message code
        if self.buffer[1] != self.MESSAGE_CODE:
            return None

        # Parse header
        magic, code, timestamp, count = struct.unpack(
            '<BBIH',
            self.buffer[:self.HEADER_SIZE]
        )

        # Parse samples
        sample_format = f'<{count}h'
        samples_raw = struct.unpack(
            sample_format,
            self.buffer[self.HEADER_SIZE:self.HEADER_SIZE + count * 2]
        )

        # Convert to volts
        samples_volts = [s / 1000.0 for s in samples_raw]

        # Remove packet from buffer
        packet_size = self.HEADER_SIZE + count * 2
        del self.buffer[:packet_size]

        return StreamingPacket(
            timestamp_ms=timestamp,
            sample_count=count,
            values_volts=samples_volts,
            values_raw=list(samples_raw)
        )

    def _resync(self):
        """Search for next magic byte and align buffer."""
        for i in range(1, len(self.buffer)):
            if self.buffer[i] == self.MAGIC_BYTE:
                del self.buffer[:i]
                return
        # No magic byte found - clear buffer
        self.buffer.clear()

# Example usage
if __name__ == "__main__":
    decoder = BinaryStreamDecoder()

    # Simulate receiving data chunks over Bluetooth
    bluetooth_data = b'\xAA\x0D...'  # Replace with actual data

    for packet in decoder.feed(bluetooth_data):
        print(f"Timestamp: {packet.timestamp_ms} ms")
        print(f"Samples: {packet.sample_count}")
        print(f"First value: {packet.values_volts[0]:.3f} V")
        print(f"Average: {sum(packet.values_volts) / len(packet.values_volts):.3f} V")
```

### JavaScript/TypeScript Implementation

```typescript
interface StreamingPacket {
  timestampMs: number;
  sampleCount: number;
  valuesVolts: number[];
  valuesRaw: Int16Array;
}

class BinaryStreamDecoder {
  private buffer: number[] = [];
  private readonly MAGIC_BYTE = 0xAA;
  private readonly MESSAGE_CODE = 0x0D;
  private readonly HEADER_SIZE = 8;

  feed(data: Uint8Array): StreamingPacket[] {
    const packets: StreamingPacket[] = [];

    // Add new data to buffer
    this.buffer.push(...Array.from(data));

    // Try to decode packets
    while (this.buffer.length >= 108) {
      const packet = this.tryDecodePacket();
      if (packet) {
        packets.push(packet);
      } else {
        this.resync();
      }
    }

    return packets;
  }

  private tryDecodePacket(): StreamingPacket | null {
    if (this.buffer.length < 108) return null;

    // Validate magic byte and message code
    if (this.buffer[0] !== this.MAGIC_BYTE ||
        this.buffer[1] !== this.MESSAGE_CODE) {
      return null;
    }

    // Parse header (little-endian)
    const timestamp = this.readUInt32LE(2);
    const count = this.readUInt16LE(6);

    // Parse samples
    const samplesRaw = new Int16Array(count);
    for (let i = 0; i < count; i++) {
      samplesRaw[i] = this.readInt16LE(8 + i * 2);
    }

    // Convert to volts
    const samplesVolts = Array.from(samplesRaw).map(s => s / 1000.0);

    // Remove packet from buffer
    this.buffer.splice(0, 8 + count * 2);

    return {
      timestampMs: timestamp,
      sampleCount: count,
      valuesVolts: samplesVolts,
      valuesRaw: samplesRaw
    };
  }

  private resync(): void {
    const index = this.buffer.indexOf(this.MAGIC_BYTE, 1);
    if (index > 0) {
      this.buffer.splice(0, index);
    } else {
      this.buffer = [];
    }
  }

  private readUInt32LE(offset: number): number {
    return this.buffer[offset] |
           (this.buffer[offset + 1] << 8) |
           (this.buffer[offset + 2] << 16) |
           (this.buffer[offset + 3] << 24);
  }

  private readUInt16LE(offset: number): number {
    return this.buffer[offset] | (this.buffer[offset + 1] << 8);
  }

  private readInt16LE(offset: number): number {
    const val = this.readUInt16LE(offset);
    return (val & 0x8000) ? val - 0x10000 : val;
  }
}

// Example usage with React Native Bluetooth
import { BleManager } from 'react-native-ble-plx';

const decoder = new BinaryStreamDecoder();

// Subscribe to Bluetooth notifications
characteristic.monitor((error, char) => {
  if (error || !char?.value) return;

  const data = new Uint8Array(
    Buffer.from(char.value, 'base64')
  );

  const packets = decoder.feed(data);
  packets.forEach(packet => {
    console.log(`Received ${packet.sampleCount} samples`);
    updateChart(packet.valuesVolts);
  });
});
```

### C/C++ Implementation (Arduino/ESP32)

```cpp
#include <Arduino.h>

struct StreamingPacket {
    uint32_t timestamp_ms;
    uint16_t sample_count;
    int16_t values[50];
};

class BinaryStreamDecoder {
private:
    static const uint8_t MAGIC_BYTE = 0xAA;
    static const uint8_t MESSAGE_CODE = 0x0D;
    static const size_t HEADER_SIZE = 8;

    uint8_t buffer[256];
    size_t buffer_len = 0;

public:
    bool feed(uint8_t* data, size_t len, StreamingPacket& packet) {
        // Add to buffer
        if (buffer_len + len > sizeof(buffer)) {
            buffer_len = 0; // Overflow - reset
        }
        memcpy(buffer + buffer_len, data, len);
        buffer_len += len;

        // Try decode
        if (buffer_len >= 108) {
            if (tryDecodePacket(packet)) {
                // Remove packet from buffer
                memmove(buffer, buffer + 108, buffer_len - 108);
                buffer_len -= 108;
                return true;
            } else {
                resync();
            }
        }
        return false;
    }

private:
    bool tryDecodePacket(StreamingPacket& packet) {
        if (buffer[0] != MAGIC_BYTE || buffer[1] != MESSAGE_CODE) {
            return false;
        }

        // Parse header (little-endian)
        packet.timestamp_ms = *(uint32_t*)(buffer + 2);
        packet.sample_count = *(uint16_t*)(buffer + 6);

        // Parse samples
        memcpy(packet.values, buffer + HEADER_SIZE,
               packet.sample_count * sizeof(int16_t));

        return true;
    }

    void resync() {
        for (size_t i = 1; i < buffer_len; i++) {
            if (buffer[i] == MAGIC_BYTE) {
                memmove(buffer, buffer + i, buffer_len - i);
                buffer_len -= i;
                return;
            }
        }
        buffer_len = 0;
    }
};
```

---

## Migration from JSON

### Backward Compatibility

The ESP32 firmware still supports JSON protocol for **configuration messages** (codes 1-12, 14). Only **streaming data** (code 13) uses binary protocol.

### Dual Protocol Support (Recommended)

```typescript
class StreamDecoder {
  private binaryDecoder = new BinaryStreamDecoder();

  processData(data: Uint8Array): StreamingPacket[] {
    const packets: StreamingPacket[] = [];

    // Check if data starts with JSON '{'
    if (data[0] === 0x7B) {
      // JSON packet - handle configuration/control
      const json = JSON.parse(new TextDecoder().decode(data));
      this.handleControlMessage(json);
    }
    // Check if data starts with magic byte 0xAA
    else if (data[0] === 0xAA) {
      // Binary packet - handle streaming data
      packets.push(...this.binaryDecoder.feed(data));
    }

    return packets;
  }
}
```

### Migration Checklist

- [ ] Implement `BinaryStreamDecoder` class
- [ ] Add packet validation (magic byte + message code)
- [ ] Implement buffer synchronization
- [ ] Convert int16 → float voltage values
- [ ] Update data visualization components
- [ ] Add error handling for corrupt packets
- [ ] Test with real ESP32 device
- [ ] Implement packet loss detection (timestamp gaps)
- [ ] Add performance monitoring (packets/sec)

---

## Testing & Validation

### Test Vectors

#### Valid Packet Example

```
Hex dump:
AA 0D 10 27 00 00 32 00  B2 09 B3 09 B4 09 B5 09
B6 09 B7 09 B8 09 B9 09  BA 09 BB 09 BC 09 BD 09
BE 09 BF 09 C0 09 C1 09  C2 09 C3 09 C4 09 C5 09
C6 09 C7 09 C8 09 C9 09  CA 09 CB 09 CC 09 CD 09
CE 09 CF 09 D0 09 D1 09  D2 09 D3 09 D4 09 D5 09
D6 09 D7 09 D8 09 D9 09  DA 09 DB 09 DC 09 DD 09
DE 09 DF 09 E0 09 E1 09  E2 09

Decoded:
- Magic: 0xAA ✓
- Code: 0x0D (13) ✓
- Timestamp: 0x00002710 = 10000 ms
- Count: 0x0032 = 50 samples
- Sample[0]: 0x09B2 = 2482 → 2.482 V
- Sample[1]: 0x09B3 = 2483 → 2.483 V
- ...
- Sample[49]: 0x09E2 = 2530 → 2.530 V
```

#### Invalid Packet Examples

**Missing Magic Byte**:
```
FF 0D 10 27 00 00 32 00 ...
^^ Should be 0xAA
→ Should trigger resync
```

**Wrong Message Code**:
```
AA FF 10 27 00 00 32 00 ...
   ^^ Should be 0x0D
→ Should trigger resync
```

**Truncated Packet**:
```
AA 0D 10 27 00 00 32 00 B2 09 B3 09
(Only 12 bytes instead of 108)
→ Should wait for more data
```

### Validation Tests

```python
import pytest

def test_valid_packet():
    decoder = BinaryStreamDecoder()

    # Create test packet
    packet = bytearray([0xAA, 0x0D])  # Header
    packet.extend(struct.pack('<I', 10000))  # Timestamp
    packet.extend(struct.pack('<H', 50))  # Count
    packet.extend(struct.pack('<50h', *range(2482, 2532)))  # Samples

    packets = list(decoder.feed(bytes(packet)))

    assert len(packets) == 1
    assert packets[0].timestamp_ms == 10000
    assert packets[0].sample_count == 50
    assert packets[0].values_volts[0] == pytest.approx(2.482)

def test_resync_after_corruption():
    decoder = BinaryStreamDecoder()

    # Corrupted data + valid packet
    data = b'\xFF\xFF\xFF' + valid_packet_bytes

    packets = list(decoder.feed(data))

    assert len(packets) == 1  # Should recover

def test_partial_packet():
    decoder = BinaryStreamDecoder()

    # Send first half
    packets1 = list(decoder.feed(valid_packet_bytes[:54]))
    assert len(packets1) == 0  # No complete packet yet

    # Send second half
    packets2 = list(decoder.feed(valid_packet_bytes[54:]))
    assert len(packets2) == 1  # Now complete
```

### Performance Benchmarks

**Target Metrics @ 215 Hz**:
- Packet rate: ~4.3 packets/second
- Latency: <300ms (packet processing + transmission)
- Packet loss: <1%
- CPU usage: <10% (mobile device)

**Monitoring Code**:
```python
import time

class StreamMonitor:
    def __init__(self):
        self.packet_count = 0
        self.start_time = time.time()
        self.last_timestamp = 0

    def on_packet(self, packet: StreamingPacket):
        self.packet_count += 1

        # Check for packet loss (timestamp gaps)
        if self.last_timestamp > 0:
            expected_gap = 232  # 50 samples @ 215 Hz = 232ms
            actual_gap = packet.timestamp_ms - self.last_timestamp
            if abs(actual_gap - expected_gap) > 50:
                print(f"⚠️ Timestamp gap: {actual_gap}ms (expected {expected_gap}ms)")

        self.last_timestamp = packet.timestamp_ms

        # Report statistics every 10 seconds
        elapsed = time.time() - self.start_time
        if elapsed >= 10.0:
            rate = self.packet_count / elapsed
            print(f"📊 Packet rate: {rate:.1f} pkts/sec (target: 4.3)")
            self.packet_count = 0
            self.start_time = time.time()
```

---

## Troubleshooting

### Issue: No packets received

**Diagnosis**:
1. Verify Bluetooth connection is active
2. Check if streaming was started with message code 11
3. Enable debug logging on ESP32

**Solution**:
```python
# Add synchronization debugging
decoder.debug = True  # Print sync attempts
```

### Issue: Corrupt packets / desync

**Symptoms**: Invalid voltage values, constant resyncing

**Diagnosis**:
1. Check Bluetooth MTU size (should be ≥108 bytes)
2. Verify endianness (should be little-endian)
3. Check for buffer overflows in receiver

**Solution**:
```python
# Increase buffer size if using BLE notifications
decoder = BinaryStreamDecoder(buffer_size=1024)
```

### Issue: High packet loss

**Symptoms**: Large timestamp gaps, dropped packets

**Diagnosis**:
1. Monitor Bluetooth RSSI (signal strength)
2. Check for concurrent Bluetooth operations
3. Verify processing doesn't block receiver thread

**Solution**:
```python
# Process packets asynchronously
import asyncio

async def process_packet(packet):
    # Heavy processing here
    pass

packets = decoder.feed(data)
asyncio.create_task(process_packet(packets[0]))
```

---

## Reference

### ESP32 Firmware Files

- `src/modules/semg/StreamingProtocol.h` - Binary protocol definition
- `src/modules/semg/Semg.cpp` - Encoding implementation
- `src/modules/bluetooth/Bluetooth.cpp` - Raw data transmission

### Configuration Constants

```cpp
#define STREAMING_BUFFER_SIZE 512
#define MAX_SAMPLES_PER_PACKET 50
#define SEMG_FIXED_RATE_HZ 215
#define PACKET_MAGIC_BYTE 0xAA
#define PACKET_MESSAGE_CODE_STREAM_DATA 13
```

### Contact

For implementation questions or issues, refer to:
- Main documentation: `CLAUDE.md`
- ESP32 firmware: `InteroperableResearchsEMGDevice/`
- Mobile app: `neurax_react_native_app/`

---

**Document Version**: 1.1
**Last Updated**: 2025-10-16
**Author**: PRISM Development Team
**Firmware Compatibility**: v3.0+ (215 Hz fixed rate)
