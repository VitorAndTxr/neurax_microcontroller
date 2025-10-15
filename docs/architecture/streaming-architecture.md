# sEMG Streaming Flowchart - Visual Reference

**Quick Reference Guide for Developers**

---

## Complete Data Flow (End-to-End)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        MOBILE APPLICATION                                │
└────────────────┬────────────────────────────────────────────────────────┘
                 │
                 │ 1. Configure Streaming
                 │ {"cd":14,"mt":"w","bd":{"rate":100,"type":"filtered"}}
                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                     ESP32 - CORE 1 (Communication)                       │
│                                                                           │
│  MessageHandler::handleIncomingMessages()                                │
│  ├─ Bluetooth::readData() ──────────────────────────┐                   │
│  │  └─ Read from UART2 (9600 baud)                  │                   │
│  │                                                    │                   │
│  ├─ MessageHandler::interpretMessage()              │                   │
│  │  └─ deserializeJson() → Parse message            │                   │
│  │                                                    │                   │
│  ├─ case SEMG_STREAMING::CONFIG_STREAM:             │                   │
│  │  └─ MessageHandler::handleStreamingConfigMessage()│                   │
│  │     ├─ Extract rate (100 Hz)                     │                   │
│  │     ├─ Extract type ("filtered")                 │                   │
│  │     └─ Semg::configureStreaming(100, "filtered") │                   │
│  │        ├─ Parse type → STREAMING_FILTERED        │                   │
│  │        ├─ SemgFilter::updateSamplingRate(10ms)   │                   │
│  │        │  └─ Compute Butterworth coefficients    │                   │
│  │        ├─ SemgFilter::resetState() ✅            │                   │
│  │        └─ streaming_config.rate = 100            │                   │
│  │                                                    │                   │
│  └─ MessageHandler::sendAck(14) ────────────────────┼───────────────┐  │
│     └─ {"cd":14,"mt":"a"}                           │               │  │
└─────────────────────────────────────────────────────┼───────────────┼──┘
                 ▲                                     │               │
                 │ ACK                                 │ Bluetooth TX  │
                 │                                     │               │
┌────────────────┴────────────────────────────────────┴───────────────┴──┐
│                        MOBILE APPLICATION                                │
│  ✅ Configuration confirmed                                             │
└────────────────┬────────────────────────────────────────────────────────┘
                 │
                 │ 2. Start Streaming
                 │ {"cd":11,"mt":"x"}
                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                     ESP32 - CORE 1 (Communication)                       │
│                                                                           │
│  MessageHandler::interpretMessage()                                      │
│  ├─ case SEMG_STREAMING::START_STREAM:                                  │
│  │  └─ Semg::enableStreaming()                                          │
│  │     ├─ buffer_write_index = 0                                        │
│  │     ├─ buffer_read_index = 0                                         │
│  │     ├─ streaming_active = true ◄──────────────┐                     │
│  │     │                                           │                     │
│  │     ├─ startStreamingSamplingTimer(10ms) ──────┼─────────────────┐  │
│  │     │  └─ xTimerCreate() → samplingTimer      │                 │  │
│  │     │                                           │                 │  │
│  │     └─ xTaskCreatePinnedToCore() ──────────────┼────────┐        │  │
│  │        ├─ Task: Semg::streamingTask           │        │        │  │
│  │        ├─ Priority: 15                         │        │        │  │
│  │        ├─ Core: 1                              │        │        │  │
│  │        └─ Stack: 4096 bytes                    │        │        │  │
│  │                                                 │        │        │  │
│  └─ MessageHandler::sendAck(11) ──────────────────┼────────┼────────┼──┐
│     └─ {"cd":11,"mt":"a"}                         │        │        │  │
└─────────────────────────────────────────────────┬─┼────────┼────────┼──┘
                 ▲                                 │ │        │        │
                 │ ACK                             │ │        │        │
                 └─────────────────────────────────┼─┘        │        │
                                                   │          │        │
