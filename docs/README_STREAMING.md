# sEMG Streaming System - Quick Reference

**Version:** 1.0 | **Last Updated:** 2025-10-14

---

## Overview

The sEMG streaming system enables **real-time biosignal transmission** from the ESP32 NeuroEstimulator device to mobile applications via Bluetooth. It implements a **dual-protocol architecture** with both JSON (legacy) and Binary (optimized) formats.

### Key Features

✅ **Configurable sampling rates:** 10-200 Hz
✅ **Three data modes:** Raw, Filtered (Butterworth 10-50 Hz), RMS
✅ **Binary protocol:** 72% bandwidth reduction vs JSON
✅ **Thread-safe:** FreeRTOS critical sections + semaphores
✅ **Automatic safety:** 10-minute timeout, disconnect detection, buffer overflow protection

---

## Quick Start

### 1. Configure Streaming (Optional)

Default: 20 Hz, raw data

```json
{"cd":14,"mt":"w","bd":{"rate":100,"type":"filtered"}}
```

**Parameters:**
- `rate`: 10-200 Hz (recommended: 20-50 Hz @ 9600 baud)
- `type`: `"raw"`, `"filtered"`, or `"rms"`

**Response:**
```json
{"cd":14,"mt":"a"}
```

### 2. Start Streaming

```json
{"cd":11,"mt":"x"}
```

**Response:**
```json
{"cd":11,"mt":"a"}
```

**Data packets (binary format, 108 bytes each):**
```
[0xAA][13][timestamp:4B][count:2B][data:100B]
```

Frequency: `rate / 50` packets/second
- 50 Hz → 1 packet/sec
- 100 Hz → 2 packets/sec
- 250 Hz → 5 packets/sec

### 3. Stop Streaming

```json
{"cd":12,"mt":"x"}
```

**Response:**
```json
{"cd":12,"mt":"a"}
```

---

## Architecture Summary

```
┌─────────────────────────────────────────────────────────┐
│                   ESP32 Dual-Core                        │
├──────────────────────────┬──────────────────────────────┤
│ CORE 0 (Time-Critical)   │ CORE 1 (Communication)       │
├──────────────────────────┼──────────────────────────────┤
│ Timer ISR (every 1/rate) │ MessageHandler Task          │
│ - Read ADC (5 ms)        │ - Bluetooth RX/TX            │
│ - Apply filter (1 ms)    │ - JSON parsing               │
│ - Write to buffer        │                              │
│                          │ Streaming Task               │
│                          │ - Read buffer (50 samples)   │
│                          │ - Encode binary packet       │
│                          │ - Bluetooth TX (11 ms)       │
└──────────────────────────┴──────────────────────────────┘
```

**Data Flow:**
1. **Timer ISR** (Core 0) samples ADC → writes to circular buffer
2. **Streaming Task** (Core 1) reads buffer → sends binary packets via Bluetooth

**Thread Safety:**
- Circular buffer protected by FreeRTOS critical sections
- Bluetooth serial protected by semaphore (100 ms timeout)

---

## Performance Constraints

| Metric | Value | Impact |
|--------|-------|--------|
| ADC read time | ~5 ms | Limits max rate to 140 Hz (200 Hz theoretical) |
| Bluetooth baud | 9600 | 960 bytes/s effective throughput |
| Binary packet size | 108 bytes | 11 ms transmission time |
| Filter overhead | 1-2 ms | Reduces max rate to 100 Hz with filtering |

**Bandwidth Usage @ Different Rates:**

| Rate | Packets/Sec | Bandwidth | % of 9600 Baud | Status |
|------|-------------|-----------|----------------|--------|
| 20 Hz | 0.4 | 43 bytes/s | 4.5% | ✅ Safe |
| 50 Hz | 1 | 108 bytes/s | 11.3% | ✅ Safe |
| 100 Hz | 2 | 216 bytes/s | 22.5% | ✅ Safe |
| 250 Hz | 5 | 540 bytes/s | 56.3% | ✅ Optimal |
| 500 Hz | 10 | 1080 bytes/s | 112.5% | ❌ Overflow |

**Recommended Operating Point:**
- **Rate:** 50 Hz (balanced performance)
- **Type:** Filtered (removes noise)
- **Bandwidth:** 11.3% (robust margin)

---

## Binary Protocol Specification

