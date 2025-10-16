# Bluetooth Protocol Documentation Guide

This document helps you choose the right protocol reference for your needs.

---

## Quick Decision Tree

```
START
  │
  ├─ "I need a 30-second overview"
  │  └─→ Read: streaming-protocol.md (TL;DR)
  │
  ├─ "I'm implementing a client/decoder"
  │  ├─ "Just the binary data packets"
  │  │  └─→ Read: bluetooth-protocol.md (Section: Packet Structure)
  │  │
  │  └─ "Both control AND streaming"
  │     └─→ Read: COMPLETE_BLUETOOTH_PROTOCOL.md
  │
  ├─ "I need to understand ALL message codes"
  │  └─→ Read: COMPLETE_BLUETOOTH_PROTOCOL.md (Section: Message Code Reference)
  │
  ├─ "I'm troubleshooting connection issues"
  │  └─→ Read: COMPLETE_BLUETOOTH_PROTOCOL.md (Sections: Connection, Troubleshooting)
  │
  ├─ "I'm integrating with a research node"
  │  └─→ Read: DEVICE_NODE_PROTOCOL_REFERENCE.md + COMPLETE_BLUETOOTH_PROTOCOL.md
  │
  └─ "I want encoder/decoder code examples"
     └─→ Read: bluetooth-protocol.md (Section: Example Code)
```

---

## Documentation Comparison

| Aspect | `streaming-protocol.md` | `bluetooth-protocol.md` | `COMPLETE_BLUETOOTH_PROTOCOL.md` |
|--------|-------------------------|------------------------|----------------------------------|
| **File Size** | 2 KB (Quick Read) | 40 KB (Detailed) | 80 KB (Comprehensive) |
| **Read Time** | 5 minutes | 30 minutes | 1-2 hours |
| **Depth** | Overview only | Deep dive (binary only) | Everything |
| **Code Examples** | Minimal (Python/JS) | Full implementations | References only |
| **Message Codes** | None | None | All 14 codes |
| **Troubleshooting** | Basic | Good | Comprehensive |
| **Security** | None | None | Yes |
| **Testing Guide** | None | Validation tests | Test cases |
| **State Machine** | None | None | Yes |
| **Firmware Versions** | None | v3.0+ | v1.0+ compatibility |

---

## Reading Paths by Role

### Mobile App Developer (React Native / Flutter)

**Goal**: Implement Bluetooth client for device communication

**Reading Path**:
1. Start: [`streaming-protocol.md`](./streaming-protocol.md) - (5 min) Understand basics
2. Deep dive: [`bluetooth-protocol.md`](./bluetooth-protocol.md) - (30 min) Binary protocol + decoder examples
3. Reference: [`COMPLETE_BLUETOOTH_PROTOCOL.md`](./COMPLETE_BLUETOOTH_PROTOCOL.md) - (1 hour) All message codes
4. Integration: [`../DEVICE_NODE_PROTOCOL_REFERENCE.md`](../DEVICE_NODE_PROTOCOL_REFERENCE.md) - (30 min) Node submission

**Key Sections**:
- Connection Establishment (COMPLETE_BLUETOOTH_PROTOCOL.md)
- Message Code Reference → Focus on codes 2, 7, 8, 11, 12, 13
- Binary Streaming Protocol (bluetooth-protocol.md) → Decoder examples
- Error Handling (COMPLETE_BLUETOOTH_PROTOCOL.md)

**Implementation Checklist**:
- [ ] Implement BinaryStreamDecoder class (from bluetooth-protocol.md)
- [ ] Handle JSON messages for commands (codes 1-12, 14)
- [ ] Switch protocols correctly after ACK
- [ ] Implement state machine for session management
- [ ] Add error handling (codes, timeouts, state violations)
- [ ] Test all 5 command sequences
- [ ] Test packet loss resilience
- [ ] Implement data visualization (215 Hz streaming)

### Backend/Research Node Developer

**Goal**: Integrate device session data into research infrastructure

**Reading Path**:
1. Quick overview: [`streaming-protocol.md`](./streaming-protocol.md) - (5 min)
2. Complete protocol: [`COMPLETE_BLUETOOTH_PROTOCOL.md`](./COMPLETE_BLUETOOTH_PROTOCOL.md) - (1 hour) Focus on message codes 2-14
3. Integration: [`../DEVICE_NODE_PROTOCOL_REFERENCE.md`](../DEVICE_NODE_PROTOCOL_REFERENCE.md) - (30 min) Data submission pipeline
4. Reference: [`../INTEGRATION_GUIDE.md`](../INTEGRATION_GUIDE.md) - (30 min) PRISM ecosystem integration