┌──────────────────────────────────────────────────┼──────────┼────────┼──┐
│                ESP32 - CORE 0 (Time-Critical)    │          │        │  │
│                                                   │          │        │  │
│  ┌─────────────────────────────────────────────┐ │          │        │  │
│  │ Timer ISR (every 10 ms @ 100 Hz)            │◄┼──────────┘        │  │
│  │ Semg::samplingCallback(TimerHandle_t xTimer)│ │                   │  │
│  │                                              │ │                   │  │
│  │  if (streaming_active) ◄─────────────────────┼─┘                   │  │
│  │  {                                           │                     │  │
│  │    // Step 1: Read ADC                       │                     │  │
│  │    float value = Adc::getValue(SEMG_ADC_PIN) │                     │  │
│  │    ├─ I2C mutex lock (i2cMutex)              │                     │  │
│  │    ├─ ADS1115 read (16-bit, ~5ms) ✅         │                     │  │
│  │    └─ I2C mutex unlock                       │                     │  │
│  │                                              │                     │  │
│  │    // Step 2: Apply filter                   │                     │  │
│  │    float processed = applyStreamingFilter(value)                   │  │
│  │    ├─ case STREAMING_RAW: return value       │                     │  │
│  │    ├─ case STREAMING_FILTERED:               │                     │  │
│  │    │  └─ SemgFilter::filter(value)           │                     │  │
│  │    │     └─ Butterworth IIR (10-50 Hz)       │                     │  │
│  │    └─ case STREAMING_RMS: return fabs(value) │                     │  │
│  │                                              │                     │  │
│  │    // Step 3: Convert to int16_t             │                     │  │
│  │    int16_t int_value = floatToInt16(processed)                     │  │
│  │    └─ Clamp to ±4096 range                   │                     │  │
│  │                                              │                     │  │
│  │    // Step 4: Write to circular buffer       │                     │  │
│  │    writeToBuffer(int_value)                  │                     │  │
│  │    ├─ portENTER_CRITICAL_ISR() ✅            │                     │  │
│  │    ├─ buffer[write_index] = int_value        │                     │  │
│  │    ├─ write_index = (write_index+1) % 200    │                     │  │
│  │    ├─ Check overflow (write==read?)          │                     │  │
│  │    └─ portEXIT_CRITICAL_ISR()                │                     │  │
│  │  }                                           │                     │  │
│  └──────────────────────────────────────────────┘                     │  │
│       │                                                                 │  │
│       │ Repeat every 10ms (100 Hz)                                     │  │
│       │                                                                 │  │
│       ▼                                                                 │  │
│  ┌─────────────────────────────────────────────┐                      │  │
│  │ Circular Buffer (200 samples)               │                      │  │
│  │ [int16_t array]                             │                      │  │
│  │                                              │                      │  │
│  │ Write Index: 123 ──┐                        │                      │  │
│  │ Read Index:   73 ──┼────────────────────────┼──┐                   │  │
│  │ Available:     50   │                        │  │                   │  │
│  └─────────────────────┼────────────────────────┘  │                   │  │
└────────────────────────┼───────────────────────────┼───────────────────┘  │
                         │                           │                      │
                         │                           │                      │
┌────────────────────────┼───────────────────────────┼──────────────────────┘
│              ESP32 - CORE 1 (Communication)        │
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │ FreeRTOS Task (Priority: 15)               │◄─┘
│  │ Semg::streamingTask(void* parameters)       │
│  │                                              │
│  │  const int interval_ms = 1000 / (rate/50)   │
│  │  // For 100 Hz: 1000 / 2 = 500ms            │
│  │                                              │
│  │  while (streaming_active) {                 │
│  │                                              │
│  │    // Safety timeout check (10 minutes)     │
│  │    if (millis() - start > 600000) {         │
│  │      disableStreaming(); break;             │
│  │    }                                         │
│  │                                              │
│  │    // Check buffer availability             │
│  │    int available = getAvailableSamples()    │
│  │    ├─ portENTER_CRITICAL()                  │
│  │    ├─ snapshot write/read indices           │
│  │    └─ calculate (write-read) % 200          │
│  │                                              │
│  │    if (available >= 50) {                   │
│  │      // Step 1: Read from buffer            │
│  │      int16_t samples[50];                   │
│  │      readStreamingSamples(samples, 50)      │
│  │      ├─ portENTER_CRITICAL()                │
│  │      ├─ Copy 50 samples                     │
│  │      ├─ read_index = (read_index+50) % 200  │
│  │      └─ portEXIT_CRITICAL()                 │
│  │                                              │
│  │      // Step 2: Encode binary packet        │
│  │      sendBinaryStreamingMessage(samples, 50)│
│  │      ├─ Build header (8 bytes)              │
│  │      │  ├─ magic = 0xAA                     │
│  │      │  ├─ message_code = 13                │
│  │      │  ├─ timestamp = millis()             │
│  │      │  └─ sample_count = 50                │
│  │      ├─ Copy samples (100 bytes)            │
│  │      │  └─ memcpy(buffer+8, samples, 100)   │
│  │      └─ Total packet: 108 bytes             │
│  │                                              │
│  │      // Step 3: Send via Bluetooth          │
│  │      Bluetooth::sendRawData(buffer, 108)    │
│  │      ├─ xSemaphoreTake(semaphore_bt, 100ms) │
│  │      ├─ BTSerial.write(buffer, 108)         │
│  │      │  └─ UART2 TX (~11 ms @ 9600 baud)    │
│  │      └─ xSemaphoreGive(semaphore_bt)        │
│  │                                              │
│  │    } else {                                  │
│  │      // Not enough samples, wait            │
│  │      vTaskDelay(interval_ms) // 500ms       │
│  │    }                                         │
│  │  }                                           │
│  └──────────────────────────────────────────────┘
│       │
│       │ Binary packet (108 bytes, every 500ms)
│       ▼
│  ┌──────────────────────────────────────────────┐
│  │ Bluetooth Module (HC-05)                    │
│  │ UART2: 9600 baud, 8N1                       │
│  │                                              │
│  │ Packet structure:                            │
│  │ [0xAA][13][TS:4B][50][DATA:100B]            │
│  │ └─┬─┘ └─┬─┘ └─┬─┘ └┬┘ └────┬────┘          │
│  │   │     │     │     │       │                │
│  │ Magic  Code Time Count  Samples             │
│  └──────────────────────────────────────────────┘
│       │
│       │ SPP (Serial Port Profile)
│       ▼
└─────────────────────────────────────────────────────────────────────────┐
                                                                           │
