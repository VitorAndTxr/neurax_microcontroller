# Quick Start - Bluetooth Testing

**3 simple steps to test Bluetooth sEMG streaming:**

---

## Step 1: Install Dependencies

```bash
pip install pyserial matplotlib numpy scipy
```

**That's it!** No complex Bluetooth libraries needed (PyBluez issue solved).

---

## Step 2: Pair Device (One-time)

### Windows:
1. Open **Settings** → **Bluetooth & devices**
2. Click **Add device**
3. Select **Bluetooth**
4. Choose **"NeuroEstimulator"**
5. Pair (PIN: **1234** or **0000**)

Windows will automatically create a COM port (e.g., COM6).

### Linux:
```bash
bluetoothctl
scan on
# Wait for device to appear
pair AA:BB:CC:DD:EE:FF
connect AA:BB:CC:DD:EE:FF
exit
```

### Mac:
1. **System Preferences** → **Bluetooth**
2. Connect to **"NeuroEstimulator"**

---

## Step 3: Run Test

```bash
python capture_bluetooth_simple.py
```

**Done!** The script will:
- ✅ Auto-detect the Bluetooth COM port
- ✅ Connect to device
- ✅ Send START command and receive JSON ACK
- ✅ Capture 10 seconds @ 215 Hz (binary packets)
- ✅ Generate plots + CSV
- ✅ Disconnect automatically

---

## Expected Output

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

✅ Target port: COM6

============================================================
  DATA CAPTURE - 10s @ 215 Hz
============================================================

✅ Connected to COM6 @ 9600 baud
📤 START_STREAM → {"cd":11,"mt":"x"}
✅ ACK received - streaming started
📊 Capturing...

    20% | Samples:  430 | Packets:   9
    40% | Samples:  860 | Packets:  17
    60% | Samples: 1290 | Packets:  26
    80% | Samples: 1720 | Packets:  34
   100% | Samples: 2150 | Packets:  43

📤 STOP_STREAM → {"cd":12,"mt":"x"}
🔌 Port closed

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

📊 Plot saved: semg_capture_20251016_153045.png
💾 CSV saved:  semg_capture_20251016_153045.csv

✅ Test completed successfully!
```

---

## Troubleshooting

### ❌ "Device not found"

**Solution 1:** Specify port manually
```bash
python capture_bluetooth_simple.py --port COM6
```

**Solution 2:** Check pairing
- Windows: Settings → Bluetooth → Make sure "NeuroEstimulator" is paired
- Run: `python -m serial.tools.list_ports` to see all ports

### ❌ "Port access denied"

**Windows:** Close PlatformIO monitor or Serial Bluetooth Terminal
**Linux:** Add user to dialout group:
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

### ❌ "No data captured"

1. Check ESP32 is powered on
2. Verify firmware is running: `pio device monitor`
3. Try reconnecting Bluetooth

---

## Files Generated

| File | Description |
|------|-------------|
| `semg_capture_YYYYMMDD_HHMMSS.csv` | Raw data (Time, ADC, Voltage) |
| `semg_capture_YYYYMMDD_HHMMSS.png` | 3 plots (ADC, Voltage, FFT) |

---

## Next Steps

### Custom Duration
```bash
python capture_bluetooth_simple.py --duration 30  # 30 seconds
```

### Multiple Captures
```bash
# Run multiple times - auto-generates unique filenames
python capture_bluetooth_simple.py
python capture_bluetooth_simple.py
python capture_bluetooth_simple.py
```

### Detailed Packet Inspection
```bash
python test_bluetooth_packets.py COM6
```

---

## Complete Documentation

For advanced usage, see:
- **[BLUETOOTH_TEST_GUIDE.md](./BLUETOOTH_TEST_GUIDE.md)** - Full testing guide
- **[docs/api/bluetooth-protocol.md](./docs/api/bluetooth-protocol.md)** - Protocol specification

---

**Last Updated:** 2025-10-16
**Firmware:** v3.1.0 (215 Hz fixed rate)