**Key Sections**:
- Message Codes 7, 8 (FES parameters and status)
- Session data model
- HL7 FHIR bundle structure
- Phase 1-4 handshake integration
- Error codes and state machine

### Firmware Developer (ESP32 / C++)

**Goal**: Modify device protocol implementation or add features

**Reading Path**:
1. Complete protocol: [`COMPLETE_BLUETOOTH_PROTOCOL.md`](./COMPLETE_BLUETOOTH_PROTOCOL.md) - (1.5 hours) Full understanding
2. Binary format deep dive: [`bluetooth-protocol.md`](./bluetooth-protocol.md) - (30 min) Packet encoding
3. C++ implementation: [`bluetooth-protocol.md`](./bluetooth-protocol.md) - (Section: Example Code) Learn encoding

**Key Sections**:
- Message Code Reference (all codes)
- Binary Streaming Protocol
- State Machine (Session and Streaming)
- Error Handling
- Timeout Management
- Security Model

**Implementation Considerations**:
- Message codes mapping to firmware commands
- State machine transitions
- Binary packet structure and encoding
- Timeout handling
- Buffer management

### Test Engineer / QA

**Goal**: Validate device protocol compliance and functionality

**Reading Path**:
1. Overview: [`streaming-protocol.md`](./streaming-protocol.md) - (5 min)
2. Complete guide: [`COMPLETE_BLUETOOTH_PROTOCOL.md`](./COMPLETE_BLUETOOTH_PROTOCOL.md) - (1 hour) Focus on Testing Guide
3. Troubleshooting: [`COMPLETE_BLUETOOTH_PROTOCOL.md`](./COMPLETE_BLUETOOTH_PROTOCOL.md) - (Section: Troubleshooting)

**Test Scenarios** (from COMPLETE_BLUETOOTH_PROTOCOL.md):
- Test Case 1: Connection Establishment
- Test Case 2: FES Session (full cycle)
- Test Case 3: Streaming (data collection)
- Test Case 4: Error Handling (4a-4c)

**Validation Checklist**:
- [ ] Test all 14 message codes
- [ ] Verify all error codes and messages
- [ ] Test state machine transitions
- [ ] Test timeout behaviors
- [ ] Test mutual exclusion (session XOR streaming)
- [ ] Verify binary packet structure
- [ ] Test error recovery
- [ ] Benchmark latencies
- [ ] Test Bluetooth reconnection
- [ ] Test with various baud rates (9600, 115200)

### Research Scientist / Clinician

**Goal**: Understand device capabilities and limitations for study design

**Reading Path**:
1. Overview: [`streaming-protocol.md`](./streaming-protocol.md) - (5 min) Understand basics
2. Key Capabilities: [`COMPLETE_BLUETOOTH_PROTOCOL.md`](./COMPLETE_BLUETOOTH_PROTOCOL.md) - (20 min) Focus on:
   - FES Parameters (Code 7): amplitude, frequency, pulse width, difficulty
   - Session Status (Code 8): session metrics
   - Streaming capabilities (Codes 11-13): 215 Hz rate, data types
3. Integration: [`../INTEGRATION_GUIDE.md`](../INTEGRATION_GUIDE.md) - (30 min) Research data flow

**Important Sections**:
- Message Code 7 (FES Parameters) - What can be controlled
- Message Code 8 (Session Status) - What data is captured
- Message Codes 11-13 (Streaming) - Data collection capabilities
- Performance Reference - Latency and accuracy
- Session State Machine - Session lifecycle
- Error Handling - Safety constraints

**Study Design Considerations**:
- FES parameter ranges and precision
- Streaming resolution (215 Hz = 4.65ms per sample)
- Session duration limits (max 60 seconds per stimulus)
- Battery constraints (need ~3.0V minimum)
- Data quality metrics
- Safety margins and timeout behaviors

---

## How the Protocols Relate

```
┌──────────────────────────────────────────────────────────┐
│      COMPLETE_BLUETOOTH_PROTOCOL.md (v2.0)              │
│      (Master reference - everything in one place)       │
│                                                          │
│  ├─ Sections 1-4: Architecture & Connection             │
│  ├─ Sections 5-6: BOTH Protocols                        │
│  │  ├─ JSON Protocol (Message Codes 1-14)               │
│  │  └─ Binary Protocol (Simplified overview)            │
│  └─ Sections 7-12: Cross-cutting concerns               │
│     (Security, Error Handling, Validation, Testing)     │
│                                                          │
└──────────────────────────────────────────────────────────┘
                         │
        ┌────────────────┼────────────────┐
        │                │                │
        ▼                ▼                ▼

┌─────────────────┐ ┌──────────────────┐ ┌────────────────┐
│streaming-proto. │ │bluetooth-proto.md│ │Quick Reference │
│(Quick Summary)  │ │(Deep Dive Binary)│ │for Integration │
│                 │ │                  │ │                │
│• Quick facts    │ │• Packet struct   │ │• Message codes │
│• Minimal code   │ │• Full decoders   │ │• Error handling│
│• Performance    │ │• Validation      │ │• State machine │
│  metrics        │ │• Benchmarks      │ │• Security      │
└─────────────────┘ └──────────────────┘ └────────────────┘
```

