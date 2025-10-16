# Bluetooth sEMG Testing Guide

Complete guide for testing the Bluetooth streaming protocol with automatic port detection.

---

## 📋 Overview

Scripts available for Bluetooth testing:

| Script | Purpose | Requirements |
|--------|---------|--------------|
| **`capture_bluetooth_simple.py`** | ✅ **Recommended** - Auto-detects COM port | pyserial only |
| **`test_bluetooth_packets.py`** | Detailed packet inspection | pyserial only |
| **`capture_bluetooth_stream.py`** | Original - Manual port | pyserial only |

---

## 🚀 Quick Start (Recommended)

### **1. Install Dependencies**

**All platforms (Windows/Linux/Mac):**
```bash
pip install pyserial matplotlib numpy scipy
```

**That's it!** No complex Bluetooth libraries needed.

### **2. Pair Device (One-time Setup)**

**Windows:**
1. Settings → Bluetooth & devices → Add device
2. Select "NeuroEstimulator"
3. Pair (PIN: 1234 or 0000)
4. Windows will create a virtual COM port (e.g., COM6)

**Linux:**
```bash
sudo bluetoothctl
scan on
pair AA:BB:CC:DD:EE:FF
connect AA:BB:CC:DD:EE:FF
```

**Mac:**
1. System Preferences → Bluetooth
2. Connect to "NeuroEstimulator"

### **3. Run Auto-Detect Capture**

```bash
python capture_bluetooth_simple.py
```

**What it does:**
1. 🔍 Auto-detects "NeuroEstimulator" COM port
2. 🔗 Connects via serial (Bluetooth SPP)
3. 📤 Sends START command: `{"cd":11,"mt":"x"}`
4. 📊 Captures 10 seconds @ 215 Hz (~2150 samples)
5. 📤 Sends STOP command: `{"cd":12,"mt":"x"}`
6. 🔌 Closes connection automatically
7. 📈 Generates analysis plots + CSV

**Expected output:**
```
============================================================
  BLUETOOTH sEMG CAPTURE (Serial Auto-Detect)
============================================================

🔍 Searching for 'NeuroEstimulator' on serial ports...

📡 Found 3 serial port(s):
      COM3
       Description: USB Serial Port
   ✅ COM6
       Description: Standard Serial over Bluetooth link (COM6)
       Manufacturer: Microsoft
      COM7
       Description: Intel(R) Active Management Technology

✅ Target port: COM6

============================================================
  DATA CAPTURE - 10s @ 215 Hz
============================================================

📤 START_STREAM → {"cd":11,"mt":"x"}
📊 Capturing...

    20% | Samples:  430 | Packets:   9
    40% | Samples:  860 | Packets:  17
    60% | Samples: 1290 | Packets:  26
    80% | Samples: 1720 | Packets:  34
   100% | Samples: 2150 | Packets:  43

📤 STOP_STREAM → {"cd":12,"mt":"x"}

============================================================
  RESULTS
============================================================
  Duration:     10.02 s
  Samples:      2150 / 2150 expected
  Packets:      43
  Rate:         214.6 Hz
  Errors:       0
  Complete:     100.0%
============================================================

============================================================
  SIGNAL ANALYSIS
============================================================
  ADC range:    1200 to 1800
  Voltage:      0.2250 to 0.3375 V
  Mean:         0.2813 V
  Std:          0.0234 V
  RMS:          0.2823 V
============================================================

  Peak freq:    23.45 Hz
  Peak mag:     145.23

📊 Plot: bluetooth_capture_20251016_153045.png
💾 CSV:  bluetooth_capture_20251016_153045.csv

✅ Test completed successfully!

🔌 Disconnected
```

---

## 🔧 Advanced Usage

### **Custom Duration**

```bash
# Capture 30 seconds
python capture_bluetooth_auto.py --duration 30

# Capture 60 seconds
python capture_bluetooth_auto.py --duration 60
```

### **Different Device Name**

```bash
python capture_bluetooth_auto.py --device "MyCustomDevice"
```

### **Detailed Packet Inspection**

For debugging packet structure:

```bash
python test_bluetooth_native.py
```

Shows:
- Raw hex dump of packets
- Header breakdown (magic byte, code, timestamp, count)
- Individual sample values
- Validation results

**Example output:**
```
📦 PACKET HEADER:
  Magic Byte:    0xAA ✅
  Message Code:  0x0D (decimal: 13) ✅
  Timestamp:     12345 ms (12.35 seconds)
  Sample Count:  50 samples
  Expected Size: 108 bytes

📊 SAMPLE DATA (first 5):
  Sample[0]:   2482 (raw) →  465.38 mV
  Sample[1]:   2483 (raw) →  465.56 mV
  Sample[2]:   2484 (raw) →  465.75 mV
  Sample[3]:   2485 (raw) →  465.94 mV
  Sample[4]:   2486 (raw) →  466.13 mV

✅ PACKET VALID (108 bytes)
```

### **Manual MAC Address**

If auto-discovery fails:

```bash
# Use specific MAC address
python test_bluetooth_native.py --address "AA:BB:CC:DD:EE:FF"
```

