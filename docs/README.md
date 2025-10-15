# Documentation Index - NeuroEstimulator Firmware

**Project:** PRISM - InteroperableResearchsEMGDevice
**Platform:** ESP32 (esp32doit-devkit-v1)
**Framework:** Arduino/ESP-IDF
**Last Updated:** 2025-10-14

---

## Quick Navigation

### Getting Started
- **[Main Project README](../README.md)** - Project overview, hardware specs, setup instructions
- **[CLAUDE.md](../CLAUDE.md)** - Complete development guide for AI assistants

### sEMG Streaming System

#### 📘 Quick Start
- **[README_STREAMING.md](README_STREAMING.md)** - Quick reference guide
  - Configuration examples
  - Binary protocol overview
  - Troubleshooting checklist
  - Performance constraints summary

#### 📊 Technical Deep Dive
- **[SEMG_STREAMING_ANALYSIS.md](SEMG_STREAMING_ANALYSIS.md)** - Comprehensive technical analysis (40 pages)
  - Complete architecture documentation
  - Thread safety analysis
  - Performance bottleneck identification
  - Bandwidth calculations
  - Safety mechanisms
  - Future optimization recommendations

#### 🗺️ Visual Reference
- **[SEMG_STREAMING_FLOWCHART.md](SEMG_STREAMING_FLOWCHART.md)** - Visual flowcharts and diagrams
  - End-to-end data flow (ASCII art)
  - Timing diagrams @ different sampling rates
  - Critical sections and mutex usage
  - Error handling flows

#### 🔧 Protocol Specifications
- **[BINARY_STREAMING_PROTOCOL.md](BINARY_STREAMING_PROTOCOL.md)** - Binary packet format specification
  - Packet structure (header + data)
  - Encoding/decoding examples
  - Bandwidth comparison (JSON vs Binary)

- **[QUICK_REFERENCE_BINARY_PROTOCOL.md](QUICK_REFERENCE_BINARY_PROTOCOL.md)** - One-page protocol reference
  - Byte-by-byte breakdown
  - Mobile app integration guide

---

## Document Hierarchy

```
InteroperableResearchsEMGDevice/
├── README.md                          [Project overview]
├── CLAUDE.md                          [AI assistant development guide]
├── CHANGELOG.md                       [Version history]
│
├── docs/
│   ├── README.md                      [This file - Documentation index]
│   │
│   ├── sEMG Streaming (Primary Documentation)
│   │   ├── README_STREAMING.md        [⭐ START HERE - Quick reference]
│   │   ├── SEMG_STREAMING_ANALYSIS.md [📚 Deep dive - Complete analysis]
│   │   ├── SEMG_STREAMING_FLOWCHART.md[📊 Visual reference - Diagrams]
│   │   │
│   │   └── Binary Protocol
│   │       ├── BINARY_STREAMING_PROTOCOL.md      [Protocol spec]
│   │       └── QUICK_REFERENCE_BINARY_PROTOCOL.md [One-page reference]
│   │
│   └── [Future: FES, Bluetooth, Session Management docs]
│
└── src/
    ├── modules/
    │   ├── semg/
    │   │   ├── Semg.cpp                [Streaming implementation]
    │   │   ├── Semg.h                  [API definitions]
    │   │   └── StreamingProtocol.h     [Binary protocol header]
    │   │
    │   ├── bluetooth/
    │   │   ├── Bluetooth.cpp           [UART communication]
    │   │   └── Bluetooth.h
    │   │
    │   └── message_handler/
    │       ├── MessageHandler.cpp      [Command routing]
    │       └── MessageHandler.h
    │
    └── main.cpp                        [Entry point]
```

---

## Documentation by Audience

### For Mobile App Developers

**Goal:** Integrate sEMG streaming into your application

**Read in this order:**
1. [README_STREAMING.md](README_STREAMING.md) - Quick start (15 min)
2. [QUICK_REFERENCE_BINARY_PROTOCOL.md](QUICK_REFERENCE_BINARY_PROTOCOL.md) - Packet format (5 min)
3. [BINARY_STREAMING_PROTOCOL.md](BINARY_STREAMING_PROTOCOL.md) - Full spec (20 min)

**Key Topics:**
- Binary packet decoding
- Bluetooth connection setup (9600 baud)
- JSON command protocol (configure, start, stop)
- Error handling (timeouts, disconnects)

**Code Example (Android/Java):**
```java
// See BINARY_STREAMING_PROTOCOL.md Section 4
// Parsing 108-byte binary packets
```

---

### For Firmware Developers

