# NeuroEstimulator ↔ Research Node Communication Protocol Reference

Quick reference for integrating NeuroEstimulator device data with PRISM research nodes via mobile applications.

---

## Architecture Overview

```
Device                Mobile App              Research Node
  │                      │                         │
  │◄─── Bluetooth ───────│                         │
  │        (SPP)         │                         │
  │                      │                         │
  │                      │────── HTTPS/REST ──────▶│
  │                      │   (Phase 1-4 Handshake) │
  │                      │                         │
  │◄─ Device Session ────│◄─── Encrypted Payload ─│
  │    Data (CSV)        │                         │
```

---

## Phase 1: Device Connection & Configuration

### Mobile App → Device

**Command: Configure FES Parameters**
```json
{
  "cd": 7,
  "mt": "w",
  "bd": {
    "a": 3.0,      // Amplitude (0-15V)
    "f": 38.0,     // Frequency (1-100 Hz)
    "pw": 12.0,    // Pulse width (1-100 ms)
    "df": 50,      // Difficulty (1-100%)
    "pd": 5        // Pulse duration (1-60 sec)
  }
}
```

**Device → Mobile App**
```json
{
  "cd": 7,
  "mt": "a"  // Acknowledge
}
```

### Validation & Safety Checks
- ✓ Amplitude range: 0-15V
- ✓ Frequency range: 1-100 Hz
- ✓ Pulse width range: 1-100 ms
- ✓ Duration range: 1-60 seconds
- ✓ Battery voltage > 3.0V

---

## Phase 2: Session Execution

### Start Session

**Mobile App → Device**
```json
{
  "cd": 2,
  "mt": "x"  // Execute start
}
```

**Device → Mobile App**
```json
{
  "cd": 2,
  "mt": "a"  // Session started
}
```

### Trigger Detection & FES Response

**Trigger Detected (Device → Mobile App)**
```json
{
  "cd": 9,
  "mt": "w"  // Trigger notification
}
```