┌──────────────────────────────────────────────────────────────────────────┘
│                        MOBILE APPLICATION
│
│  Binary Packet Decoder:
│  ├─ Read header (8 bytes)
│  │  ├─ Check magic == 0xAA
│  │  ├─ Verify message_code == 13
│  │  └─ Extract timestamp, sample_count
│  │
│  ├─ Read data (100 bytes)
│  │  └─ int16_t samples[50]
│  │
│  ├─ Convert to voltage
│  │  └─ voltage[i] = samples[i] / 1000.0  // mV to V
│  │
│  └─ Display real-time waveform
│     └─ Update chart (100 samples/sec)
│
└─────────────────────────────────────────────────────────────────────────┘
                 │
                 │ 3. Stop Streaming
                 │ {"cd":12,"mt":"x"}
                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                     ESP32 - MessageHandler                               │
│                                                                           │
│  Semg::disableStreaming()                                                │
│  ├─ streaming_active = false ────────────────────────────┐              │
│  │  (Stops ISR from writing to buffer)                   │              │
│  │                                                         │              │
│  ├─ vTaskDelete(streaming_task_handle) ──────────────────┼──┐           │
│  │  (Kills transmission task)                            │  │           │
│  │                                                         │  │           │
│  └─ Timer automatically stops on next callback            │  │           │
│     (samplingCallback checks streaming_active)            │  │           │
│                                                            │  │           │
│  MessageHandler::sendAck(12)                              │  │           │
│  └─ {"cd":12,"mt":"a"}                                    │  │           │
└────────────────────────────────────────────────────────┬──┼──┼───────────┘
                 ▲                                        │  │  │
                 │ ACK                                    │  │  │
                 └────────────────────────────────────────┼──┘  │
                                                          │     │
┌─────────────────────────────────────────────────────────┼─────┼─────────┐
│                ESP32 - Timer ISR (Core 0)               │     │         │
│                                                          │     │         │
│  samplingCallback() {                                   │     │         │
│    if (streaming_active) ◄─────────────────────────────┼─────┘         │
│       // FALSE - no more writes                         │               │
│  }                                                       │               │
└──────────────────────────────────────────────────────────┘               │
                                                                           │
┌──────────────────────────────────────────────────────────────────────────┘
│                ESP32 - Streaming Task (Core 1)
│
│  streamingTask() {
│    while (streaming_active) ◄─────────────────────────────┐
│       // Loop exits (FALSE)                               │
│                                                            │
│    vTaskDelete(NULL); ◄────────────────────────────────────┘
│    // Task terminates
│  }
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Timing Diagram @ 100 Hz Streaming

```
Time (ms):     0      10      20      30      40      50      ... 500    510
               │       │       │       │       │       │           │       │
┌──────────────┼───────┼───────┼───────┼───────┼───────┼─ ─ ─ ─ ──┼───────┼──
│ Timer ISR    │▲      │▲      │▲      │▲      │▲      │▲          │▲      │▲
│ (Core 0)     ││      ││      ││      ││      ││      ││          ││      ││
│              │└─ADC──┘└─ADC──┘└─ADC──┘└─ADC──┘└─ADC──┘└─         │└─ADC──┘└─
│              │ 5ms    5ms    5ms    5ms    5ms    5ms           5ms    5ms
└──────────────┴───────┴───────┴───────┴───────┴───────┴─ ─ ─ ─ ──┴───────┴──

┌──────────────────────────────────────────────────────────────────────────────
│ Circular     Write:  1       2       3       4       5               50     51
│ Buffer       Samples:1       2       3       4       5      ...      50     51
│              Available: Increments every 10ms
└──────────────────────────────────────────────────────────────────────────────

┌──────────────────────────────────────────────────────────────────────────────
│ Streaming                                                    ▼Read 50 samples
│ Task                                                         ▼Encode packet
│ (Core 1)                                                     ▼Send BT (11ms)
│                                                              └──────┘
│                                                              500ms wait
└──────────────────────────────────────────────────────────────────────────────
```

