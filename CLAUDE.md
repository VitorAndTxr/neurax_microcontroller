# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the **NeuroEstimulator** firmware - an ESP32-based electrostimulation device that combines Functional Electrical Stimulation (FES) with surface electromyography (sEMG) for neurorehabilitation. The system detects muscle activity via sEMG sensors and triggers electrical stimulation in response.

**Key Hardware**: ESP32 (esp32doit-devkit-v1), Bluetooth HC-05/HC-06, MPU6050 gyroscope, ADS1115 ADC, AD8232 sEMG sensor, H-bridge for FES, digital potentiometer.

## Build and Development Commands

### Building and Flashing
```bash
# Build firmware
pio run

# Upload to ESP32 (auto-detect port)
pio run --target upload

# Upload to specific port (Windows)
pio run --target upload --upload-port COM3

# Upload to specific port (Linux/Mac)
pio run --target upload --upload-port /dev/ttyUSB0

# Clean build
pio run --target clean
```

### Monitoring and Debugging
```bash
# Open serial monitor (115200 baud)
pio device monitor

# List available serial ports
pio device list

# Build, upload, and monitor in one command
pio run --target upload && pio device monitor
```

### Configuration
All hardware and feature configuration is done via `build_flags` in `platformio.ini`. Key flags:
- `FES_MODULE_ENABLE`: Enable/disable FES module (set to `false` for testing without hardware)
- `DEBUG`: Enable verbose serial logging
- Pin assignments, thresholds, and timing parameters

## Architecture

### Core Architecture Pattern: Multi-Core FreeRTOS

The firmware uses **dual-core ESP32** with FreeRTOS task distribution:

- **Core 0**: Session management and sEMG detection (time-critical)
- **Core 1**: Message handling and Bluetooth communication

**Critical globals** (`src/globals.h`):
- `session_cpu = 0` and `secondary_cpu = 1`: Core assignments
- `i2cMutex`: Protects shared I2C bus (ADC, gyroscope)
- `MutexBlu`: Protects Bluetooth serial access

### Message Flow Architecture

The system operates on a **command-response** pattern over Bluetooth:

1. **App → ESP32**: JSON commands via Bluetooth (UART2, 9600 baud)
2. **MessageHandler** (Core 1 task): Deserializes and routes commands
3. **Module execution**: Gyroscope, Session, FES, sEMG
4. **ESP32 → App**: JSON responses via Bluetooth

**Message protocol** (`src/modules/message_handler/CommunicationProtocol.h`):
- All messages are JSON: `{"cd": <code>, "mt": "<method>", "bd": {...}}`
- Methods: `r` (read), `w` (write), `x` (execute)
- Codes 1-9 map to different commands (see `src/modules/message_handler/README.md`)

### Session State Machine

The **Session module** (`src/modules/session/`) is the orchestrator:

**States**:
- `SessionStatus::ongoing`: Session created
- `SessionStatus::paused`: Temporarily suspended
- `SessionStatus::complete_stimuli_amount` / `interrupted_stimuli_amount`: Tracking

**Flow**:
1. `Session::start()` → Creates FreeRTOS task on Core 0
2. `Session::loop()` → Infinite loop: `detectionAndStimulation()`
3. `Semg::isTrigger()` → Detects muscle activation
4. `Fes::fesLoop()` → Generates biphasic pulses via H-bridge
5. Automatic pause after stimulation, resume on command

**Critical**: Session task can be paused two ways:
- `pauseFromSession()`: Self-suspension via `vTaskSuspend(NULL)`
- `pauseFromMessageHandler()`: Emergency stop via `Fes::emergency_stop = true`

### FES Pulse Generation

**Synchronous blocking loop** (`Fes::fesLoop()` in `src/modules/fes/Fes.cpp`):
```cpp
// Biphasic pulse generation
while (millis() - startTime < duration) {
    positiveHBridge();     // Set H-bridge to positive
    delayMicroseconds(pulse_width);
    negativeHBridge();     // Invert polarity
    delayMicroseconds(pulse_width);
    hBridgeReset();        // Discharge
    delayMicroseconds(remaining_time);  // Wait for next cycle
}
```

**Safety mechanisms**:
- `FES_MODULE_ENABLE` compile-time flag (disables all GPIO writes)
- `Fes::emergency_stop`: Checked every pulse cycle
- Voltage verification: Stops if potentiometer error > 0.5V