**Goal:** Modify or debug the streaming system

**Read in this order:**
1. [README_STREAMING.md](README_STREAMING.md) - Overview (15 min)
2. [SEMG_STREAMING_FLOWCHART.md](SEMG_STREAMING_FLOWCHART.md) - Architecture (30 min)
3. [SEMG_STREAMING_ANALYSIS.md](SEMG_STREAMING_ANALYSIS.md) - Deep dive (2 hours)

**Key Topics:**
- Dual-core FreeRTOS architecture
- Timer ISR vs streaming task
- Circular buffer implementation
- Thread safety (critical sections, mutexes)
- Performance bottlenecks (ADC, Bluetooth)

**Code Locations:**
- `src/modules/semg/Semg.cpp:113-639` - Main implementation
- `src/modules/semg/StreamingProtocol.h` - Binary protocol definition
- `src/modules/bluetooth/Bluetooth.cpp:83-92` - Raw data transmission

---

### For Research Scientists

**Goal:** Understand system capabilities and limitations

**Read in this order:**
1. [README_STREAMING.md](README_STREAMING.md) - Overview (15 min)
2. [SEMG_STREAMING_ANALYSIS.md](SEMG_STREAMING_ANALYSIS.md) - Sections:
   - Executive Summary
   - Performance Constraints Summary (Appendix)
   - Bandwidth Utilization Table (Appendix B)

**Key Metrics:**
- **Sampling Rate Range:** 10-200 Hz (recommended: 20-50 Hz)
- **Data Types:** Raw, Filtered (Butterworth 10-50 Hz + 60 Hz notch), RMS
- **Latency:** 1-2 seconds (packet buffering)
- **Precision:** int16_t (1 mV resolution for ±4.096V range)
- **Maximum Throughput:** 250 Hz @ 9600 baud (56% utilization)

---

### For System Integrators

**Goal:** Integrate device into larger PRISM ecosystem

**Read in this order:**
1. [CLAUDE.md](../CLAUDE.md) - PRISM architecture overview
2. [README_STREAMING.md](README_STREAMING.md) - sEMG capabilities
3. [SEMG_STREAMING_ANALYSIS.md](SEMG_STREAMING_ANALYSIS.md) - Section: "Data Flow (Complete Research Session)"

**Integration Points:**
- **Mobile App** → **sEMG Device** (Bluetooth JSON + Binary)
- **Mobile App** → **Research Node** (HTTPS REST API, future)
- **Research Node** → **Federated Network** (4-phase handshake protocol)

**See Also:**
- `../InteroperableResearchNode/CLAUDE.md` - Backend integration guide
- `../neurax_react_native_app/` - Mobile app codebase

---

## Common Tasks

### Task: Test Streaming Locally

1. Connect ESP32 via USB
2. Open serial monitor:
   ```bash
   pio device monitor --baud 115200
   ```
3. Use Bluetooth terminal app (Android: Serial Bluetooth Terminal)
4. Send commands:
   ```json
   {"cd":14,"mt":"w","bd":{"rate":50,"type":"filtered"}}
   {"cd":11,"mt":"x"}
   // Wait for binary packets...
   {"cd":12,"mt":"x"}
   ```
5. Verify in serial logs:
   - "Streaming task started"
   - No buffer overflow warnings
   - "Streaming task finished (sent X packets total)"

