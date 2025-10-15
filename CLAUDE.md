# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## ⚠️ IMPORTANT: Build and Upload Policy

**DO NOT execute build or upload commands automatically.** All firmware compilation and upload operations (`pio run`, `pio run --target upload`) must be performed manually by the user. The assistant should:
- ✅ Modify code files as requested
- ✅ Suggest build/upload commands to the user
- ✅ Explain what changes were made
- ❌ **NEVER** execute `pio run` or `pio run --target upload` automatically
- ❌ **NEVER** attempt to compile or flash the firmware without explicit user confirmation

The user will handle all compilation steps themselves.

## PRISM Project - Master Architecture Overview

**PRISM** (Project Research Interoperability and Standardization Model) is a comprehensive federated framework for biomedical research data management, designed to break down data silos and enable secure, standardized collaboration across research institutions.

### Ecosystem Components

The PRISM framework consists of four interconnected components:

1. **InteroperableResearchNode** (Backend): Core backend server implementing federated research data exchange with 4-phase cryptographic handshake protocol, PostgreSQL/Redis persistence, and 28-table clinical data model. See `../InteroperableResearchNode/CLAUDE.md`.

2. **InteroperableResearchsEMGDevice** (Embedded - **This Project**): ESP32-based hardware device for biosignal acquisition and therapeutic stimulation.

3. **InteroperableResearchInterfaceSystem** (Interface): TypeScript/Node.js middleware for protocol translation and communication orchestration. See `../InteroperableResearchInterfaceSystem/CLAUDE.md`.

4. **neurax_react_native_app** (Mobile): React Native application for research data collection and device control.

### PRISM Model Abstraction

```
┌─────────────────────────────────────────────────────────────┐
│                    Research Institution                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐         ┌──────────────┐                  │
│  │ Application  │────────▶│   Device     │  ◄── YOU ARE HERE│
│  │ (Mobile App) │  BT     │  (sEMG/FES)  │                  │
│  └──────┬───────┘         └──────────────┘                  │
│         │ HTTPS                                              │
│  ┌──────▼────────────────────────────────────────┐          │
│  │    Interoperable Research Node (IRN)          │          │
│  └───────────────────────────┬───────────────────┘          │
│                              │ Encrypted Channel              │
└──────────────────────────────┼───────────────────────────────┘
                               │
                ┌──────────────▼──────────────┐
                │   Federated PRISM Network   │
                └─────────────────────────────┘
```

### Key Design Principles

1. **Separation of Concerns**: Device (capture) ≠ Application (context) ≠ Node (storage/federation)
2. **Standardization**: HL7 FHIR + SNOMED CT for interoperability
3. **Real-time Processing**: Dual-core ESP32 architecture for time-critical signal processing
4. **Bluetooth Protocol**: JSON-based command-response pattern for mobile app communication
5. **Safety**: Multi-layer protections for therapeutic stimulation (emergency stops, voltage verification, timeouts)

### Data Flow (Complete Research Session)

```
1. Researcher configures via Mobile App → Bluetooth JSON commands
2. ESP32 Device acquires biosignals → 860 Hz sampling, Butterworth filtering
3. Trigger detection → sEMG threshold exceeded
4. FES stimulation → Biphasic pulses via H-bridge
5. Real-time streaming → JSON packets to Mobile App (10-200 Hz)
6. Data submission → Mobile App sends to Research Node (future)
```

### Navigation for AI Assistants

When working on this codebase:

1. **Device Firmware** → You are here (`InteroperableResearchsEMGDevice/CLAUDE.md`)
2. **Backend/Node Development** → See `../InteroperableResearchNode/CLAUDE.md`
3. **Interface System** → See `../InteroperableResearchInterfaceSystem/CLAUDE.md`
4. **Master Overview** → See root `../CLAUDE.md` for cross-component context

---

## Project Overview

This is the **NeuroEstimulator** firmware - an ESP32-based electrostimulation device that combines Functional Electrical Stimulation (FES) with surface electromyography (sEMG) for neurorehabilitation. The system detects muscle activity via sEMG sensors and triggers electrical stimulation in response.

**Key Hardware**: ESP32 (esp32doit-devkit-v1), Bluetooth HC-05/HC-06, MPU6050 gyroscope, ADS1115 ADC, AD8232 sEMG sensor, H-bridge for FES, digital potentiometer.