**Key Observations:**
- **Sample rate:** 100 Hz → 1 sample every 10 ms
- **Packet rate:** 2 Hz → 1 packet every 500 ms (50 samples)
- **ISR duration:** ~5-7 ms (ADC read + filter + buffer write)
- **Bluetooth TX:** ~11 ms per packet (108 bytes @ 9600 baud)
- **Buffer usage:** Peaks at 50 samples, then drains to ~0

---

## Critical Sections and Mutex Usage

```
┌────────────────────────────────────────────────────────────────────────┐
│                        RESOURCE PROTECTION                              │
├─────────────────────┬──────────────────────┬────────────────────────────┤
│ Resource            │ Protection           │ Accessors                  │
├─────────────────────┼──────────────────────┼────────────────────────────┤
│ streaming_buffer[]  │ portENTER_CRITICAL   │ ISR (write)                │
│                     │ _ISR / _NON_ISR      │ streamingTask (read)       │
├─────────────────────┼──────────────────────┼────────────────────────────┤
│ buffer_write_index  │ Critical sections    │ ISR (write)                │
│ buffer_read_index   │                      │ streamingTask (write)      │
│                     │                      │ getAvailableSamples (read) │
├─────────────────────┼──────────────────────┼────────────────────────────┤
│ streaming_active    │ volatile bool        │ All tasks                  │
│                     │ (atomic on ESP32)    │                            │
├─────────────────────┼──────────────────────┼────────────────────────────┤
│ BTSerial (UART2)    │ semaphore_bluetooth  │ streamingTask              │
│                     │ (FreeRTOS mutex)     │ MessageHandler             │
├─────────────────────┼──────────────────────┼────────────────────────────┤
│ I2C Bus (ADS1115)   │ i2cMutex             │ ISR (ADC read)             │
│                     │                      │ Gyroscope module           │
└─────────────────────┴──────────────────────┴────────────────────────────┘
```

---

## Error Handling Flow

```
┌──────────────────────────────────────────────────────────────────────────┐
│                        FAILURE SCENARIOS                                  │
└───────┬──────────────────────────────────────────────────────────────────┘
        │
        ├─ [1] Buffer Overflow (write catches read)
        │   ├─ Detection: write_index == read_index after increment
        │   ├─ Action: read_index++ (drop oldest sample)
        │   └─ Log: "Buffer overflow" (rate-limited to 1/sec)
        │
        ├─ [2] Bluetooth Mutex Timeout (100ms)
        │   ├─ Detection: xSemaphoreTake() returns false
        │   ├─ Action: Skip packet transmission, return false
        │   └─ Log: "Send raw data failed: mutex timeout"
        │
        ├─ [3] Bluetooth Disconnect
        │   ├─ Detection: Bluetooth::isConnected() == false
        │   ├─ Action: Semg::disableStreaming()
        │   └─ Log: "Bluetooth disconnected, stopping streaming"
        │
        ├─ [4] Streaming Timeout (10 minutes)
        │   ├─ Detection: (millis() - start) > 600000
        │   ├─ Action: Semg::disableStreaming()
        │   └─ Log: "Streaming timeout (10 min), stopping..."
        │
        └─ [5] Task Creation Failure
            ├─ Detection: xTaskCreatePinnedToCore() returns !pdPASS
            ├─ Action: streaming_active = false
            └─ Log: "Failed to create streaming task"
```

---

## Performance Constraints Summary

| Constraint | Value | Impact | Mitigation |
|------------|-------|--------|------------|
| ADC read time | 5 ms | Max 140 Hz sampling | Upgrade to ADS1256 (30 kSPS) |
| Bluetooth baud | 9600 | 960 bytes/s bandwidth | Upgrade to 115200 baud (AT+BAUD4) |
| Filter overhead | 1-2 ms | Max 100 Hz with filtering | Use fixed-point arithmetic |
| Buffer size | 200 samples | 2 sec @ 100 Hz | Increase if needed (RAM permits) |
| Packet size | 108 bytes | 11 ms TX time @ 9600 | Already optimized (binary) |

**Recommended Operating Point:**
- **Rate:** 50 Hz (20 ms period)
- **Type:** Filtered (10-50 Hz bandpass)
- **Bandwidth:** 11.3% of 9600 baud
- **Margin:** 88.7% (robust against congestion)

---

**Document Version:** 1.0
**Companion to:** SEMG_STREAMING_ANALYSIS.md