**Session Status Update (Device → Mobile App)**
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
      "csa": 10,      // Complete stimulus amount
      "isa": 2,       // Interrupted stimulus amount
      "tlt": 45000,   // Time of last trigger (ms)
      "sd": 120000    // Session duration (ms)
    }
  }
}
```

### Stop Session

**Mobile App → Device**
```json
{
  "cd": 3,
  "mt": "x"  // Execute stop
}
```

**Device → Mobile App**
```json
{
  "cd": 3,
  "mt": "a"  // Session stopped
}
```

---

## Phase 3: Real-Time Streaming (Optional Parallel)

### Configure Streaming

**Mobile App → Device**
```json
{
  "cd": 14,
  "mt": "w",
  "bd": {
    "rate": 215,       // Hz (fixed at 215)
    "type": "filtered" // "raw", "filtered", or "rms"
  }
}
```

**Device → Mobile App**
```json
{
  "cd": 14,
  "mt": "a"  // Configuration acknowledged
}
```

### Start Streaming

**Mobile App → Device**
```json
{
  "cd": 11,
  "mt": "x"  // Execute start
}
```

**Device → Mobile App**
```json
{
  "cd": 11,
  "mt": "a"  // Streaming started
}
```

### Continuous Data Packets

**Device → Mobile App (Continuous)**
```json
{
  "cd": 13,
  "mt": "w",
  "bd": {
    "t": 12345,                              // Timestamp (ms since boot)
    "v": [23.4, 25.1, 22.8, 24.5, 26.2, ...] // 50 samples per packet
  }
}
```

**Bandwidth Calculation:**
- 50 samples/packet × 4 bytes/sample = 200 bytes payload
- ~4.3 packets/second @ 215 Hz = ~860 bytes/sec
- @ 9600 baud: 9600 ÷ 8 = 1200 bytes/sec ✓ Safe margin
- Recommended max streaming rate: 30 Hz to avoid overflow

### Stop Streaming

**Mobile App → Device**
```json
{
  "cd": 12,
  "mt": "x"  // Execute stop
}
```

**Device → Mobile App**
```json
{
  "cd": 12,
  "mt": "a"  // Streaming stopped
}
```

---

## Phase 4: Session Data Submission to Research Node

### Step 1: Collect Session Data (Mobile App)

**Local Session Record Structure:**
```javascript
const sessionData = {
  // Device Info
  deviceId: "device-uuid",
  serialNumber: "ESP32-12345",
  firmwareVersion: "v3.1.0",

  // Session Metadata
  sessionId: "session-uuid",
  volunteerId: "volunteer-uuid",
  researcherId: "researcher-uuid",
  projectId: "project-uuid",
  sessionStartTime: "2025-10-16T14:30:00Z",
  sessionEndTime: "2025-10-16T14:45:00Z",

  // Device Configuration
  deviceParameters: {
    amplitude: 3.0,    // volts
    frequency: 38.0,   // Hz
    pulseWidth: 12.0,  // ms
    difficulty: 50,    // %
    duration: 5        // sec
  },

  // Session Metrics (from device)
  sessionMetrics: {
    completeStimuliCount: 45,
    interruptedStimuliCount: 3,
    triggerTimestamps: [1234, 5678, ...],
    totalDuration: 900  // seconds
  },

  // Optional: Biosignal Data
  biosignalData: {
    type: "filtered",   // raw, filtered, or rms
    samplingRate: 215,  // Hz
    dataFile: "base64-encoded-csv-data",
    sampleCount: 193500
  },

  // Clinical Context
  clinicalAnnotations: {
    muscleGroup: "quadriceps",
    bodySide: "left",
    painBefore: 6,
    painAfter: 3,
    notes: "Good patient compliance"
  }
};
```

### Step 2: Prepare Encrypted Payload (Mobile App)

**Transform to HL7 FHIR Bundle:**
```json
{
  "resourceType": "Bundle",
  "type": "transaction",
  "entry": [
    {
      "resource": {
        "resourceType": "Device",
        "identifier": {"system": "urn:prism:device", "value": "device-uuid"},
        "type": {"coding": [{"system": "http://snomed.info/sct", "code": "258087007"}]},
        "manufacturer": "PRISM Biomedical Lab"
      }
    },
    {
      "resource": {
        "resourceType": "Observation",
        "code": {"coding": [{"system": "http://snomed.info/sct", "code": "11732-4"}]},
        "subject": {"reference": "Patient/volunteer-uuid"},
        "effectiveDateTime": "2025-10-16T14:30:00Z",
        "component": [
          {"code": {"text": "Stimulus Amplitude"}, "valueQuantity": {"value": 3.0, "unit": "V"}},
          {"code": {"text": "Trigger Count"}, "valueQuantity": {"value": 45}}
        ]
      }
    }
  ]
}
```

### Step 3: Perform Handshake with Research Node (Mobile App)

**Phase 1: Establish Encrypted Channel**
```
POST /api/connection/establish-channel HTTP/1.1
Host: research-node.example.com
Content-Type: application/json

{
  "ephemeralPublicKey": "base64-encoded-ECDH-P384-public-key",
  "supportedCiphers": ["AES-256-GCM"]
}

←

{
  "serverEphemeralPublicKey": "base64-encoded-server-public-key",
  "selectedCipher": "AES-256-GCM",
  "iv": "base64-encoded-iv"
}
```

**Phase 2: Node Identification**
```
POST /api/connection/identify HTTP/1.1
Host: research-node.example.com

[Encrypted with shared symmetric key]

{
  "nodeId": "institution-node-id",
  "certificate": "base64-encoded-X509-certificate",
  "certificateFingerprint": "sha256-hash"
}

←

{
  "status": "IDENTIFIED",
  "nodeId": "research-node-123"
}
```

**Phase 3: Mutual Authentication**
```
POST /api/connection/authenticate HTTP/1.1
Host: research-node.example.com

[Encrypted]

{
  "challenge": "base64-32-byte-challenge",
  "signature": "base64-rsa2048-signature-of-challenge"
}

←

{
  "status": "AUTHENTICATED",
  "sessionToken": "auth-token"
}
```

**Phase 4: Submit Device Session Data**
```
POST /api/data/submit HTTP/1.1
Host: research-node.example.com
Authorization: Bearer {sessionToken}
X-Prism-Session: {sessionToken}