### sEMG Signal Processing Pipeline

**Real-time filtering** (`src/modules/semg/`):
1. **Timer ISR** (1.162ms period): ADC sampling → `raw_value[]` buffer
2. **Butterworth filter**: 2nd-order bandpass (10-40 Hz) via `libFilter`
3. **RMS calculation**: Average of 50 filtered samples
4. **Threshold comparison**: Dynamic threshold based on difficulty (1-100%)

**Trigger detection** (`Semg::isTrigger()`):
- Requires `SEMG_SAMPLES_PER_VALUE` (50) samples ready
- Filters entire buffer in one pass
- Compares average to `Semg::parameters.threshold`
- Returns boolean, sends BT message if triggered

## Module Interdependencies

**Critical coupling points**:

1. **Session ↔ Semg ↔ Fes**: Session calls `Semg::isTrigger()`, which calls `Fes::fesLoop()` on detection
2. **MessageHandler ↔ All modules**: Central router, imports all module headers
3. **Potentiometer ↔ Fes**: Amplitude control; Fes checks voltage before stimulation
4. **Adc ↔ Semg/BatteryMonitor**: Shared I2C resource, protected by mutex
5. **Gyroscope**: Standalone, only called when app requests reading

**Singleton pattern**: All modules use static-only classes (deleted constructors/destructors)

## Configuration Philosophy

**Compile-time configuration** via `-D` flags in `platformio.ini`:
- Allows hardware-specific builds
- No runtime config files
- Pin assignments, thresholds, timing all in build flags

**Important**: Default values in headers are **overridden** by platformio.ini flags. Always check `platformio.ini` for actual values.

## Safety-Critical Code Regions

When modifying these areas, extreme caution required:

1. **Fes::fesLoop()**: Direct hardware control, pulse timing affects safety
2. **Session::pauseFromMessageHandler()**: Must handle mid-stimulation stops
3. **I2C operations**: Race conditions can crash ADC/gyroscope reads
4. **Timer callbacks**: `Semg::samplingCallback()` runs in ISR context

## Bluetooth Protocol Testing

Use `src/modules/message_handler/sample_messages.json` for test messages:

**Common test sequence**:
1. Connect via Bluetooth terminal app
2. Send: `{"cd":7,"mt":"w","bd":{"a":3.0,"f":38.0,"pw":12.0,"df":5,"pd":5}}` (set parameters)
3. Send: `{"cd":2,"mt":"x"}` (start session)
4. Wait for trigger detection
5. Send: `{"cd":3,"mt":"x"}` (stop session)

**Expected logs**:
```
[MSG] === Received data ===
[MSG] SESSION_COMMANDS::PARAMETERS
[SESSION] Session Start
[sEMG] Variavel istrigger = 1
[FES] Starting stimulation
```

## Common Pitfalls

1. **FES_MODULE_ENABLE=false**: FES code compiles but does nothing. Check platformio.ini if stimulation doesn't work.
2. **I2C conflicts**: ADC and gyroscope share bus. Always use `i2cMutex` when adding I2C operations.
3. **Task priorities**: Session (20) and MessageHandler (20) have equal priority. Core pinning prevents conflicts.
4. **JSON buffer size**: `JSON_BUFFER_SIZE=512`. Large messages silently fail to parse.
5. **UART2 conflict**: Bluetooth uses UART2. Don't reassign GPIO16/GPIO17.
6. **Timer periods**: `Semg::samplingTimer` is **hardware timer**, not FreeRTOS software timer. Different API.

## Development Workflow

1. **Modify code** in `src/modules/<module>/`
2. **Update configuration** in `platformio.ini` if needed
3. **Build**: `pio run` (check for compilation errors)
4. **Flash**: `pio run --target upload`
5. **Monitor**: `pio device monitor` (watch for ESP_LOG messages)
6. **Test via Bluetooth**: Send JSON commands, verify responses

For module testing without full system:
- Uncomment test functions in `main.cpp` `loop()`
- Rebuild and upload
- Monitor serial output

## Project Structure Notes

- `src/modules/`: Each hardware/functional module is self-contained
- `lib/libFilter/`: External filter library (git submodule)
- `include/`: Currently unused (Arduino framework convention)
- `test/`: Empty (no unit tests currently)

All module headers define TAG constants for logging (e.g., `TAG_FES`, `TAG_SEMG`). Use `ESP_LOGI(TAG_*, ...)` for consistent log formatting.