### Packet Structure (108 bytes total)

```
┌────────────────────────────────────────────┐
│ HEADER (8 bytes)                           │
├────────────────────────────────────────────┤
│ Byte 0:       Magic (0xAA)                 │
│ Byte 1:       Message Code (13)            │
│ Bytes 2-5:    Timestamp (uint32_t, ms)     │
│ Bytes 6-7:    Sample Count (uint16_t)      │
├────────────────────────────────────────────┤
│ DATA (100 bytes)                           │
├────────────────────────────────────────────┤
│ Bytes 8-9:    Sample[0] (int16_t)          │
│ Bytes 10-11:  Sample[1] (int16_t)          │
│ ...                                        │
│ Bytes 106-107: Sample[49] (int16_t)        │
└────────────────────────────────────────────┘
```

### Data Encoding

**int16_t to Voltage Conversion:**
- ADC range: ±4096 maps to ±4.096V
- 1 LSB = 1 mV precision
- Example: `2500` → `2.500V`

**Decoding (Mobile App):**
```javascript
function decodeInt16ToVoltage(int16_value) {
    return int16_value / 1000.0;  // mV to V
}
```

---

## Safety Mechanisms

### 1. Automatic Timeout
- Streaming stops after **10 minutes**
- Prevents battery drain if app crashes

### 2. Bluetooth Disconnect Detection
- Monitors hardware status pin (HC-05)
- Immediate shutdown on disconnect

### 3. Buffer Overflow Protection
- **200-sample circular buffer** (2 seconds @ 100 Hz)
- **FIFO policy:** Drops oldest samples when full
- **Rate-limited logging:** 1 warning/second

### 4. Mutex Timeout
- Bluetooth send has **100 ms timeout**
- Prevents deadlock if MessageHandler holds semaphore

---

## Known Issues

### Issue 1: Maximum Safe Rate = 100 Hz

**Cause:** ADC I2C read takes ~5 ms

**Workaround:**
- Use `"raw"` mode (skips filter) → enables 140 Hz
- Upgrade to ADS1256 (30 kSPS ADC) → enables 500+ Hz

### Issue 2: Cannot Run Streaming + FES Session Simultaneously

**Cause:** Both modes share the same sampling timer

**Mitigation:** App should enforce mutual exclusion

### Issue 3: Filter Oscillation on Configuration Change

**Status:** ✅ **FIXED** in current version

**Fix Applied:**
```cpp
SemgFilter::updateSamplingRate(sampling_time_ms, 10, 50, false);
SemgFilter::resetState();  // Critical: resets IIR filter state
```

### Issue 4: Bluetooth Limited to 9600 Baud

**Current:** 9600 baud (max 250 Hz streaming)

**Upgrade Path:**
1. Uncomment lines in `Bluetooth.cpp:21-24`:
   ```cpp
   BTSerial.write("AT+BAUD4");  // Set to 115200 baud
   BTSerial.end(true);
   BTSerial.begin(115200);
   ```
2. Update mobile app to 115200 baud
3. Rebuild and reflash firmware

**New Capability:** 1000+ Hz streaming (115200 baud = 11,520 bytes/s)

---

## Troubleshooting

### Problem: No data received after start command

**Checks:**
1. Verify ACK received: `{"cd":11,"mt":"a"}`
2. Check serial monitor for "Streaming task started"
3. Verify Bluetooth connection (LED should be solid)
4. Ensure mobile app is listening for binary packets (not JSON)

### Problem: Buffer overflow warnings

**Cause:** Bluetooth transmission slower than sampling rate

**Fix:**
1. Lower sampling rate (e.g., 100 Hz → 50 Hz)
2. Upgrade Bluetooth to 115200 baud
3. Check for MessageHandler blocking Bluetooth semaphore

### Problem: Filtered data looks wrong

**Checks:**
1. Verify `SemgFilter::resetState()` is called in `configureStreaming()`
2. Check sampling rate matches filter configuration
3. Wait 2 seconds for filter transient to settle

### Problem: Streaming stops unexpectedly

**Possible Causes:**
1. **10-minute timeout** (expected behavior)
2. **Bluetooth disconnect** (check connection)
3. **Task creation failure** (check serial logs for error)

---

## Code Locations

### Key Files

