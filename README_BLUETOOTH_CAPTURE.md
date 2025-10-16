# Bluetooth sEMG Streaming - Data Capture Guide

## Overview

This guide explains how to capture and visualize real-time sEMG data from the ESP32 device via Bluetooth using the **binary streaming protocol** (215 Hz sampling rate).

## Prerequisites

### Hardware
- ESP32 NeuroEstimulator device
- Bluetooth HC-05/HC-06 module connected
- AD8232 sEMG sensor with electrodes

### Software
1. **Python 3.7+** installed
2. **Required packages:**
   ```bash
   pip install -r requirements.txt
   ```

## Quick Start

### 1. Upload Firmware to ESP32
```bash
pio run --target upload
```

### 2. Pair Bluetooth Device

**Windows:**
1. Open Settings → Bluetooth & devices
2. Pair with "NeuroEstimulator" (PIN: 1234 or 0000)
3. Note the COM port (e.g., `COM6`)

**Linux:**
```bash
sudo rfcomm bind /dev/rfcomm0 <MAC_ADDRESS> 1
```

### 3. Run Capture Script

**Windows:**
```bash
python capture_bluetooth_stream.py COM6
```

**Linux/Mac:**
```bash
python capture_bluetooth_stream.py /dev/rfcomm0
```

## Script Features

### What It Does

1. **Connects** to ESP32 via Bluetooth serial
2. **Sends** JSON command to start streaming: `{"cd":11,"mt":"x"}`
3. **Receives** binary packets (215 Hz) for 10 seconds
4. **Parses** binary protocol:
   - Header: magic byte (0xAA) + code + timestamp + sample count
   - Payload: int16_t array (50 samples/packet)
5. **Analyzes** signal:
   - Time-domain statistics (mean, std, RMS)
   - Frequency-domain analysis (FFT)
6. **Generates** plots:
   - Raw ADC values
   - Voltage (mV)
   - Frequency spectrum (0-100 Hz)
7. **Saves** data to CSV and PNG

### Output Files

After capture, you'll get:

- `semg_stream_YYYYMMDD_HHMMSS.csv` - Raw data (time, ADC value, voltage)
- `semg_analysis_YYYYMMDD_HHMMSS.png` - Visualization (3 plots)

### Example Output

```
============================================================
  sEMG STREAMING CAPTURE - 10s @ 215 Hz
============================================================

[SERIAL] Connected to COM6 @ 115200 baud
[CMD] Sent START_STREAM command
[CAPTURE] Recording for 10 seconds...
[CAPTURE] Expected samples: 2150

[PROGRESS] 10% - Samples: 215, Packets: 5
[PROGRESS] 20% - Samples: 430, Packets: 9
...
[PROGRESS] 100% - Samples: 2150, Packets: 43

[CMD] Sent STOP_STREAM command

============================================================
  CAPTURE STATISTICS
============================================================
  Duration:        10.05 seconds
  Samples:         2150 / 2150 expected
  Packets:         43
  Actual rate:     213.9 Hz
  Packet errors:   0
  Completeness:    100.0%
============================================================

============================================================
  SIGNAL ANALYSIS
============================================================
  Raw ADC range:   1200 to 1800
  Voltage range:   0.2250 to 0.3375 V
  Mean voltage:    0.2813 V
  Std deviation:   0.0234 V
  RMS voltage:     0.2823 V
============================================================

  Dominant frequency: 23.45 Hz
  FFT peak magnitude: 145.23

[DATA] Saved to: semg_stream_20251015_143022.csv
[PLOT] Saved to: semg_analysis_20251015_143022.png
```

## Customization

### Change Capture Duration

Edit line 27 in `capture_bluetooth_stream.py`:
```python
CAPTURE_DURATION = 30  # Capture 30 seconds instead of 10
```

### Change Baud Rate

Edit line 30:
```python
BAUD_RATE = 9600  # Use 9600 if HC-05 not upgraded
```

### Change COM Port

```bash
python capture_bluetooth_stream.py COM7  # Use different port
```

## Troubleshooting

### No Data Received

**Check:**
1. Bluetooth paired and connected
2. Correct COM port
3. Firmware uploaded and running
4. Streaming enabled (send command manually via Serial Bluetooth Terminal app)

### Low Sample Rate (<200 Hz)

**Possible causes:**
- Bluetooth baud rate too low (upgrade to 115200)
- Buffer overflow on ESP32 (check serial logs)
- Interference or packet loss

### Parser Errors

**Symptoms:** "Skipped N bytes to find magic byte"

**Solutions:**
- Flush serial buffer before capture
- Increase serial timeout
- Check for corrupted packets (verify ESP32 logs)

## Protocol Reference

### Binary Packet Structure

```
Header (8 bytes):
  [0]     magic       0xAA (170)
  [1]     code        13 (STREAM_DATA)
  [2-5]   timestamp   uint32_t (milliseconds since boot)
  [6-7]   count       uint16_t (number of samples)

Payload (count × 2 bytes):
  [8-9]   sample[0]   int16_t (ADC value)
  [10-11] sample[1]   int16_t
  ...
```

### Start/Stop Commands (JSON)

```json
// Start streaming
{"cd":11,"mt":"x"}

// Stop streaming
{"cd":12,"mt":"x"}

// Configure streaming (optional, before start)
{"cd":14,"mt":"w","bd":{"rate":215,"type":"filtered"}}
```

## Signal Processing Chain

```
AD8232 Sensor
  ↓
ADS1115 ADC (16-bit, 860 Hz)
  ↓
Averaging (4 samples) → 215 Hz
  ↓
Butterworth Bandpass (10-50 Hz, 3rd order)
  ↓
Notch Filter (60 Hz, 2 Hz bandwidth)
  ↓
Bluetooth Binary Packet (50 samples/packet)
  ↓
Python Script (parsing + visualization)
```

## Data Analysis Tips

### Time-Domain Analysis

- **RMS (Root Mean Square):** Overall signal strength
- **Standard Deviation:** Signal variability
- **Peak-to-Peak:** Dynamic range

### Frequency-Domain Analysis

- **10-50 Hz range:** Valid sEMG band (bandpass filter)
- **60 Hz notch:** Should show suppression of power line interference
- **Dominant frequency:** Main muscle activation frequency

### Signal Quality Indicators

✅ **Good Signal:**
- RMS: 0.1-1.0 mV
- Dominant frequency: 20-40 Hz
- No 60 Hz peak (notch filter working)

❌ **Poor Signal:**
- RMS < 0.01 mV (no muscle activity or disconnected)
- Dominant frequency < 10 Hz (motion artifact)
- Strong 60 Hz peak (notch filter not working)

## Advanced Usage

### Batch Processing

Capture multiple sessions:
```bash
for i in {1..5}; do
    python capture_bluetooth_stream.py COM6
    sleep 5
done
```

### Real-Time Monitoring

Use `capture_semg_data.py` (old script) for real-time serial monitor capture without Bluetooth protocol parsing.

### Custom Analysis

Load saved CSV in your own script:
```python
import pandas as pd

data = pd.read_csv("semg_stream_20251015_143022.csv")
time = data["Time(s)"]
voltage = data["Voltage(V)"]

# Your analysis here...
```

## References

- **Firmware Documentation:** `docs/api/bluetooth-protocol.md`
- **Streaming Architecture:** `docs/architecture/streaming-architecture.md`
- **Old Capture Script:** `capture_semg_data.py` (serial monitor method)
- **Protocol Specification:** `docs/api/streaming-protocol.md`

---

**Version:** 1.0.0
**Last Updated:** 2025-01-15
**Firmware Compatibility:** v1.0.0+
