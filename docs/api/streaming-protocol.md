# Binary Protocol - Quick Reference

**TL;DR**: 215 Hz streaming @ 48% bandwidth (fixed, hardware-optimized)

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

### Start Streaming (Fixed 215 Hz)
```json
{"cd":11,"mt":"x"}
```

**Note**: Configuration message (code 14) is no longer supported in v3.0+. The system automatically uses the optimal 215 Hz fixed configuration.

### Stop Streaming
```json
{"cd":12,"mt":"x"}
```

---

## Expected Performance

| Metric | Value |
|--------|-------|
| Sampling rate | 215 Hz (fixed) |
| Packet rate | ~4.3 packets/s |
| Packet size | 108 bytes |
| Bandwidth | 464 bytes/s (48% @ 9600 baud) |
| Latency | ~230ms |

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
- Monitor timestamp gaps (expect ~232ms)
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

## Files Changed (v3.0)

- `platformio.ini` - Fixed 215 Hz constants
- `Adc.cpp/h` - Continuous mode with circular buffer
- `StreamingProtocol.h` - Binary packet struct
- `Semg.cpp` - Simplified streaming (removed configuration)
- `Bluetooth.cpp` - sendRawData() method

---

For complete documentation, see [`bluetooth-protocol.md`](./bluetooth-protocol.md)
