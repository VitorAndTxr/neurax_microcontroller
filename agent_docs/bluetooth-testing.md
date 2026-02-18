# Bluetooth Protocol Testing

Use `src/modules/message_handler/sample_messages.json` for reference.

## Testing FES Sessions

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

## Testing sEMG Streaming (v3.0+ Fixed 215 Hz)

**Streaming test sequence** (simplified from v2.x — no config needed):

1. **Start streaming**:
```json
{"cd":11,"mt":"x"}
```

**ACK response:**
```json
{"cd":11,"mt":"a"}
```

2. **Receive streaming data** (binary packets, auto-sent at 215 Hz):
   - Binary packets: `[0xAA][0x0D][timestamp:4][count:2][samples:100]`
   - 108 bytes/packet, ~4.3 packets/sec
   - Each sample is `int16_t` (1 LSB = 1 mV)

3. **Stop streaming**:
```json
{"cd":12,"mt":"x"}
```

**ACK response:**
```json
{"cd":12,"mt":"a"}
```

**Expected streaming logs**:
```
[MSG] === Received data ===
[MSG] --->
[MSG] {"cd":11,"mt":"x"}
[MSG] SEMG_STREAMING::START_STREAM (fixed 215 Hz)
[sEMG] Enabling streaming...
[sEMG] Streaming task created successfully
[sEMG] Streaming task started
[MSG] Sending ACK for message code 11

[MSG] === Received data ===
[MSG] --->
[MSG] {"cd":12,"mt":"x"}
[MSG] SEMG_STREAMING::STOP_STREAM
[sEMG] Disabling streaming...
[sEMG] Streaming task finished
[MSG] Sending ACK for message code 12
```

## Legacy Testing (v1.x-v2.x JSON Streaming)

> **Note**: Config message (code 14) was **removed in v3.0.0**. The following is for reference only.

1. Configure: `{"cd":14,"mt":"w","bd":{"rate":100,"type":"filtered"}}` → ACK
2. Start: `{"cd":11,"mt":"x"}` → ACK
3. Receive JSON: `{"cd":13,"mt":"w","bd":{"t":12345,"v":[23.4,25.1,...]}}`
4. Stop: `{"cd":12,"mt":"x"}` → ACK

**Important notes**:
- Streaming automatically stops after 10 minutes or if Bluetooth disconnects
- Cannot run streaming and FES session simultaneously (shared timer)
- Buffer holds 300 samples; overflow drops oldest data
- Bandwidth: 464 B/s at 215 Hz (48% of 9600 baud — safe margin)
