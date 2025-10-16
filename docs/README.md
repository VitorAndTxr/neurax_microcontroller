# InteroperableResearchsEMGDevice - Documentation

This documentation covers the sEMG/FES device firmware for the PRISM project.

## Quick Links

- **[Main README](../README.md)** - Project overview and quick start
- **[CLAUDE.md](../CLAUDE.md)** - AI assistant guidance and development reference
- **[CHANGELOG](../CHANGELOG.md)** - Version history and changes

---

## Documentation Structure

### 📡 API Documentation (`/api`)

Communication protocols and message formats:

- **[bluetooth-protocol.md](api/bluetooth-protocol.md)** - Binary streaming protocol specification (v1.1, 215 Hz)
- **[streaming-protocol.md](api/streaming-protocol.md)** - Quick reference for 215 Hz streaming

### 🏗️ Architecture (`/architecture`)

System design and data flow:

- **[streaming-architecture.md](architecture/streaming-architecture.md)** - sEMG streaming system flowchart and architecture

### 🔧 Development Notes (`/development`)

Technical analysis and implementation details:

- **[adc-analysis.md](development/adc-analysis.md)** - ADC performance analysis (ADS1115 @ 860 SPS)
- **[continuous-mode.md](development/continuous-mode.md)** - Continuous ADC sampling implementation plan (COMPLETED in v3.0.0)
- **[bugfixes.md](development/bugfixes.md)** - Sample loss fix with circular buffer (v3.1.0)

### 📖 User Guides (`/guides`)

End-user documentation:

- **[data-capture.md](guides/data-capture.md)** - Guide for capturing and analyzing sEMG data at 215 Hz

---

## Core Features

### Hardware
- **Platform**: ESP32 (esp32doit-devkit-v1)
- **ADC**: ADS1115 (16-bit, I2C, 860 SPS)
- **sEMG Sensor**: AD8232 (analog front-end)
- **Gyroscope**: MPU6050 (I2C)
- **Bluetooth**: HC-05/HC-06 (SPP, 9600/115200 baud)
- **FES**: H-bridge for biphasic stimulation

### Firmware Capabilities
- **FES Stimulation**: Programmable biphasic pulses (amplitude, frequency, pulse width)
- **sEMG Processing**: Butterworth bandpass filtering (10-50 Hz) + 60 Hz notch filter
- **Real-time Streaming**: 215 Hz filtered data via Bluetooth binary protocol
- **Trigger Detection**: Threshold-based sEMG trigger for FES activation
- **Dual-core Architecture**: Core 0 for time-critical ADC tasks, Core 1 for communication

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   ESP32 Dual-Core                        │
├──────────────────────┬──────────────────────────────────┤
│      Core 0          │           Core 1                  │
│  (Time-Critical)     │      (Communication)              │
│                      │                                   │
│  • ADC Sampling      │  • Bluetooth Handler              │
│    (860 Hz)          │  • Message Processing             │
│  • Downsampling      │  • Streaming Task                 │
│    (4x → 215 Hz)     │  • JSON Serialization             │
│  • Buffer Management │                                   │
└──────────────────────┴──────────────────────────────────┘
         │                        │
         ▼                        ▼
   [ADS1115 ADC]         [HC-05 Bluetooth]
         │                        │
         ▼                        ▼
   [AD8232 sEMG]          [Mobile App]
```

---

## Development Workflow

### Prerequisites
- [PlatformIO](https://platformio.org/) installed
- ESP32 USB drivers (CH340/CP2102)
- Python 3.7+ (for data capture scripts)

### Build and Upload
```bash
# Build firmware
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor

# All in one
pio run --target upload && pio device monitor
```

### Testing Streaming
```bash
# Capture 215 Hz data to CSV
python capture_semg_data.py
```

---

## Configuration

All hardware parameters are configured via `platformio.ini`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `SEMG_ADC_PIN` | 0 | ADC channel for sEMG |
| `ADC_DOWNSAMPLE_RATIO` | 4 | 860 Hz → 215 Hz |
| `STREAMING_BUFFER_SIZE` | 512 | Circular buffer size |
| `SEMG_FILTER_LOW_CUTOFF_FREQUENCY` | 10.0 | Butterworth low cutoff (Hz) |
| `SEMG_FILTER_HIGH_CUTOFF_FREQUENCY` | 50.0 | Butterworth high cutoff (Hz) |
| `FES_MODULE_ENABLE` | false | Enable/disable FES hardware |
| `SEMG_FIXED_RATE_HZ` | 215 | Fixed sampling rate |

See [`platformio.ini`](../platformio.ini) for full configuration.

---

## Version Information

**Firmware Version**: v3.1.0 (current)
**Protocol Version**: Binary v1.1 (215 Hz fixed rate)
**Documentation Updated**: 2025-10-16

---

## Contributing

When adding documentation:
1. Place in appropriate category (`api/`, `architecture/`, `development/`, `guides/`)
2. Update this README with a link
3. Use clear, concise English
4. Include code examples where applicable
5. Add diagrams for complex concepts

---

## Support

For issues, questions, or contributions:
- Check existing documentation first
- Review [CHANGELOG.md](../CHANGELOG.md) for recent changes
- Consult [CLAUDE.md](../CLAUDE.md) for AI-assisted development guidance
- Use PlatformIO serial monitor with `DEBUG=true` for detailed logs

---

**Developed for PRISM (Project Research Interoperability and Standardization Model)**
*Federal University - Biomedical Engineering Laboratory*
