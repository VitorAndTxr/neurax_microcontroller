# sEMG Data Capture Guide

Quick guide for capturing and analyzing 215 Hz sEMG data from the ESP32 device.

---

## 🚀 Quick Start: Automatic Bluetooth Capture (Recommended)

### **Option 1: Fully Automatic (Best for Testing)**

```bash
# Install dependencies
pip install pybluez-win10 matplotlib numpy scipy  # Windows
pip install pybluez matplotlib numpy scipy        # Linux/Mac

# Run automatic capture
python capture_bluetooth_auto.py
```

**What it does:**
- ✅ Auto-discovers "NeuroEstimulator" device
- ✅ Connects via native Bluetooth
- ✅ Captures 10 seconds @ 215 Hz
- ✅ Generates plots + CSV
- ✅ Disconnects automatically

**For detailed Bluetooth testing instructions**, see: **[../../BLUETOOTH_TEST_GUIDE.md](../../BLUETOOTH_TEST_GUIDE.md)**

---

## 📋 Manual Capture Methods

### **Option 2: Serial Monitor Capture**

1. **Install Python dependencies**:
```bash
pip install -r requirements.txt
```

Or install only the essentials:
```bash
pip install pyserial
```

2. **Close PlatformIO monitor** if running (it locks the serial port)

## Basic Usage

### Capture 5 seconds (default)
```bash
python capture_semg_data.py
```

Output: `semg_data_YYYYMMDD_HHMMSS.csv`

### Custom duration
```bash
# Capture 10 seconds
python capture_semg_data.py --duration 10

# Capture 30 seconds
python capture_semg_data.py --duration 30
```

### Custom port
```bash
# Windows
python capture_semg_data.py --port COM3

# Linux/Mac
python capture_semg_data.py --port /dev/ttyUSB0
```

### Save to specific file
```bash
python capture_semg_data.py --output my_experiment.csv
```

### Capture and plot
```bash
python capture_semg_data.py --plot
```
(Requires matplotlib and pandas)

## Data Format

The output CSV has three columns:

| Column | Description | Example |
|--------|-------------|---------|
| `timestamp` | Milliseconds since ESP32 boot | 12345 |
| `raw_adc` | Raw ADC value (downsampled 4:1) | 1520 |
| `filtered_value` | Butterworth filtered (10-50 Hz + 60 Hz notch) | 45.23 |

**Sampling rate**: ~215 Hz (860 Hz ADC ÷ 4 downsample)

## Example Output

```
[INFO] Opening serial port COM6 @ 115200 baud...
[INFO] Capturing data for 5 seconds...
[INFO] Output file: semg_data_20251015_183045.csv
------------------------------------------------------------
[PROGRESS] Samples: 50, Rate: 214.3 Hz, Time: 0.2s
[PROGRESS] Samples: 100, Rate: 215.1 Hz, Time: 0.5s
[PROGRESS] Samples: 150, Rate: 214.8 Hz, Time: 0.7s
...
------------------------------------------------------------
[SUCCESS] Capture complete!
  • Total samples: 1075
  • Duration: 5.01 seconds
  • Average rate: 214.6 Hz
  • File saved: semg_data_20251015_183045.csv
```

## Analyzing Data

### Python (pandas)
```python
import pandas as pd
import matplotlib.pyplot as plt

# Load data
df = pd.read_csv('semg_data_20251015_183045.csv')

# Convert to relative time
df['time_s'] = (df['timestamp'] - df['timestamp'].iloc[0]) / 1000.0

# Plot filtered signal
plt.figure(figsize=(12, 4))
plt.plot(df['time_s'], df['filtered_value'])
plt.xlabel('Time (s)')
plt.ylabel('Filtered sEMG (mV)')
plt.title('215 Hz sEMG Signal - Butterworth 10-50 Hz')
plt.grid(True)
plt.show()

# Calculate statistics
print(f"Mean: {df['filtered_value'].mean():.2f}")
print(f"Std: {df['filtered_value'].std():.2f}")
print(f"Min: {df['filtered_value'].min():.2f}")
print(f"Max: {df['filtered_value'].max():.2f}")
```

### MATLAB
```matlab
% Load data
data = readtable('semg_data_20251015_183045.csv');

% Convert to relative time
time_s = (data.timestamp - data.timestamp(1)) / 1000.0;

% Plot
figure;
subplot(2,1,1);
plot(time_s, data.raw_adc);
ylabel('Raw ADC');
title('sEMG Signal @ 215 Hz');
grid on;

subplot(2,1,2);
plot(time_s, data.filtered_value, 'r');
xlabel('Time (s)');
ylabel('Filtered (mV)');
grid on;

% FFT analysis
Fs = 215; % Sampling frequency
L = length(data.filtered_value);
Y = fft(data.filtered_value);
P2 = abs(Y/L);
P1 = P2(1:L/2+1);
P1(2:end-1) = 2*P1(2:end-1);
f = Fs*(0:(L/2))/L;

figure;
plot(f, P1);
xlim([0 100]);
xlabel('Frequency (Hz)');
ylabel('Magnitude');
title('Frequency Spectrum');
grid on;
```

### Excel
Simply open the CSV file in Excel or Google Sheets for basic visualization.

## Troubleshooting

### Error: "Could not open serial port"
- Close PlatformIO monitor (`Ctrl+C`)
- Check port name: `pio device list`
- Verify ESP32 is connected

### Error: "No data captured"
- ESP32 may still be booting (wait 5 seconds after upload)
- Check if ESP32 is running the test firmware
- Monitor manually first: `pio device monitor`

### Low sample rate (<200 Hz)
- **Expected rate: 215 Hz (±5 Hz)**
- Normal variation: 210-220 Hz is acceptable
- If consistently <200 Hz, check ADC logs for buffer overflow
- ADC buffer size: 512 samples (2.4 seconds @ 215 Hz)
- Serial output bottleneck has been fixed with circular buffer (v3.1.0)

## Tips

- **Multiple captures**: Script auto-generates timestamped filenames
- **Long recordings**: Use `--duration 60` for 1 minute (13,000 samples)
- **Live monitoring**: Use `pio device monitor` to see data in real-time
- **Visualization**: Use `--plot` flag for instant graphs

## Advanced: Frequency Analysis

To verify the 10-50 Hz bandpass filter:

```python
import numpy as np
from scipy import signal

df = pd.read_csv('your_data.csv')

# Calculate sampling rate
time_diff = np.diff(df['timestamp']) / 1000.0  # Convert to seconds
fs = 1.0 / np.mean(time_diff)  # Hz

print(f"Actual sampling rate: {fs:.1f} Hz")

# FFT
freqs, psd = signal.welch(df['filtered_value'], fs=fs, nperseg=512)

# Plot frequency spectrum
plt.semilogy(freqs, psd)
plt.xlabel('Frequency (Hz)')
plt.ylabel('Power Spectral Density')
plt.xlim([0, 100])
plt.grid(True)
plt.show()
```

Expected result: Strong attenuation below 10 Hz, above 50 Hz, and at 60 Hz (notch).
