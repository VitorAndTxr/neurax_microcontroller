# CLAUDE.md — NeuroEstimulator Firmware (PRISM sEMG/FES Device)

ESP32-based sEMG acquisition + FES therapeutic stimulation firmware. Part of the [PRISM ecosystem](../CLAUDE.md). C++ / Arduino / FreeRTOS / PlatformIO.

## Build and Upload Policy

**DO NOT execute build or upload commands automatically.** The user handles all `pio run` and `pio run --target upload` operations manually.

## Commands

```bash
pio run                           # Build firmware
pio run --target upload           # Flash to ESP32
pio run --target clean            # Clean build
pio device monitor                # Serial monitor (115200 baud)
pio device list                   # List serial ports
```

All hardware config via `-D` flags in `platformio.ini`. Default values in headers are **overridden** by platformio.ini.

## Safety-Critical Code

Extreme caution when modifying:
1. `Fes::fesLoop()` — Direct H-bridge control, pulse timing affects patient safety
2. `Session::pauseFromMessageHandler()` — Must handle mid-stimulation emergency stops
3. I2C operations — Race conditions crash ADC/gyroscope; always use `i2cMutex`
4. Timer callbacks — `Semg::samplingCallback()` runs in ISR context

## Module Interdependencies

- **Session ↔ Semg ↔ Fes**: Session calls `Semg::isTrigger()`, triggers `Fes::fesLoop()`
- **MessageHandler ↔ All**: Central Bluetooth command router on Core 1
- **Potentiometer ↔ Fes**: Amplitude control with voltage verification
- **Adc ↔ Semg/BatteryMonitor**: Shared I2C bus, protected by `i2cMutex`
- **Singleton pattern**: All modules use static-only classes (deleted constructors)

## Common Pitfalls

1. `FES_MODULE_ENABLE=false` in platformio.ini — FES compiles but does nothing
2. I2C conflicts — ADC + gyroscope share bus; always use `i2cMutex`
3. `JSON_BUFFER_SIZE=512` — Large messages silently fail to parse
4. UART2 conflict — Bluetooth uses GPIO16/GPIO17; don't reassign
5. `Semg::samplingTimer` is a **hardware timer**, not FreeRTOS software timer

## Development Workflow

1. Modify code in `src/modules/<module>/`
2. Update `platformio.ini` if needed
3. Build: `pio run`
4. Flash: `pio run --target upload`
5. Monitor: `pio device monitor`
6. Test via Bluetooth: Send JSON commands, verify responses

---

## Documentation Index

Load only docs relevant to the current task.

| Document | Path | When to Load |
|----------|------|-------------|
| **PRISM Ecosystem** | `agent_docs/prism-ecosystem.md` | Cross-component work, understanding PRISM architecture |
| **Firmware Architecture** | `agent_docs/firmware-architecture.md` | Understanding FreeRTOS tasks, message flow, session state machine, FES pulses, sEMG pipeline |
| **Bluetooth Testing** | `agent_docs/bluetooth-testing.md` | Testing FES sessions or sEMG streaming via Bluetooth |
| **Project Structure** | `agent_docs/project-structure.md` | Finding files, understanding directory layout |
| **Protocol Endpoints** | `docs/endpoints.md` | Complete Bluetooth message code catalog (codes 0-14) |
| **System Workflows** | `docs/workflows.md` | Mermaid diagrams: boot, command routing, session lifecycle, streaming |
| **Class Diagram** | `docs/diagrams/class-diagram.md` | Module dependencies, task map, hardware peripherals |
| **Data Structures** | `docs/database/diagrams.md` | Runtime data model, circular buffer architecture |
| **Complete BT Protocol** | `docs/api/COMPLETE_BLUETOOTH_PROTOCOL.md` | Full dual-protocol reference (JSON + binary) |
| **Streaming Protocol** | `docs/api/streaming-protocol.md` | 215 Hz binary streaming quick reference |
| **Integration Guide** | `docs/INTEGRATION_GUIDE.md` | Cross-component integration patterns |
| **Changelog** | `CHANGELOG.md` | Version history (v1.0 → v3.1) |

### Technology Summary

| Aspect | Detail |
|--------|--------|
| **Platform** | ESP32 (esp32doit-devkit-v1), Dual-Core Xtensa LX6 |
| **Stack** | C++ / Arduino / FreeRTOS / PlatformIO |
| **Communication** | Bluetooth SPP (HC-05/HC-06), UART2, 9600 baud |
| **Sensors** | ADS1115 ADC (860 SPS), MPU6050 IMU, AD8232 sEMG frontend |
| **Streaming** | Fixed 215 Hz, binary protocol, 108-byte packets, 464 B/s |
| **Libraries** | ArduinoJson 6.x, Adafruit ADS1X15, Adafruit MPU6050, libFilter |
| **Version** | v3.1.0 |
