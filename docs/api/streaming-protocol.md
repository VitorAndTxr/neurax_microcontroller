# Binary Protocol - Quick Reference

**TL;DR**: 250 Hz streaming @ 55% bandwidth (was 100 Hz @ 211% with JSON)

---

## Packet Format (108 bytes)

```c
struct BinaryPacket {
    uint8_t  magic;         // 0xAA
    uint8_t  code;          // 0x0D (13)
    uint32_t timestamp;     // millis()
    uint16_t count;         // 50
    int16_t  samples[50];   // -4096 to +4096 (mV precision)
} __attribute__((packed));
```

---

## Python Decoder (Minimal)

```python
import struct

def decode_packet(data: bytes) -> dict:
    if data[0] != 0xAA or data[1] != 0x0D:
        raise ValueError("Invalid packet")

    magic, code, ts, count = struct.unpack('<BBIH', data[:8])
    samples = struct.unpack(f'<{count}h', data[8:8+count*2])
    volts = [s / 1000.0 for s in samples]

    return {'timestamp': ts, 'values': volts}
```

---

## JavaScript Decoder (Minimal)

```javascript
class Decoder {
  decode(buf) {
    if (buf[0] !== 0xAA || buf[1] !== 0x0D) return null;

    const view = new DataView(buf.buffer);
    const ts = view.getUint32(2, true);  // little-endian
    const count = view.getUint16(6, true);
    const values = [];

    for (let i = 0; i < count; i++) {
      values.push(view.getInt16(8 + i * 2, true) / 1000.0);
    }

    return { timestamp: ts, values };
  }
}
```

---

## Configuration Commands

### Start Streaming @ 250 Hz
```json
{"cd":14,"mt":"w","bd":{"rate":250,"type":"raw"}}
{"cd":11,"mt":"x"}
```

### Stop Streaming
```json
{"cd":12,"mt":"x"}
```

---

## Expected Performance

| Metric | Value |
|--------|-------|
| Packet rate | 5 packets/s |
| Packet size | 108 bytes |
| Bandwidth | 540 bytes/s (56% @ 9600 baud) |
| Latency | ~200ms |

---

## Common Issues

### Desync?
- Check for magic byte `0xAA`
- Verify little-endian byte order
- Ensure buffer size ≥ 108 bytes

### Wrong values?
- Divide int16 by 1000.0 to get volts
- Expect baseline ~2.49V (AD8232 VCC/2)
- Filtered values can be negative

### Packet loss?
- Monitor timestamp gaps (expect ~200ms)
- Check Bluetooth signal strength
- Don't block receiver thread

---

## Test Vector

**Hex**: `AA 0D 10 27 00 00 32 00 B2 09 B3 09 ...`

**Decoded**:
- Magic: `0xAA` ✓
- Code: `0x0D` ✓
- Timestamp: `10000 ms`
- Count: `50`
- Sample[0]: `2482` → `2.482 V`

---

## Files Changed

- `platformio.ini` - Constants (250 Hz, 50 samples, 300 buffer)
- `StreamingProtocol.h` - Binary packet struct
- `Semg.cpp` - Encoder + buffer (int16_t)
- `Bluetooth.cpp` - sendRawData() method

---

For complete documentation, see [`BINARY_STREAMING_PROTOCOL.md`](./BINARY_STREAMING_PROTOCOL.md)
