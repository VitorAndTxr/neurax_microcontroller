# Complete Bluetooth Communication Protocol - NeuroEstimulator

Comprehensive reference for all Bluetooth communication protocols used in the NeuroEstimulator device, including command/control (JSON) and real-time streaming (binary).

**Document Version**: 2.0
**Firmware Version**: v3.1.0+
**Date**: 2025-10-16
**Status**: Production Ready

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Connection Establishment](#connection-establishment)
3. [Dual Protocol System](#dual-protocol-system)
4. [JSON Command/Control Protocol](#json-commandcontrol-protocol)
5. [Binary Streaming Protocol](#binary-streaming-protocol)
6. [Complete Message Code Reference](#complete-message-code-reference)
7. [Security and Authentication](#security-and-authentication)
8. [Error Handling](#error-handling)
9. [Message Validation](#message-validation)
10. [Timeout and State Management](#timeout-and-state-management)
11. [Testing Guide](#testing-guide)
12. [Troubleshooting](#troubleshooting)

---

## Architecture Overview

### Multi-Core Communication Architecture

```
┌─────────────────────────────────────────────────────────┐
│              Mobile App / Client                         │
│              (HTTPS or SPP)                              │
└────────────────────┬────────────────────────────────────┘
                     │
            ┌────────▼─────────┐
            │   Bluetooth SPP   │
            │   9600 baud       │
            │   (or 115200)     │
            └────────┬──────────┘
                     │
    ┌────────────────▼────────────────┐
    │    ESP32 Dual-Core (UART2)      │
    ├──────────────┬──────────────────┤
    │              │                  │
    │ Core 1       │   Core 0         │
    │ Message      │   Session        │
    │ Handler      │   Management     │
    │ • JSON Parse │   • ADC Sampling │
    │ • Routing    │   • sEMG Signal  │
    │ • Binary Enc │   • Trigger Detect
    │              │                  │
    └──────────────┴──────────────────┘
            │              │
            ▼              ▼
    ┌─────────────┐ ┌──────────────┐
    │ Bluetooth TX│ │ Sensors      │
    │ • Commands  │ │ • ADC        │
    │ • Data      │ │ • IMU        │
    └─────────────┘ │ • Battery    │
                    └──────────────┘
```

### Protocol Stack

```
Layer 7 (Application)
├─ Session Management
├─ Data Streaming
├─ Device Status
└─ Diagnostics

Layer 6 (Presentation)
├─ JSON Serialization (Commands)
├─ Binary Encoding (Streaming)
└─ Message Formatting

Layer 5 (Session)
├─ Message Sequencing
├─ Acknowledgment Handling
└─ Timeout Management

Layer 4 (Transport)
└─ UART2 (9600/115200 baud)

Layer 3 (Physical)
└─ Bluetooth SPP (HC-05/HC-06)
```

---

## Connection Establishment

### Phase 1: Bluetooth Pairing

**Time**: ~2-5 seconds
**Operating System**: Both mobile app and ESP32

```
Mobile Device                         NeuroEstimulator
    │                                      │
    │◄─ Bluetooth Discovery ───────────────│
    │   (Discoverable: 120 seconds)        │
    │                                      │
    ├─ Pairing Request ──────────────────▶│
    │                                      │
    │◄─ Pairing Confirmation ─────────────│
    │   (Default PIN: 1234)                │
    │                                      │
    └─ Paired & Bonded ──────────────────▶│
        (Credentials saved)                │
```

**Device Name**: `NeuroEstimulator`
**Default PIN**: `1234` (user-configurable in firmware)
**Service UUID**: `0x1101` (Serial Port Profile)
**Baud Rate**: `9600` (default) or `115200` (optional upgrade)

### Phase 2: SPP Connection

**Time**: <500ms

```
Mobile App                            ESP32 UART2
    │                                   │
    ├─ Open SPP Socket ──────────────▶ │
    │   (RFCOMM Channel)                │
    │                                   │
    │◄─ Connection Accepted ─────────── │
    │   (STATUS_PIN = HIGH)             │
    │   (LED_POWER = ON)                │
    │                                   │
    └─ Ready for Communication ────────▶│
        (9600 baud, 8N1)                │
```

**Parameters**:
- Baud Rate: 9600 or 115200
- Data Bits: 8
- Stop Bits: 1
- Parity: None
- Flow Control: None
- Terminator: Null byte (`\0`) for JSON messages

### Phase 3: Device Initialization

After successful SPP connection, the device is ready to receive commands. No authentication required for basic commands.

```
Device State after Connection:
├─ Bluetooth: CONNECTED
├─ Message Handler: READY (awaiting commands)
├─ ADC: IDLE (ready for sampling)
├─ Session: IDLE (no active session)
├─ Streaming: DISABLED
└─ FES: SAFE (no stimulation active)
```

---

## Dual Protocol System

### Protocol Selection Strategy

The NeuroEstimulator uses **two complementary protocols**:

| Aspect | JSON (Command/Control) | Binary (Streaming) |
|--------|------------------------|-------------------|
| **Purpose** | Device configuration and control | High-speed data transmission |
| **Use Cases** | Start/stop session, set parameters, read status | Continuous sEMG data @ 215 Hz |
| **Message Format** | Human-readable text | Optimized binary structure |
| **Bandwidth** | ~50-200 bytes/message | 108 bytes @ 4.3 Hz = 464 bytes/s |
| **Latency** | <100ms | ~230ms |
| **Error Detection** | JSON validation | Magic byte + checksum |
| **Mutual Exclusion** | Cannot run session AND streaming simultaneously (shared timer) | - |

### Protocol Interleaving Example

```
Timeline:

T+0ms:    Mobile → Device: {"cd":7,"mt":"w",...}       (Configure FES - JSON)
T+50ms:   Device → Mobile: {"cd":7,"mt":"a"}           (ACK - JSON)
T+100ms:  Mobile → Device: {"cd":2,"mt":"x"}           (Start Session - JSON)
T+150ms:  Device → Mobile: {"cd":2,"mt":"a"}           (ACK - JSON)
T+200ms:  Device → Mobile: {"cd":8,"mt":"w",...}       (Status - JSON)
T+250ms:  Mobile → Device: {"cd":11,"mt":"x"}          (Start Streaming - JSON)
T+300ms:  Device → Mobile: {"cd":11,"mt":"a"}          (ACK - JSON)
T+400ms:  Device → Mobile: [Binary Packet 1]           (Streaming - BINARY)
T+632ms:  Device → Mobile: [Binary Packet 2]           (Streaming - BINARY)
T+864ms:  Device → Mobile: [Binary Packet 3]           (Streaming - BINARY)
T+1096ms: Device → Mobile: [Binary Packet 4]           (Streaming - BINARY)
...
T+5000ms: Mobile → Device: {"cd":12,"mt":"x"}          (Stop Streaming - JSON)
T+5050ms: Device → Mobile: {"cd":12,"mt":"a"}          (ACK - JSON)
T+5100ms: Mobile → Device: {"cd":3,"mt":"x"}           (Stop Session - JSON)
T+5150ms: Device → Mobile: {"cd":3,"mt":"a"}           (ACK - JSON)
```

---

## JSON Command/Control Protocol

### Message Structure

All JSON messages follow a standardized format:

```json
{
  "cd": <integer 1-14>,     // Message code (identifies command type)
  "mt": "<method>",         // Method: 'r' (read), 'w' (write), 'x' (execute), 'a' (ack)
  "bd": <optional object>   // Body: command-specific data (may be omitted)
}
```

### Message Methods (mt field)

| Method | Meaning | Sender | Description |
|--------|---------|--------|-------------|
| `r` | Read | Client → Device | Request data without executing action |
| `w` | Write | Client → Device OR Device → Client | Send data or notify of event |
| `x` | Execute | Client → Device | Trigger an action/command |
| `a` | Acknowledge | Device → Client | Confirm receipt (response to r/w/x) |

### Message Codes (cd field)

Complete reference for all message codes:

| Code | Name | Method | Direction | Firmware | Purpose |
|------|------|--------|-----------|----------|---------|
| 1 | Gyroscope | x/a | ↔ | v1.0+ | Read IMU accelerometer/gyroscope |
| 2 | Session Start | x/a | ← | v1.0+ | Start therapeutic FES session |
| 3 | Session Stop | x/a | ← | v1.0+ | Stop active FES session |
| 4 | Session Pause | x/a | ← | v2.0+ | Pause without ending session |
| 5 | Session Resume | x/a | ← | v2.0+ | Resume paused session |
| 6 | Single Stimulus | x/a | ← | v2.0+ | Trigger one FES stimulus manually |
| 7 | Set Parameters | w/a | ← | v1.0+ | Configure FES parameters |
| 8 | Session Status | w | → | v1.0+ | Send session execution status |
| 9 | Trigger Detected | w | → | v1.0+ | Notify trigger detected in FES session |
| 10 | Battery Status | r/a | ↔ | v2.5+ | Request battery voltage levels |
| 11 | Start Streaming | x/a | ← | v3.0+ | Start real-time sEMG streaming |
| 12 | Stop Streaming | x/a | ← | v3.0+ | Stop real-time sEMG streaming |
| 13 | Streaming Data | w | → | v3.0+ | Send sEMG data packet (binary or JSON) |
| 14 | Stream Config | w/a | ← | v2.5-v3.0 | Configure streaming parameters (DEPRECATED in v3.1+) |

---

## Complete Message Code Reference

### Code 1: Gyroscope Reading

**Purpose**: Read MPU6050 6-axis IMU data (accelerometer + gyroscope)

**Request**:
```json
{
  "cd": 1,
  "mt": "x"
}
```

**Response**:
```json
{
  "cd": 1,
  "mt": "a",
  "bd": {
    "ax": 0.125,        // X-axis acceleration (g)
    "ay": -0.032,       // Y-axis acceleration (g)
    "az": 0.980,        // Z-axis acceleration (g)
    "gx": 2.5,          // X-axis gyroscope (°/s)
    "gy": -1.8,         // Y-axis gyroscope (°/s)
    "gz": 0.4,          // Z-axis gyroscope (°/s)
    "temp": 24.5        // Temperature (°C)
  }
}
```

**Details**:
- Sampling Rate: 100 Hz
- Precision: ±8g acceleration, ±250°/s gyroscope
- Updates: Real-time

### Code 2: Start FES Session

**Purpose**: Begin closed-loop FES therapeutic session with trigger detection

**Request**:
```json
{
  "cd": 2,
  "mt": "x"
}
```

**Response** (Acknowledge):
```json
{
  "cd": 2,
  "mt": "a"
}
```

**Preconditions**:
- FES parameters must be configured (code 7)
- Amplitude > 0
- Frequency > 0
- No streaming active

**Session Flow**:
1. Device enables sEMG sampling (860 Hz ADC)
2. Monitors sEMG signal against configured threshold
3. On trigger detection: sends code 9, executes FES stimulus
4. Updates session status (code 8) every stimulus
5. Session continues until code 3 (stop) received or timeout

### Code 3: Stop FES Session

**Purpose**: Terminate active FES session immediately

**Request**:
```json
{
  "cd": 3,
  "mt": "x"
}
```

**Response** (Acknowledge):
```json
{
  "cd": 3,
  "mt": "a"
}
```

**Effects**:
- Stops sEMG sampling
- Disables trigger detection
- Sends final session status
- Returns to IDLE state

### Code 4: Pause Session (v2.0+)

**Purpose**: Temporarily suspend session without terminating

**Request**:
```json
{
  "cd": 4,
  "mt": "x"
}
```

**Response**:
```json
{
  "cd": 4,
  "mt": "a"
}
```

**Behavior**:
- Stops FES stimulation
- Maintains session state
- Can be resumed with code 5
- Session timeout still applies

### Code 5: Resume Session (v2.0+)

**Purpose**: Resume previously paused session

**Request**:
```json
{
  "cd": 5,
  "mt": "x"
}
```

**Response**:
```json
{
  "cd": 5,
  "mt": "a"
}
```

**Conditions**:
- Must have active paused session
- Resumes from exact pause point

### Code 6: Single Stimulus (v2.0+)

**Purpose**: Manually trigger one FES stimulus without waiting for sEMG detection

**Request**:
```json
{
  "cd": 6,
  "mt": "x"
}
```

**Response**:
```json
{
  "cd": 6,
  "mt": "a"
}
```

**Parameters Used**:
- Amplitude, frequency, pulse width from last code 7 command
- Single stimulus delivery (not continuous)

### Code 7: Set FES Parameters

**Purpose**: Configure all FES stimulation parameters before session start

**Request**:
```json
{
  "cd": 7,
  "mt": "w",
  "bd": {
    "a": 3.0,         // Amplitude (0-15V)
    "f": 38.0,        // Frequency (1-100 Hz)
    "pw": 12.0,       // Pulse width (1-100 ms)
    "df": 50,         // Difficulty/Threshold (1-100%)
    "pd": 5           // Pulse duration (1-60 sec)
  }
}
```

**Response**:
```json
{
  "cd": 7,
  "mt": "a"
}
```

**Parameter Details**:

| Parameter | Range | Unit | Default | Description |
|-----------|-------|------|---------|-------------|
| **a** (Amplitude) | 0-15 | Volts | 3.0 | H-bridge output voltage for biphasic pulse |
| **f** (Frequency) | 1-100 | Hz | 38 | Stimulation pulse rate |
| **pw** (Pulse Width) | 1-100 | ms | 12 | Duration of each biphasic phase |
| **df** (Difficulty) | 1-100 | % | 50 | sEMG trigger threshold sensitivity (% of max) |
| **pd** (Pulse Duration) | 1-60 | sec | 5 | Total stimulation duration per trigger |

**Validation**:
- All values must be within specified ranges
- Non-numeric values cause error
- Amplitude verified against battery voltage
- Frequency × Pulse Width must be ≤ period (e.g., 100 Hz @ 12ms requires 10ms minimum)

### Code 8: Session Status

**Purpose**: Send real-time session execution metrics to mobile app

**Sent By**: Device → Client (automatic, after each stimulus)

**Message**:
```json
{
  "cd": 8,
  "mt": "w",
  "bd": {
    "parameters": {
      "a": 3.0,
      "f": 38.0,
      "pw": 12.0,
      "df": 50,
      "pd": 5
    },
    "status": {
      "csa": 45,              // Complete stimuli amount (successful)
      "isa": 3,               // Interrupted stimuli amount (failed)
      "tlt": 156000,          // Time of last trigger (ms since session start)
      "sd": 300000,           // Session duration (ms elapsed)
      "battery_main": 4.2,    // Main battery voltage (V)
      "battery_stim": 4.1,    // Stimulation battery voltage (V)
      "state": "active"       // Session state: active, paused, complete
    }
  }
}
```

**Frequency**: After each FES stimulus delivery

### Code 9: Trigger Detected

**Purpose**: Notify client that sEMG threshold exceeded (muscle activated)

**Sent By**: Device → Client (real-time during session)

**Message**:
```json
{
  "cd": 9,
  "mt": "w"
}
```

**Timing**: Sent <100ms after threshold crossing
**Frequency**: Variable (depends on session triggers)

### Code 10: Battery Status (v2.5+)

**Purpose**: Request current battery voltage levels

**Request**:
```json
{
  "cd": 10,
  "mt": "r"
}
```

**Response**:
```json
{
  "cd": 10,
  "mt": "a",
  "bd": {
    "battery_main_voltage": 4.2,      // Main system battery (V)
    "battery_main_percent": 85,       // Estimated charge (%)
    "battery_stim_voltage": 4.1,      // FES stimulation battery (V)
    "battery_stim_percent": 82,       // Estimated charge (%)
    "low_battery_main": false,        // Main battery critical (<3.0V)
    "low_battery_stim": false,        // Stim battery critical (<3.0V)
    "charger_connected": false        // USB charging active
  }
}
```

### Code 11: Start Streaming

**Purpose**: Begin continuous real-time sEMG data transmission (215 Hz, binary protocol)

**Request**:
```json
{
  "cd": 11,
  "mt": "x"
}
```

**Response** (Acknowledge):
```json
{
  "cd": 11,
  "mt": "a"
}
```

**After ACK**: Device switches to binary protocol mode and sends packets continuously.

**Important**: See [Binary Streaming Protocol](#binary-streaming-protocol) section for packet structure.

**Frequency**: ~4.3 packets/second (50 samples/packet @ 215 Hz)
**Bandwidth**: 464 bytes/s (48% @ 9600 baud)

### Code 12: Stop Streaming

**Purpose**: Terminate continuous sEMG streaming

**Request**:
```json
{
  "cd": 12,
  "mt": "x"
}
```

**Response** (Acknowledge):
```json
{
  "cd": 12,
  "mt": "a"
}
```

**Effects**:
- Stops binary packet transmission
- Returns to JSON command mode
- Device waits for next command

### Code 13: Streaming Data

**Purpose**: Transmit sEMG data packets (binary or JSON format)

**Binary Format**: See [Binary Streaming Protocol](#binary-streaming-protocol)

**Alternative JSON Format** (v2.x compatibility):
```json
{
  "cd": 13,
  "mt": "w",
  "bd": {
    "t": 12345,                  // Timestamp (ms since session/stream start)
    "v": [2482, 2483, 2484, ...] // Array of int16 samples (50 per packet)
  }
}
```

**Note**: Firmware v3.0+ always uses binary format. JSON format deprecated.

### Code 14: Stream Configuration (DEPRECATED v3.1+)

**Deprecated**: Firmware v3.1+ uses fixed 215 Hz configuration.

**Historical Reference** (v2.5-v3.0):
```json
{
  "cd": 14,
  "mt": "w",
  "bd": {
    "rate": 215,        // Hz (10-200 range)
    "type": "filtered"  // "raw", "filtered", or "rms"
  }
}
```

---

## Binary Streaming Protocol

For comprehensive binary protocol details, see: [`bluetooth-protocol.md`](./bluetooth-protocol.md)

### Quick Reference

**Packet Structure** (108 bytes):
```c
struct BinaryPacket {
    uint8_t  magic;         // 0xAA (packet start marker)
    uint8_t  code;          // 0x0D (message code 13)
    uint32_t timestamp;     // millis() since session/stream start
    uint16_t count;         // Number of samples (always 50)
    int16_t  samples[50];   // sEMG data (-4096 to +4096, mV precision)
} __attribute__((packed));  // 108 bytes total
```

**Byte Order**: Little-endian (LSB first)
**Value Conversion**: `volts = sample_int16 / 1000.0`

**Packet Rate**:
- 215 Hz sampling ÷ 50 samples/packet = 4.3 packets/second
- ~232ms between packets
- 464 bytes/second (48% of 9600 baud)

---

## Security and Authentication

### Communication Security Model

The NeuroEstimulator implements a **device-level security model** (not node-level):

```
┌─────────────────────────────────────────────┐
│   Mobile App / Research Node                │
│   (Handles advanced authentication)         │
└────────────────┬────────────────────────────┘
                 │ HTTPS/TLS
                 │ (Session tokens, encryption)
                 │
    ┌────────────▼─────────────┐
    │  Mobile Bluetooth SPP     │
    │  (Optional pairing)       │
    └────────────┬──────────────┘
                 │ Bluetooth SPP
                 │ (Device-level basic auth)
                 │
    ┌────────────▼────────────────────┐
    │  ESP32 Device                   │
    │  • Message validation           │
    │  • Session timeouts             │
    │  • State machine enforcement    │
    └─────────────────────────────────┘
```

### Device Authentication

**Level 1: Bluetooth Pairing**
- Device Name: "NeuroEstimulator"
- PIN: 1234 (default, user-configurable)
- Method: Legacy pairing (SPP compatible)

**Level 2: Message Validation**
- JSON schema validation
- Parameter range checking
- Command sequence validation

**Level 3: State Machine**
- Commands only accepted in valid states
- Attempted invalid commands return error

### No Runtime Authentication

The NeuroEstimulator does **NOT** implement per-message authentication because:
1. Device-level (local) use only
2. Bluetooth SPP is point-to-point
3. Authentication delegated to research node (Phase 1-4 handshake in backend)

**However**, when submitting session data to research nodes (future Phase 5):
- Mobile app performs HTTPS Phase 1-4 handshake
- Data encrypted with node's public key
- HL7 FHIR payloads signed with researcher certificate

---

## Error Handling

### Error Response Format

**Invalid Command**:
```json
{
  "cd": 999,
  "mt": "a",
  "error": "INVALID_COMMAND_CODE",
  "details": "Command code 999 not recognized"
}
```

**Invalid Parameters**:
```json
{
  "cd": 7,
  "mt": "a",
  "error": "PARAMETER_OUT_OF_RANGE",
  "details": "Amplitude 25V exceeds maximum 15V"
}
```

**State Violation**:
```json
{
  "cd": 2,
  "mt": "a",
  "error": "INVALID_STATE",
  "details": "Cannot start session: streaming active (mutual exclusion)"
}
```

### Error Codes

| Error Code | HTTP Equivalent | Meaning | Recovery |
|------------|-----------------|---------|----------|
| `INVALID_COMMAND_CODE` | 400 Bad Request | Message code not recognized | Check code (1-14) |
| `INVALID_JSON` | 400 Bad Request | JSON parse error | Verify syntax, null terminator |
| `INVALID_STATE` | 409 Conflict | Command not allowed in current state | Check device state |
| `PARAMETER_OUT_OF_RANGE` | 422 Unprocessable Entity | Parameter value outside valid range | Check min/max values |
| `BATTERY_CRITICAL` | 503 Service Unavailable | Battery voltage too low | Charge device |
| `TIMEOUT` | 504 Gateway Timeout | No response within timeout period | Reconnect |
| `BUFFER_OVERFLOW` | 507 Insufficient Storage | Internal buffer full | Reduce data rate |

### Timeout Handling

| Scenario | Timeout | Action |
|----------|---------|--------|
| Command ACK timeout | 5 seconds | Retry or reconnect |
| Streaming inactivity | 10 minutes | Auto-stop streaming |
| Session duration exceeded | Configured pd value | Auto-stop session |
| Bluetooth disconnect | 30 seconds | Reconnect required |

---

## Message Validation

### JSON Schema Validation

All JSON messages must validate against this schema:

```json
{
  "type": "object",
  "required": ["cd", "mt"],
  "properties": {
    "cd": {
      "type": "integer",
      "minimum": 1,
      "maximum": 14
    },
    "mt": {
      "type": "string",
      "enum": ["r", "w", "x", "a"]
    },
    "bd": {
      "type": "object",
      "properties": {
        "a": {"type": "number", "minimum": 0, "maximum": 15},
        "f": {"type": "number", "minimum": 1, "maximum": 100},
        "pw": {"type": "number", "minimum": 1, "maximum": 100},
        "df": {"type": "integer", "minimum": 1, "maximum": 100},
        "pd": {"type": "integer", "minimum": 1, "maximum": 60}
      }
    }
  }
}
```

### Validation Checklist

Before sending any command:

```
□ Message is valid JSON
□ Contains "cd" field (1-14)
□ Contains "mt" field ("r", "w", "x", or "a")
□ bd object (if present) has correct structure
□ All numeric values within range
□ Message ends with null byte (\0)
□ Message length < 512 bytes
□ Device is in correct state for command
□ Bluetooth connection is active
```

### Common Validation Errors

| Problem | Validation Issue | Solution |
|---------|-----------------|----------|
| No null terminator | Incomplete message | Add `\0` at end |
| Extra whitespace | JSON syntax | Remove spaces around colons |
| Wrong quotes | JSON syntax | Use double quotes only |
| Amplitude > 15 | Out of range | Check max value is 15V |
| Frequency = 0 | Out of range | Frequency must be 1-100 Hz |
| Battery < 3.0V | Hardware state | Charge device before use |

---

## Timeout and State Management

### Session State Machine

```
┌──────────┐
│  IDLE    │◄──────────────────┐
└────┬─────┘                   │
     │ Code 2 (Start)          │
     ▼                         │
┌──────────┐                   │
│ ACTIVE   │                   │
└────┬─────┘                   │
     │                         │
     ├─ Code 4 (Pause)────┐    │
     │                   ▼    │
     │              ┌────────┐│
     │              │ PAUSED ││
     │              └────────┘│
     │                   ▲    │
     └─ Code 5 (Resume)─┘    │
     │                         │
     └─ Code 3 (Stop)──────────┘
     │                         │
     └─ Timeout after pd sec──┘
```

### Streaming State Machine

```
┌──────────┐
│  IDLE    │
└────┬─────┘
     │ Code 11 (Start)
     ▼
┌──────────────┐
│ STREAMING    │
│ (Binary Mode)│
└────┬─────────┘
     │
     ├─ Code 12 (Stop)──────┐
     │                      │
     └─ Timeout 10 min──────┼────┐
                            │    │
                       ┌────▼────▼────┐
                       │ IDLE (JSON)  │
                       └──────────────┘
```

### State Constraints

**Cannot Occur Simultaneously**:
- Active FES session + Streaming active (shared timer resource)
- Multiple active sessions
- Paused session + New session start

**Auto-Cleanup**:
- Session timeout: Auto-stop after `pd` seconds
- Streaming timeout: Auto-stop after 10 minutes
- Bluetooth disconnect: All states reset to IDLE

---

## Testing Guide

### Pre-Test Checklist

```
Hardware:
□ ESP32 powered on
□ Bluetooth module paired with mobile device
□ SPP connection established
□ Serial connection OK (if using debug terminal)

Mobile App:
□ Bluetooth connected to "NeuroEstimulator"
□ Serial terminal or BT app ready
□ Baud rate set to 9600
□ Message buffer cleared

Device State:
□ LED_POWER on (connected)
□ LED_TRIGGER off (no trigger)
□ LED_FES off (no stimulation)
□ No error indicators
```

### Test Case 1: Connection Establishment

**Steps**:
1. Pair mobile device with ESP32 (name: "NeuroEstimulator", pin: 1234)
2. Open SPP connection from mobile app
3. Verify LED_POWER lights up
4. Send: `{"cd":10,"mt":"r"}\0` (read battery)
5. Expect: Battery status JSON response within 1 second

**Expected Output**:
```json
{
  "cd": 10,
  "mt": "a",
  "bd": {
    "battery_main_voltage": 4.2,
    "battery_main_percent": 85,
    "battery_stim_voltage": 4.1,
    "battery_stim_percent": 82,
    "low_battery_main": false,
    "low_battery_stim": false
  }
}
```

### Test Case 2: FES Session

**Steps**:
1. Configure FES parameters:
   ```json
   {"cd":7,"mt":"w","bd":{"a":3.0,"f":38,"pw":12,"df":50,"pd":5}}\0
   ```
2. Expect ACK: `{"cd":7,"mt":"a"}`
3. Start session: `{"cd":2,"mt":"x"}\0`
4. Expect ACK: `{"cd":2,"mt":"a"}`
5. Wait for triggers or manual stimulus (code 6)
6. Observe status updates (code 8)
7. Stop session: `{"cd":3,"mt":"x"}\0`
8. Expect ACK: `{"cd":3,"mt":"a"}`

**Expected Behavior**:
- Session starts immediately
- Status updates sent after each stimulus
- Session stops cleanly
- No errors

### Test Case 3: Streaming

**Steps**:
1. Start streaming: `{"cd":11,"mt":"x"}\0`
2. Expect ACK: `{"cd":11,"mt":"a"}` (JSON)
3. Wait 100ms, then expect binary packets
4. Collect packets for 5 seconds
5. Stop streaming: `{"cd":12,"mt":"x"}\0`
6. Expect ACK: `{"cd":12,"mt":"a"}`

**Validation**:
- First packet within 200ms of start command
- Packets arrive every ~232ms (±50ms)
- No corrupted packets (magic byte always 0xAA)
- Timestamps incrementing
- 50 samples per packet
- Values between -4096 and +4096

### Test Case 4: Error Handling

**Steps**:

**Test 4a: Invalid command code**
```json
{"cd":99,"mt":"x"}\0
```
Expected: `{"cd":99,"mt":"a","error":"INVALID_COMMAND_CODE"}`

**Test 4b: Parameter out of range**
```json
{"cd":7,"mt":"w","bd":{"a":20,"f":38,"pw":12,"df":50,"pd":5}}\0
```
Expected: `{"cd":7,"mt":"a","error":"PARAMETER_OUT_OF_RANGE","details":"Amplitude 20V exceeds maximum 15V"}`

**Test 4c: State violation (session + streaming)**
```
1. Start session: {"cd":2,"mt":"x"}
2. Try start streaming: {"cd":11,"mt":"x"}
```
Expected: `{"cd":11,"mt":"a","error":"INVALID_STATE",...}`

---

## Troubleshooting

### Issue: No Response to Commands

**Symptoms**: Send command, receive nothing for >5 seconds

**Diagnosis Checklist**:
1. Is Bluetooth connected? (Check LED_POWER status)
2. Is message null-terminated? (Add `\0` at end)
3. Is message valid JSON? (Use JSON validator)
4. Is device in valid state? (Check state machine)
5. Is buffer full? (Try simpler command first)

**Solution**:
```
1. Reconnect Bluetooth (SPP disconnect/reconnect)
2. Send simple read: {"cd":10,"mt":"r"}\0
3. If still no response, restart ESP32
4. Enable DEBUG=true in platformio.ini and check serial output
```

### Issue: Streaming Packets Corrupted

**Symptoms**: Cannot parse binary packets, frequent desync

**Diagnosis**:
1. Check packet starts with 0xAA
2. Verify little-endian byte order
3. Monitor timestamp gaps (expect ~232ms)
4. Check buffer size (should be ≥108 bytes)

**Solution**:
```
1. Lower Bluetooth baud to 9600 (if using 115200)
2. Reduce streaming duration to isolate issue
3. Check for interference (move away from WiFi router)
4. Increase Bluetooth MTU if supported
```

### Issue: Session Timeout During Use

**Symptoms**: Session stops unexpectedly after N seconds

**Cause**: Session duration (`pd` parameter) exceeded

**Solution**:
- Verify `pd` parameter sent with code 7
- Increase `pd` value before starting session
- Maximum `pd` is 60 seconds

### Issue: Mutual Exclusion Error

**Symptoms**: Cannot start session when streaming active (or vice versa)

**Cause**: Shared timer resource - only one can use time-critical operations

**Solution**:
```
1. Stop streaming: {"cd":12,"mt":"x"}\0
2. Wait for ACK
3. Then start session: {"cd":2,"mt":"x"}\0
```

### Issue: Battery Critical Error

**Symptoms**: All stimulation disabled, error "BATTERY_CRITICAL"

**Cause**: Battery voltage < 3.0V (hardware safety)

**Solution**:
```
1. Charge device immediately
2. Check both battery connectors
3. Verify battery voltage with: {"cd":10,"mt":"r"}\0
4. Test after voltage > 3.5V
```

---

## Performance Reference

### Latency Measurements

| Operation | Typical | Max | Notes |
|-----------|---------|-----|-------|
| Command ACK | 50-100ms | 500ms | JSON processing |
| Streaming packet | 200-300ms | 500ms | 50 samples @ 215 Hz |
| FES response (trigger→stimulus) | 50-100ms | 200ms | Real-time on Core 0 |
| Bluetooth SPP handshake | 200-500ms | 1s | Once per session |
| Battery read | 100-200ms | 500ms | ADC sampling |
| Gyroscope read | 100-200ms | 500ms | I2C communication |

### Bandwidth Allocation

```
@ 9600 baud:
├─ Theoretical: 9600 bits/s = 1200 bytes/s
├─ With overhead (start/stop): ~1000 bytes/s effective
│
└─ Usage:
    ├─ Streaming: 464 bytes/s (46%)
    ├─ Status updates: 50 bytes/s (5%)
    ├─ Trigger notifications: 10 bytes/s (1%)
    └─ Headroom: 476 bytes/s (48%) available
```

Upgrade to 115200 baud for:
- Higher streaming rates (>30 Hz useful data)
- Simultaneous streaming + commands
- Lower latency requirements

---

## Firmware Version Compatibility

| Feature | v1.0 | v2.0 | v2.5 | v3.0 | v3.1 |
|---------|------|------|------|------|------|
| Basic FES | ✓ | ✓ | ✓ | ✓ | ✓ |
| Session Control | ✓ | ✓ | ✓ | ✓ | ✓ |
| Pause/Resume | - | ✓ | ✓ | ✓ | ✓ |
| Battery Status | - | - | ✓ | ✓ | ✓ |
| JSON Streaming | ✓ | ✓ | ✓ | ✓ | ✗ |
| Binary Streaming | - | - | - | ✓ | ✓ |
| Stream Config (14) | - | - | ✓ | ✓ | ✗ |
| Fixed 215 Hz | - | - | - | ✓ | ✓ |
| Improved ACK | - | - | - | - | ✓ |

---

## References

- **Binary Streaming Details**: [`bluetooth-protocol.md`](./bluetooth-protocol.md)
- **Quick Reference**: [`streaming-protocol.md`](./streaming-protocol.md)
- **Integration Guide**: `../INTEGRATION_GUIDE.md`
- **Device Development**: `../../CLAUDE.md`

---

**Document Version**: 2.0
**Last Updated**: 2025-10-16
**Status**: Production
**Maintained By**: PRISM Development Team
**Firmware Target**: v3.1.0+