| File | Description |
|------|-------------|
| `src/modules/semg/Semg.cpp` | Main streaming implementation |
| `src/modules/semg/Semg.h` | API and configuration structs |
| `src/modules/semg/StreamingProtocol.h` | Binary protocol definition |
| `src/modules/bluetooth/Bluetooth.cpp` | UART communication |
| `src/modules/message_handler/MessageHandler.cpp` | Command routing |

### Configuration Parameters (`platformio.ini`)

```ini
-D DEFAULT_STREAMING_RATE=20          ; Default: 20 Hz
-D MAX_SAMPLES_PER_PACKET=50          ; Fixed at 50
-D STREAMING_BUFFER_SIZE=200          ; 200 samples
-D STREAMING_TIMEOUT_MINUTES=10       ; 10-minute auto-stop
```

---

## Advanced Topics

### Upgrading to High-Speed Mode (115200 Baud)

**Steps:**
1. Enable AT command in `Bluetooth::init()`:
   ```cpp
   BTSerial.write("AT+BAUD4");  // Uncomment line 21
   BTSerial.end(true);          // Uncomment line 23
   BTSerial.begin(115200);      // Uncomment line 24
   ```

2. Update mobile app Bluetooth connection:
   ```java
   // Android example
   bluetoothSocket = device.createRfcommSocketToServiceRecord(uuid);
   bluetoothSocket.connect();
   // Note: Baud rate is negotiated via AT command, not in app
   ```

3. Rebuild firmware:
   ```bash
   pio run --target upload
   ```

**New Bandwidth:** 11,520 bytes/sec (12x increase)

### Adding CRC for Error Detection

**Modify `StreamingProtocol.h`:**
```cpp
struct BinaryPacketHeader {
    uint8_t  magic;
    uint8_t  message_code;
    uint32_t timestamp;
    uint16_t sample_count;
    uint16_t crc16;  // NEW: CRC-16-CCITT
} __attribute__((packed));
```

**Compute CRC in `sendBinaryStreamingMessage()`:**
```cpp
uint16_t crc = crc16_ccitt(buffer + sizeof(BinaryPacketHeader), count * sizeof(int16_t));
header->crc16 = crc;
```

### Implementing Adaptive Packet Size

**Goal:** Reduce latency for low-rate streaming

**Logic:**
```cpp
if (streaming_config.rate <= 50) {
    samples_per_packet = 25;  // Send packets more frequently
} else {
    samples_per_packet = 50;  // Default
}
```

**Benefit:** 20 Hz streaming sends packets every 1.25s (instead of 2.5s)

---

## Testing Checklist

### Basic Functionality

- [ ] Configure streaming @ 50 Hz, raw mode
- [ ] Start streaming, verify ACK received
- [ ] Receive 50 binary packets (50 seconds)
- [ ] Stop streaming, verify graceful shutdown

### Filtered Mode

- [ ] Configure @ 50 Hz, filtered mode
- [ ] Verify no oscillation in received data
- [ ] Check filter settling time (<2 seconds)

### Error Handling

- [ ] Disconnect Bluetooth during streaming → verify auto-stop
- [ ] Let streaming run for 10 minutes → verify timeout
- [ ] Start streaming at 200 Hz → check for buffer overflow warnings

### Concurrency

- [ ] Start streaming @ 20 Hz (low rate)
- [ ] Send gyroscope command while streaming
- [ ] Verify gyroscope response received within 500 ms

---

## Further Documentation

- **Detailed Analysis:** `SEMG_STREAMING_ANALYSIS.md` (comprehensive 40-page technical document)
- **Visual Flowchart:** `SEMG_STREAMING_FLOWCHART.md` (ASCII diagrams of data flow)
- **Protocol Spec:** `src/modules/semg/StreamingProtocol.h` (binary format definition)

---

## Support

**Issues:**
- Check serial monitor logs (115200 baud)
- Enable verbose logging: `esp_log_level_set("*", ESP_LOG_DEBUG)`
- Review `SEMG_STREAMING_ANALYSIS.md` Appendix D for troubleshooting

**Performance Optimization:**
- See "Recommendations for Future Development" in analysis document
- Consider hardware upgrades (ADS1256, BLE module)

---

**Document Version:** 1.0
**Status:** Production-ready
**Tested on:** ESP32 DevKit V1, HC-05 Bluetooth @ 9600 baud