[Encrypted FHIR Bundle]

{
  "encryptedData": "base64-ciphertext",
  "iv": "base64-iv",
  "authTag": "base64-auth-tag"
}

←

{
  "status": "ACCEPTED",
  "dataId": "submitted-data-uuid",
  "storageLocation": "prism_node_a_registry.recording_sessions"
}
```

---

## Error Handling

### Device-Level Error Responses

**Invalid Command**
```json
{
  "cd": 999,
  "mt": "a",
  "error": "INVALID_COMMAND_CODE"
}
```

**Parameter Out of Range**
```json
{
  "cd": 7,
  "mt": "a",
  "error": "AMPLITUDE_OUT_OF_RANGE",
  "details": "Expected 0-15V, received 25V"
}
```

**Device Safety Lockout**
```json
{
  "cd": 2,
  "mt": "a",
  "error": "BATTERY_CRITICAL",
  "details": "Main battery voltage 2.8V < 3.0V threshold"
}
```

### Node-Level HTTP Errors

| Code | Meaning | Action |
|------|---------|--------|
| 200 | Success | Proceed |
| 400 | Invalid request | Check payload format |
| 401 | Unauthorized | Re-authenticate |
| 403 | Forbidden | Insufficient permissions |
| 409 | Conflict | Data already exists |
| 500 | Server error | Retry with exponential backoff |

---

## Security Best Practices

### 1. Message Validation
```
✓ Always validate JSON schema before sending
✓ Check numeric ranges (voltage, frequency, duration)
✓ Verify device is connected and responsive
✗ Never bypass safety validation
```

### 2. Encryption During Transit
```
✓ Use HTTPS/TLS for all node communication
✓ Implement certificate pinning for production
✓ Encrypt biosignal data at rest (AES-256-GCM)
✗ Never transmit sensitive data over unencrypted channels
```

### 3. Access Control
```
✓ Authenticate before submitting data to node
✓ Verify researcher identity and permissions
✓ Log all device access and data submissions
✗ Never allow anonymous data collection
```

### 4. Data Integrity
```
✓ Calculate SHA-256 checksums for data files
✓ Sign FHIR bundles with researcher private key
✓ Include audit trail in all submissions
✗ Never modify data after collection
```

---

## Testing Checklist

### Device Connection
- [ ] Device discoverable as "NeuroEstimulator" via Bluetooth
- [ ] SPP connection established with 9600 baud
- [ ] Command timeout: 5 seconds max

### FES Session
- [ ] Parameters accepted within valid ranges
- [ ] Session starts and stops cleanly
- [ ] Triggers detected correctly
- [ ] Status updates received continuously

### Streaming
- [ ] Configuration acknowledged
- [ ] Continuous data packets received
- [ ] No data loss during 30-second stream
- [ ] Stream stops on command

### Node Submission
- [ ] Phase 1 encrypted channel established
- [ ] Phase 2 node identification successful
- [ ] Phase 3 mutual authentication verified
- [ ] Phase 4 data stored in PostgreSQL
- [ ] Data queryable via Node API

---

## Performance Metrics

| Metric | Target | Measured | Status |
|--------|--------|----------|--------|
| ADC Sampling Rate | 860 Hz | 860 Hz | ✓ |
| Output Streaming Rate | 215 Hz | 215 Hz | ✓ |
| Bluetooth Latency | <100ms | ~50ms | ✓ |
| Trigger Detection Time | <200ms | ~120ms | ✓ |
| FES Response Time | <50ms | ~30ms | ✓ |
| Session Start Time | <500ms | ~200ms | ✓ |
| Node Authentication Time | <2s | ~1.5s | ✓ |

---

## References

- **Full Integration Guide**: `docs/INTEGRATION_GUIDE.md`
- **Bluetooth Protocol**: `docs/api/bluetooth-protocol.md`
- **Device Development**: `CLAUDE.md`
- **Backend Handshake**: `../InteroperableResearchNode/docs/architecture/handshake-protocol.md`
- **HL7 FHIR**: http://hl7.org/fhir/

---

**Document Version**: 1.0
**Last Updated**: 2025-10-16
**Audience**: Mobile App Developers, Backend Integration Engineers