**Role in PRISM Ecosystem**: Represents the **Device** abstraction - specialized hardware for biosignal capture and therapeutic stimulation, communicating with mobile applications via Bluetooth for data collection and real-time visualization.

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
- Methods: `r` (read), `w` (write), `x` (execute), `a` (acknowledgment)
- Message codes:
  - `1`: Gyroscope commands
  - `2-8`: Session commands (start, stop, pause, resume, single stimulus, parameters, status)
  - `9`: Trigger test
  - `11-14`: sEMG streaming commands (start, stop, data, config)

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

The sEMG module supports **two operating modes**: trigger detection for FES sessions and real-time data streaming.

#### Mode 1: Trigger Detection (FES Sessions)

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

#### Mode 2: Real-Time Streaming

**Streaming architecture** (`src/modules/semg/Semg.cpp:298-483`):
1. **Timer callback** writes to circular buffer (`STREAMING_BUFFER_SIZE=200` samples)
2. **Streaming task** (Core 1, priority 10) sends packets via Bluetooth
3. **Configurable data types**: `raw`, `filtered` (Butterworth), or `rms` (envelope)
4. **Configurable rates**: 10-200 Hz (default: 20 Hz, recommended: 10-30 Hz for 9600 baud Bluetooth)

**Flow**:
1. `Semg::configureStreaming(rate, type)` → Set parameters
2. `Semg::enableStreaming()` → Start sampling timer + streaming task
3. `samplingCallback()` → Reads ADC, applies filter, writes to buffer
4. `streamingTask()` → Batches samples, sends JSON packets at configured rate
5. `Semg::disableStreaming()` → Stop task and reset buffer

**Safety features**:
- Automatic timeout after 10 minutes (configurable via `STREAMING_TIMEOUT_MINUTES`)
- Buffer overflow protection (drops oldest samples)
- Bluetooth disconnect detection

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

Use `src/modules/message_handler/sample_messages.json` for reference.

### Testing FES Sessions

**Basic session test sequence**:
1. Connect via Bluetooth terminal app (9600 baud)
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

### Testing sEMG Streaming

**Real-time streaming test sequence**:

1. **Configure streaming parameters** (optional, defaults: 50 Hz, raw data):
```json
{"cd":14,"mt":"w","bd":{"rate":100,"type":"filtered"}}
```
Available types: `"raw"`, `"filtered"`, `"rms"`

**ACK response received:**
```json
{"cd":14,"mt":"a"}
```

2. **Start streaming**:
```json
{"cd":11,"mt":"x"}
```

**ACK response received:**
```json
{"cd":11,"mt":"a"}
```

3. **Receive streaming data** (automatically sent at configured rate):
```json
{"cd":13,"mt":"w","bd":{"t":12345,"v":[23.4,25.1,22.8,24.5,26.2,23.9,25.7,24.1,22.5,25.4]}}
```
- `"t"`: Timestamp (milliseconds since boot)
- `"v"`: Array of sEMG values (5-10 samples per packet depending on rate)

4. **Stop streaming**:
```json
{"cd":12,"mt":"x"}
```

**ACK response received:**
```json
{"cd":12,"mt":"a"}
```

**Expected streaming logs**:
```
[MSG] === Received data ===
[MSG] --->
[MSG] {"cd":14,"mt":"w","bd":{"rate":100,"type":"filtered"}}
[MSG] SEMG_STREAMING::CONFIG_STREAM
[sEMG] Configuring streaming: rate=100 Hz, type=filtered
[sEMG] Streaming config: 10 samples/packet, 10 packets/second
[MSG] Sending ACK for message code 14
[MSG] Serialized message:
[MSG] {"cd":14,"mt":"a"}
[MSG] Message sent!

[MSG] === Received data ===
[MSG] --->
[MSG] {"cd":11,"mt":"x"}
[MSG] SEMG_STREAMING::START_STREAM
[sEMG] Enabling streaming...
[sEMG] Streaming task created successfully
[sEMG] Streaming task started
[MSG] Sending ACK for message code 11
[MSG] Serialized message:
[MSG] {"cd":11,"mt":"a"}
[MSG] Message sent!

[MSG] Sending message...
[MSG] {"cd":13,"mt":"w","bd":{"t":12345,"v":[...]}}

[MSG] === Received data ===
[MSG] --->
[MSG] {"cd":12,"mt":"x"}
[MSG] SEMG_STREAMING::STOP_STREAM
[sEMG] Disabling streaming...
[sEMG] Streaming task finished
[MSG] Sending ACK for message code 12
[MSG] Serialized message:
[MSG] {"cd":12,"mt":"a"}
[MSG] Message sent!
```