---

## 📊 Output Files

### **CSV File** (`bluetooth_capture_YYYYMMDD_HHMMSS.csv`)

Format:
```csv
Time(s),ADC,Voltage(V)
0.0000,2482,0.4654
0.0047,2483,0.4656
0.0093,2484,0.4658
...
```

### **Plot File** (`bluetooth_capture_YYYYMMDD_HHMMSS.png`)

Contains 3 subplots:
1. **Raw ADC Signal** - Time-domain ADC values
2. **Voltage Signal** - Converted to millivolts with RMS line
3. **Frequency Spectrum** - FFT showing passband (10-50 Hz) and notch (60 Hz)

---

## 🐛 Troubleshooting

### ❌ "pyserial not installed"

**All platforms:**
```bash
pip install pyserial
```

---

### ❌ "Device not found"

**Check:**
1. ESP32 is powered on
2. Bluetooth is enabled on your computer
3. Device name matches "NeuroEstimulator"

**Try:**
```bash
# Scan manually first
python -c "import bluetooth; print(bluetooth.discover_devices(lookup_names=True))"
```

---

### ❌ "Connection failed"

**Possible causes:**

1. **Device already connected to another app**
   - Close Serial Bluetooth Terminal
   - Close PlatformIO monitor
   - Disconnect in system Bluetooth settings

2. **Pairing issues (Windows)**
   - Go to Settings → Bluetooth & devices
   - Remove "NeuroEstimulator" if paired
   - Run script again (will connect without pairing)

3. **Permission denied (Linux)**
   ```bash
   # Add user to bluetooth group
   sudo usermod -a -G bluetooth $USER
   # Log out and log back in
   ```

---

### ❌ "No data captured" / "Low sample rate"

**Check ESP32 logs:**
```bash
pio device monitor
```

**Should see:**
```
[MAIN] Starting NeuroEstimulator firmware...
[ADC] Initializing ADS1115...
[sEMG] Streaming enabled (215 Hz)
[ADC] buffer: 50/512 samples
```

**If buffer overflow:**
```
[ADC] buffer: 512/512 samples  ← Buffer full!
```
→ Increase `STREAMING_BUFFER_SIZE` in `platformio.ini`

---

### ⚠️ "Packet errors" / "Incomplete data"

**Common causes:**
1. **Bluetooth interference** - Move closer to device
2. **Buffer overflow** - Check ESP32 logs
3. **Packet corruption** - Verify binary protocol version

**Test packet structure:**
```bash
python test_bluetooth_native.py
```

---

## ✅ Success Criteria

Your Bluetooth streaming is working correctly if:

- ✅ Device found in <10 seconds
- ✅ Connection established successfully
- ✅ ~2150 samples captured in 10 seconds
- ✅ Actual rate: 210-220 Hz (±5 Hz tolerance)
- ✅ Packet errors: 0
- ✅ Completeness: >95%
- ✅ FFT shows attenuation at 60 Hz (notch filter)
- ✅ Connection closes cleanly

---

## 📚 Protocol Reference

### **JSON Commands**

```json
Start streaming:
{"cd":11,"mt":"x"}

Stop streaming:
{"cd":12,"mt":"x"}
```

### **Binary Packet Structure**

```
Header (8 bytes):
  [0]     magic       0xAA (170)
  [1]     code        13 (0x0D)
  [2-5]   timestamp   uint32_t (ms since boot)
  [6-7]   count       uint16_t (samples in packet)

Payload (count × 2 bytes):
  [8-9]   sample[0]   int16_t (ADC value)
  [10-11] sample[1]   int16_t
  ...
```

### **Expected Values**

| Parameter | Value |
|-----------|-------|
| Sampling Rate | 215 Hz (fixed) |
| Samples/Packet | 50 |
| Packet Size | 108 bytes |
| Packet Rate | ~4.3 packets/second |
| Bandwidth | 464 bytes/s (48% @ 9600 baud) |

---

## 🎯 Comparison: Auto vs Manual Connection

| Feature | `capture_bluetooth_auto.py` | `capture_bluetooth_stream.py` |
|---------|---------------------------|------------------------------|
| **Connection** | ✅ Automatic | ❌ Manual pairing required |
| **Scan** | ✅ Auto-discovery | ❌ Must know COM port |
| **Disconnect** | ✅ Automatic | ⚠️ Manual |
| **Platform** | ✅ Cross-platform | ⚠️ COM port varies |
| **Ease of Use** | 🌟🌟🌟🌟🌟 | 🌟🌟 |

**Recommendation**: Use `capture_bluetooth_auto.py` for all testing! 🚀

---

## 📖 Documentation References

- **Binary Protocol**: `docs/api/bluetooth-protocol.md`
- **Quick Reference**: `docs/api/streaming-protocol.md`
- **Data Capture Guide**: `docs/guides/data-capture.md`
- **Firmware Source**: `src/modules/semg/Semg.cpp`

---

**Last Updated**: 2025-10-16
**Firmware Compatibility**: v3.0+ (215 Hz fixed rate)
**Python Version**: 3.7+
