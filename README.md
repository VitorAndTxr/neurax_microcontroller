# InteroperableResearchsEMGDevice

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)

ESP32-based sEMG (surface electromyography) device with FES (Functional Electrical Stimulation) capabilities for the PRISM research framework.

## 📡 Real-Time sEMG Streaming

The firmware supports high-speed sEMG data streaming via **binary protocol** over Bluetooth:

- **215 Hz sampling rate** (fixed, hardware-optimized)
- **50 samples per packet** (~230ms latency)
- **108 bytes per packet** (72% smaller than JSON)
- **48% bandwidth usage** @ 9600 baud (comfortable margin)

For implementation details, see:
- **Full documentation**: [`docs/api/bluetooth-protocol.md`](./docs/api/bluetooth-protocol.md)
- **Quick reference**: [`docs/api/streaming-protocol.md`](./docs/api/streaming-protocol.md)
- **Data capture guide**: [`docs/guides/data-capture.md`](./docs/guides/data-capture.md)

## Configuration values

Configuration values are set in the [platformio.ini](./platformio.ini) file.
Available configurations are:

| General configurations                  | Flag Value    | Explanation     |
|-----------------------------------------|---------------|-----------------|
| SDA_PIN                                 | GPIO21        |I2C SDA pin in ESP32. |
| SCL_PIN                                 | GPIO22        |I2C SCL pin in ESP32. |
| ADC_I2C_ADDR                            | 0x48          |I2C address of the ADC. |
| DEBUG                                   | true          |Enables or disables the debug serial prints. |

| Battery configurations                  | Default Value | Explanation     |
|-----------------------------------------|---------------|-----------------|
| STIMULI_BATTERY_INPUT_PIN               | 3             |The ADC pin to which the stimulation batteries are connected in order to be monitored.|
| MAIN_BATTERY_INPUT_PIN                  | 2             |The ADC pin to which the main batteries are connected in order to be monitored.|
| STIMULI_BATTERY_THRESHOLD               | 3.0           |Low battery voltage threshold (after attenuation)|
| MAIN_BATTERY_THRESHOLD                  | 3.0           |Low battery voltage threshold (after attenuation)|

| LEDs configurations                     | Default Value | Explanation     |
|-----------------------------------------|---------------|-----------------|
| LED_PIN_TRIGGER                         | GPIO25        |                 |
| LED_PIN_FES                             | GPIO26        |                 |
| LED_PIN_POWER                           | GPIO27        |                 |

| FES configurations                      | Default Value | Explanation     |
|-----------------------------------------|---------------|-----------------|
| FES_MODULE_ENABLE                       | true          |Define whether the stimulation is enabled or not. May be disabled for testing purposes. |
| H_BRIDGE_INPUT_1                        | 32            |                 |
| H_BRIDGE_INPUT_2                        | 33            |                 |
| POTENTIOMETER_PIN_INCREMENT             | 25            |                 |
| POTENTIOMETER_PIN_UP_DOWN               | 26            |                 |
| POTENTIOMETER_PIN_CS                    | 27            |                 |
| DEFAULT_POTENTIOMETER_STEPS             | 1             |Amount of increment steps the digital potentiometer should take every time a increment (or decrement) function is called.|
| MAXIMUM_POTENTIOMETER_STEPS             | 100           |                 |
| DEFAULT_STIMULI_DURATION                | 0             | Total duration of FES stimulation in seconds. |
| DEFAULT_PULSE_WIDTH                     | 0             |                 |
| DEFAULT_FREQUENCY                       | 0             |                 |


| sEMG configurations                     | Default Value | Explanation     |
|-----------------------------------------|---------------|-----------------|
| SEMG_ADC_PIN                            | 0             | The ADC pin to which the sEMG module's output is connected. |
| SEMG_ENABLE_PIN                         | 18            | The ESP32 pin to which the AD8232 sEMG module Enable pin is connected.|
| SEMG_FILTER_LOW_CUTOFF_FREQUENCY        | 10.0          |                 |
| SEMG_FILTER_HIGH_CUTOFF_FREQUENCY       | 40.0          |                 |
| SEMG_SAMPLING_TIME                      | 0.01          |                 |
| SEMG_DEFAULT_GAIN                       | 100           |                 |
| SEMG_DIFFICULTY_DEFAULT                 | 16            |                 |
| SEMG_DIFFICULTY_MINIMUM                 | 1             |                 |
| SEMG_DIFFICULTY_MAXIMUM                 | 100           |                 |
| SEMG_DIFFICULTY_INCREMENT               | 1             |                 |
| SEMG_SAMPLES_PER_VALUE                  | 50            |                 |
| SEMG_SAMPLES_PER_AVERAGE                | 10            |                 |
| SEMG_LOW_IMPEDANCE_THRESHOLD            | 10            | Voltage above which the impedance is considered too low. Value in Volts.|
| SEMG_TRIGGER_THRESHOLD_MINIMUM          | 3             | Minimum trigger threshold voltage. |

| Streaming configurations                | Default Value | Explanation     |
|-----------------------------------------|---------------|-----------------|
| STREAMING_BUFFER_SIZE                   | 512           | Circular buffer size for streaming samples (int16_t values). |
| MAX_SAMPLES_PER_PACKET                  | 50            | Number of samples per binary packet. |
| SEMG_FIXED_RATE_HZ                      | 215           | Fixed sampling rate in Hz (860 Hz ADC ÷ 4 downsample). |
| STREAMING_TIMEOUT_MINUTES               | 10            | Automatic streaming stop timeout in minutes. |

## 🛠️ Build and Upload

```bash
# Build firmware
pio run

# Upload to ESP32 (auto-detect port)
pio run --target upload

# Open serial monitor
pio device monitor

# Build, upload, and monitor
pio run --target upload && pio device monitor
```

**Important:** Per project policy, firmware compilation and upload must be done manually by the user.

## 🧪 Testing Bluetooth Streaming

### **Automatic Test (Recommended)**

```bash
# Install dependencies (simple!)
pip install pyserial matplotlib numpy scipy

# Pair device in Bluetooth settings (one-time)
# Windows: Settings → Bluetooth → Add "NeuroEstimulator"
# Linux: bluetoothctl → pair & connect

# Run automatic capture (auto-detects port, captures 10s)
python capture_bluetooth_simple.py
```

### **What it does:**
- ✅ Auto-detects "NeuroEstimulator" COM port
- ✅ Connects via Bluetooth (serial SPP)
- ✅ Captures 10 seconds @ 215 Hz
- ✅ Generates plots + CSV
- ✅ Disconnects automatically

**For detailed instructions**, see: **[BLUETOOTH_TEST_GUIDE.md](./BLUETOOTH_TEST_GUIDE.md)**

## 📚 Documentation

Comprehensive documentation is available in the [`docs/`](./docs/) directory:
- **[docs/README.md](./docs/README.md)** - Documentation index
- **[CLAUDE.md](./CLAUDE.md)** - AI assistant development guide
- **[CHANGELOG.md](./CHANGELOG.md)** - Version history

## 📦 Dependencies

- **FreeRTOS** - Real-time operating system (included with ESP32)
- **[libFilter](https://github.com/MartinBloedorn/libFilter)** - Digital signal processing library (git submodule)
- **ArduinoJson** - JSON serialization (via PlatformIO)
- **Adafruit ADS1X15** - ADC driver (via PlatformIO)