**See:** [README_STREAMING.md - Testing Checklist](README_STREAMING.md#testing-checklist)

---

### Task: Upgrade Bluetooth to 115200 Baud

**Goal:** Enable high-speed streaming (1000+ Hz)

1. Edit `src/modules/bluetooth/Bluetooth.cpp:21-24`
   ```cpp
   // UNCOMMENT these lines:
   BTSerial.write("AT+BAUD4");
   BTSerial.end(true);
   BTSerial.begin(115200);
   ```

2. Rebuild and flash:
   ```bash
   pio run --target upload
   ```

3. Update mobile app Bluetooth connection to 115200 baud

**See:** [README_STREAMING.md - Advanced Topics](README_STREAMING.md#upgrading-to-high-speed-mode-115200-baud)

---

### Task: Debug Buffer Overflow

**Symptoms:**
- Serial log: "Streaming buffer overflow! Dropping oldest samples."
- Mobile app shows gaps in data

**Diagnosis:**
1. Check sampling rate vs Bluetooth bandwidth:
   - @ 9600 baud: Max 250 Hz (56% utilization)
   - @ 115200 baud: Max 1000+ Hz
2. Verify MessageHandler isn't blocking Bluetooth semaphore
3. Monitor buffer usage in serial logs

**Fix:**
- Lower sampling rate: 100 Hz → 50 Hz
- Upgrade Bluetooth to 115200 baud (see above)
- Increase buffer size: `STREAMING_BUFFER_SIZE=200` → `400` in `platformio.ini`

**See:** [SEMG_STREAMING_ANALYSIS.md - Section: Known Issues](SEMG_STREAMING_ANALYSIS.md#known-issues-and-limitations)

---

### Task: Add Custom Filter

**Example:** Implement adaptive threshold filtering

1. Edit `src/modules/semg/Semg.cpp:505-521` (`applyStreamingFilter()`)
2. Add new case to `StreamingDataType` enum in `Semg.h:23-27`
3. Update JSON parser in `MessageHandler.cpp:262-275`
4. Test with:
   ```json
   {"cd":14,"mt":"w","bd":{"rate":50,"type":"adaptive"}}
   ```

**See:** [SEMG_STREAMING_ANALYSIS.md - Section: Filter Application](SEMG_STREAMING_ANALYSIS.md#32-filter-application-applyStreamingFilter---semgcpp505-521)

---

## Glossary

| Term | Definition |
|------|------------|
| **sEMG** | Surface Electromyography - measurement of muscle electrical activity |
| **FES** | Functional Electrical Stimulation - therapeutic muscle stimulation |
| **ISR** | Interrupt Service Routine - timer callback function |
| **Critical Section** | FreeRTOS atomic code block (`portENTER_CRITICAL`) |
| **Circular Buffer** | Ring buffer for streaming data (200 samples) |
| **Binary Protocol** | Custom packet format (108 bytes: header + int16_t array) |
| **ADC** | Analog-to-Digital Converter (ADS1115, 16-bit, I2C) |
| **Butterworth Filter** | IIR bandpass filter (10-50 Hz) for noise removal |
| **Notch Filter** | IIR filter to remove 60 Hz power line interference |

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-10-14 | Initial documentation set created |
|     |            | - README_STREAMING.md (quick reference) |
|     |            | - SEMG_STREAMING_ANALYSIS.md (deep dive) |
|     |            | - SEMG_STREAMING_FLOWCHART.md (visual diagrams) |
|     |            | - Documentation index (this file) |

---

## Contributing

When adding new documentation:

1. **Follow existing structure:**
   - Quick reference (README_*.md)
   - Deep dive (*_ANALYSIS.md)
   - Visual reference (*_FLOWCHART.md)

2. **Update this index:**
   - Add entry to "Document Hierarchy"
   - Update "Documentation by Audience" if needed
   - Add common task if applicable

3. **Use consistent formatting:**
   - Markdown with GitHub-flavored extensions
   - Code blocks with language hints (```cpp, ```json)
   - ASCII diagrams for flowcharts

4. **Document version:**
   - Update CHANGELOG.md
   - Add entry to "Version History" above

---

## External Resources

### ESP32 Documentation
- **ESP-IDF:** https://docs.espressif.com/projects/esp-idf/en/latest/
- **FreeRTOS:** https://www.freertos.org/Documentation/FreeRTOS_Reference_Manual_V10.0.0.pdf
- **PlatformIO:** https://docs.platformio.org/en/latest/platforms/espressif32.html

### Bluetooth
- **HC-05 AT Commands:** https://www.gme.cz/data/attachments/dsh.772-148.1.pdf
- **Bluetooth SPP:** https://www.bluetooth.com/specifications/specs/serial-port-profile-1-2/

### Signal Processing
- **Butterworth Filters:** https://en.wikipedia.org/wiki/Butterworth_filter
- **sEMG Signal Processing:** https://www.ncbi.nlm.nih.gov/pmc/articles/PMC6514953/

### PRISM Project
- **Backend Repository:** `../InteroperableResearchNode/`
- **Mobile App Repository:** `../neurax_react_native_app/`
- **Main Documentation:** `../CLAUDE.md`

---

## Support

**For technical questions:**
1. Check relevant documentation (see above)
2. Review serial logs (115200 baud): `pio device monitor`
3. Enable verbose logging: `esp_log_level_set("*", ESP_LOG_DEBUG)` in `main.cpp`
4. Consult troubleshooting sections in SEMG_STREAMING_ANALYSIS.md

**For bug reports:**
- Include serial logs (full boot sequence + error)
- Specify firmware version (git commit hash)
- Describe expected vs actual behavior
- Attach Bluetooth packet captures if relevant

---

**Document Version:** 1.0
**Maintained by:** PRISM Development Team
**Last Review:** 2025-10-14
