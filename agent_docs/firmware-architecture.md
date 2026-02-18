# Firmware Architecture

## Core Architecture Pattern: Multi-Core FreeRTOS

The firmware uses **dual-core ESP32** with FreeRTOS task distribution:

- **Core 0**: Session management and sEMG detection (time-critical)
- **Core 1**: Message handling and Bluetooth communication

**Critical globals** (`src/globals.h`):
- `session_cpu = 0` and `secondary_cpu = 1`: Core assignments
- `i2cMutex`: Protects shared I2C bus (ADC, gyroscope)
- `MutexBlu`: Protects Bluetooth serial access

## Message Flow Architecture

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

## Session State Machine

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

## FES Pulse Generation

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

## sEMG Signal Processing Pipeline

The sEMG module supports **two operating modes**: trigger detection for FES sessions and real-time data streaming.

### Mode 1: Trigger Detection (FES Sessions)

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

### Mode 2: Real-Time Streaming (Fixed 215 Hz)

**Streaming architecture** (`src/modules/semg/Semg.cpp`):
1. ADC continuous mode (860 Hz with 4x downsample → 215 Hz)
2. **Streaming task** (Core 1, priority 10) sends binary packets via Bluetooth
3. Butterworth 10-50 Hz bandpass + 60 Hz notch filter applied
4. Binary protocol: 108-byte packets, 50 samples/packet, ~4.3 packets/sec

**Flow**:
1. `Semg::enableStreaming()` → Start ADC continuous mode + streaming task
2. ADC task polls ADS1115, writes to circular buffer (512 samples)
3. `streamingTask()` → Reads samples, filters, batches 50, sends binary packet
4. `Semg::disableStreaming()` → Stop task and ADC

**Safety features**:
- Automatic timeout after 10 minutes (configurable via `STREAMING_TIMEOUT_MINUTES`)
- Buffer overflow protection (drops oldest samples)
- Bluetooth disconnect detection