**Streaming rates and packet structure**:
- **Rate < 100 Hz**: 5 samples/packet
- **Rate ≥ 100 Hz**: 10 samples/packet
- Packets sent at `rate / samples_per_packet` Hz (e.g., 100 Hz → 10 packets/sec)

**Important notes**:
- Streaming automatically stops after 10 minutes or if Bluetooth disconnects
- Cannot run streaming and FES session simultaneously (shared timer)
- Buffer holds 200 samples; overflow drops oldest data
- **Bluetooth bandwidth limitation**: At 9600 baud, use rates ≤ 30 Hz to avoid buffer overflow
- For higher rates (50-200 Hz), consider upgrading Bluetooth module to 115200 baud

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

## Project Structure

```
InteroperableResearchsEMGDevice/
├── src/
│   ├── main.cpp                    # Entry point with 215Hz streaming test
│   ├── globals.h                   # Core assignments and mutexes
│   └── modules/
│       ├── adc/                    # ADS1115 ADC driver (860 SPS → 215 Hz)
│       ├── semg/                   # sEMG processing + streaming
│       │   ├── Semg.cpp            # Main implementation
│       │   ├── Semg.h              # API definitions
│       │   └── StreamingProtocol.h # Binary protocol header
│       ├── SemgFilter/             # Butterworth filters
│       ├── fes/                    # FES stimulation control
│       ├── session/                # Session state machine
│       ├── message_handler/        # Bluetooth command router
│       ├── bluetooth/              # UART2 communication
│       ├── gyroscope/              # MPU6050 driver
│       ├── potentiometer/          # Digital potentiometer control
│       ├── battery_monitor/        # Battery voltage monitoring
│       ├── led/                    # LED indicators
│       └── debug/                  # Debug utilities
│
├── lib/
│   └── libFilter/                  # External filter library (git submodule)
│
├── docs/                           # 📚 Documentation (organized structure)
│   ├── README.md                   # Documentation index
│   ├── api/                        # Protocol specifications
│   │   ├── bluetooth-protocol.md   # Binary streaming protocol
│   │   └── streaming-protocol.md   # 215Hz streaming quick reference
│   ├── architecture/               # System design
│   │   └── streaming-architecture.md # Flowcharts and diagrams
│   ├── development/                # Technical notes
│   │   ├── adc-analysis.md         # ADC performance analysis
│   │   ├── continuous-mode.md      # Implementation plan
│   │   └── bugfixes.md             # Bug fix documentation
│   └── guides/                     # User documentation
│       └── data-capture.md         # Data capture guide
│
├── platformio.ini                  # Build configuration
├── CLAUDE.md                       # This file - AI assistant guidance
├── README.md                       # Project overview
├── CHANGELOG.md                    # Version history
├── capture_semg_data.py            # Python data capture utility
├── requirements.txt                # Python dependencies
└── .gitignore                      # Excludes CSV data files

```

**Note**: `include/` and `test/` directories exist but are currently unused.

## Documentation

All project documentation is organized in the `docs/` folder:

- **API Documentation** (`docs/api/`): Bluetooth protocol specifications
- **Architecture** (`docs/architecture/`): System design and data flow diagrams
- **Development Notes** (`docs/development/`): Technical analysis and implementation details
- **User Guides** (`docs/guides/`): End-user documentation for data capture

See `docs/README.md` for the complete documentation index.

**Key Documentation Files**:
- `docs/api/bluetooth-protocol.md` - Binary streaming protocol (v1.0)
- `docs/api/streaming-protocol.md` - 215Hz streaming quick reference
- `docs/guides/data-capture.md` - Guide for capturing sEMG data
- `CHANGELOG.md` - Version history and recent changes

All module headers define TAG constants for logging (e.g., `TAG_FES`, `TAG_SEMG`). Use `ESP_LOGI(TAG_*, ...)` for consistent log formatting.