---

## Document Update History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2024-12-15 | Initial Bluetooth protocol (JSON only) |
| 2.0 (binary) | 2025-03-20 | Added binary streaming protocol |
| 2.1 | 2025-06-10 | Added error codes and state machine |
| 2.2 | 2025-09-01 | Comprehensive testing guide |
| 2.0 (complete) | 2025-10-16 | Unified all docs + decision tree |

---

## Frequently Asked Questions

**Q: Which file should I read first?**
A: Start with `streaming-protocol.md` (5 min) for a quick overview, then move to the comprehensive guide based on your role.

**Q: Do I need to read all three files?**
A: No. Each file serves a different audience. Use the decision tree above to find your path.

**Q: What's the difference between the protocols?**
A: **JSON** = human-readable commands (control), **Binary** = high-speed data (streaming). See "Dual Protocol System" in COMPLETE_BLUETOOTH_PROTOCOL.md.

**Q: Where do I find information about specific message codes?**
A: COMPLETE_BLUETOOTH_PROTOCOL.md, Section "Complete Message Code Reference" (Codes 1-14).

**Q: How do I implement a client?**
A: Use `bluetooth-protocol.md` Section "Example Code" for decoders, then refer to COMPLETE_BLUETOOTH_PROTOCOL.md for message codes.

**Q: What about error handling?**
A: See "Error Handling" section in COMPLETE_BLUETOOTH_PROTOCOL.md for all error codes and recovery strategies.

**Q: Where are the test cases?**
A: COMPLETE_BLUETOOTH_PROTOCOL.md, Section "Testing Guide" (4 comprehensive test cases).

**Q: How do I troubleshoot issues?**
A: See "Troubleshooting" section in COMPLETE_BLUETOOTH_PROTOCOL.md for common issues and solutions.

**Q: Which firmware versions are supported?**
A: See compatibility table in COMPLETE_BLUETOOTH_PROTOCOL.md Section "Firmware Version Compatibility" (v1.0 through v3.1).

---

## Cross-References

**Want to understand a specific message code?**
- Code 1-14 detailed specs: `COMPLETE_BLUETOOTH_PROTOCOL.md` → Section "Complete Message Code Reference"

**Need decoder implementation examples?**
- Python: `bluetooth-protocol.md` → "Python Implementation (Complete)"
- JavaScript/TypeScript: `bluetooth-protocol.md` → "JavaScript/TypeScript Implementation"
- C/C++: `bluetooth-protocol.md` → "C/C++ Implementation (Arduino/ESP32)"

**Researching integration with backend?**
- Start: `COMPLETE_BLUETOOTH_PROTOCOL.md` → Message Code 8 (Session Status)
- Deep dive: `../DEVICE_NODE_PROTOCOL_REFERENCE.md` → Phase 4 data submission
- Full context: `../INTEGRATION_GUIDE.md` → PRISM Ecosystem Architecture

**Designing a research study?**
- Device capabilities: `COMPLETE_BLUETOOTH_PROTOCOL.md` → Codes 7-8 (Parameters & Status)
- Data acquisition: `COMPLETE_BLUETOOTH_PROTOCOL.md` → Codes 11-13 (Streaming)
- Session constraints: `COMPLETE_BLUETOOTH_PROTOCOL.md` → State Machines & Timeouts

**Troubleshooting connection issues?**
- Connection flow: `COMPLETE_BLUETOOTH_PROTOCOL.md` → Section "Connection Establishment"
- Error codes: `COMPLETE_BLUETOOTH_PROTOCOL.md` → Section "Error Handling"
- Advanced diagnosis: `COMPLETE_BLUETOOTH_PROTOCOL.md` → Section "Troubleshooting"

---

## Document Maintenance

**Last Updated**: 2025-10-16
**Maintained By**: PRISM Development Team
**Audience**: All stakeholders (developers, researchers, clinicians)

For updates, corrections, or clarifications, refer to:
- Firmware documentation: `../../CLAUDE.md`
- Integration guide: `../INTEGRATION_GUIDE.md`
- Device node protocol: `../DEVICE_NODE_PROTOCOL_REFERENCE.md`

---

**Happy Reading!** 🚀

Start with the decision tree above to find your ideal learning path.
