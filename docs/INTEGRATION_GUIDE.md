# NeuroEstimulator Integration Guide - PRISM Ecosystem

## Overview

The **NeuroEstimulator** (sEMG/FES Device) is a specialized biosignal acquisition and therapeutic intervention device within the PRISM federated research framework. This guide explains how the device integrates with other PRISM components to enable secure, standardized biomedical research data management.

---

## PRISM Ecosystem Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                Research Institution                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌────────────────────────┐     ┌──────────────────────────┐   │
│  │  Mobile App            │────▶│  NeuroEstimulator Device │   │
│  │  (React Native)        │ BT  │  (ESP32 sEMG/FES)        │   │
│  └────────────┬───────────┘     └──────────────────────────┘   │
│               │ HTTPS                                            │
│  ┌────────────▼──────────────────────────────────────────────┐  │
│  │     Interoperable Research Node (IRN)                     │  │
│  │     • Phase 1-4 Handshake Protocol                        │  │
│  │     • PostgreSQL Registry                                 │  │
│  │     • HL7 FHIR Data Model                                │  │
│  │     • Session & Device Management                        │  │
│  └────────────┬──────────────────────────────────────────────┘  │
│               │ Encrypted Channel                                │
└───────────────┼─────────────────────────────────────────────────┘
                │
    ┌───────────▼────────────────┐
    │  Federated PRISM Network   │
    │  • Cross-Node Queries      │
    │  • Data Aggregation        │
    │  • Multi-Institution View  │
    └────────────────────────────┘
```

---

## Data Flow Architecture

### Flow 1: Real-Time Therapeutic Session (Closed-Loop)

```
┌─────────────────────┐
│ Mobile App          │
│ (User Interface)    │
└──────────┬──────────┘
           │ BT Cmd: Configure FES
           ▼
┌─────────────────────────────────────────────┐
│ NeuroEstimulator (Bluetooth Message Handler)│
│ • Receives configuration parameters         │
│ • Validates safety thresholds               │
└──────────┬──────────────────────────────────┘
           │ Configure:
           │ - Amplitude (0-15V)
           │ - Frequency (1-100 Hz)
           │ - Pulse Width (1-100 ms)
           │ - Difficulty (1-100%)
           │ - Duration (1-60 sec)
           ▼
┌──────────────────────────────────────┐
│ sEMG Signal Processing (Core 0)      │
│ • 860 Hz ADC sampling                │
│ • Butterworth filtering (10-40 Hz)   │
│ • Downsampling to 215 Hz             │
│ • RMS calculation                    │
└──────────┬───────────────────────────┘
           │
           ▼ Trigger Detection (Threshold)
┌──────────────────────────────────────┐
│ FES Stimulation Module               │
│ • Biphasic pulse generation          │
│ • H-Bridge control                   │
│ • Voltage verification               │
│ • Safety stop conditions             │
└──────────┬───────────────────────────┘
           │ BT Response: Trigger + Status
           ▼
┌─────────────────────────┐
│ Mobile App              │
│ • Visual feedback       │
│ • Session logging       │
│ • Data recording        │
└─────────────────────────┘
```

### Flow 2: Research Data Acquisition (Open-Loop)

```
┌─────────────────────────────────────┐
│ Mobile App                          │
│ • Data Collection Interface         │
└──────────┬────────────────────────┬─┘
           │ BT Cmd: Configure     │
           │ Streaming             │
           ▼                       │
┌──────────────────────────────┐   │
│ Config: Rate, Data Type      │   │
│ • Rate: 215 Hz (fixed)       │   │
│ • Type: raw/filtered/rms     │   │
└──────────┬────────────────────┘   │
           │                        │
           │ BT Cmd: Start Streaming
           ▼                        │
┌────────────────────────────────────────┐
│ sEMG Real-Time Streaming (Core 0/1)    │
│ • Circular buffer management           │
│ • Binary protocol encoding             │
│ • 50 samples per packet                │
│ • ~230ms latency                       │
└──────────┬─────────────────────────────┘
           │ Continuous Binary Packets
           │ "t": timestamp, "v": [samples]
           ▼
┌────────────────────────────┐
│ Mobile App                 │
│ • Real-time visualization  │
│ • CSV data export          │
│ • Session storage          │
└────────────────────────────┘
```

---

## Component Integration Details

### 1. Mobile App ↔ NeuroEstimulator (Bluetooth)

**Primary Communication Protocol: Bluetooth Serial (SPP)**

| Interface | Value |
|-----------|-------|
| **Transport** | SPP (Serial Port Profile) |
| **Module** | HC-05 or HC-06 |
| **Baud Rate** | 9600 (default), 115200 (optional) |
| **Connection Name** | "NeuroEstimulator" |
| **Data Format** | JSON (commands/responses) + Binary (streaming) |
| **MTU Size** | 512 bytes |

**Key Message Exchange Patterns:**

```
PATTERN 1: FES Session Control

Mobile App                    NeuroEstimulator
     │                             │
     ├─→ {"cd":7,"mt":"w",...}    │  Configure FES
     │                             │
     ├←─ {"cd":7,"mt":"a"}        │  ACK
     │                             │
     ├─→ {"cd":2,"mt":"x"}        │  Start Session
     │                             │
     │    [Session Running]        │
     │    Monitoring: sEMG         │
     │    Threshold: Active        │
     │                             │
     ├←─ {"cd":9,"mt":"w"}        │  Trigger Detected
     ├←─ {"cd":8,"mt":"w",...}    │  Status Update
     │                             │
     ├─→ {"cd":3,"mt":"x"}        │  Stop Session
     │                             │
     └←─ {"cd":3,"mt":"a"}        │  ACK


PATTERN 2: sEMG Streaming

Mobile App                    NeuroEstimulator
     │                             │
     ├─→ {"cd":14,"mt":"w",...}   │  Configure Stream
     │   rate: 215 Hz              │
     │   type: "filtered"          │
     │                             │
     ├←─ {"cd":14,"mt":"a"}       │  ACK
     │                             │
     ├─→ {"cd":11,"mt":"x"}       │  Start Streaming
     │                             │
     ├←─ {"cd":11,"mt":"a"}       │  ACK
     │                             │
     │  [Continuous Data Stream]   │
     ├←─ {"cd":13,"mt":"w",...}   │  Packet 1 (50 samples)
     ├←─ {"cd":13,"mt":"w",...}   │  Packet 2 (50 samples)
     ├←─ {"cd":13,"mt":"w",...}   │  Packet 3 (50 samples)
     │   ... (continuous @ ~4.3 packets/sec)
     │                             │
     ├─→ {"cd":12,"mt":"x"}       │  Stop Streaming
     │                             │
     └←─ {"cd":12,"mt":"a"}       │  ACK
```

### 2. Mobile App ↔ InteroperableResearchNode (HTTPS)

**Research Data Submission Pipeline (Future Phase 5)**

The mobile application can submit device-originated session data to research nodes:

```
Mobile App Session Data
    ├─ Device Metadata
    │   ├─ Device ID
    │   ├─ Serial Number
    │   ├─ Firmware Version
    │   └─ Calibration Date
    │
    ├─ Session Parameters
    │   ├─ Amplitude (0-15V)
    │   ├─ Frequency (1-100 Hz)
    │   ├─ Pulse Width (1-100 ms)
    │   ├─ Difficulty Level (1-100%)
    │   └─ Intended Duration (1-60 sec)
    │
    ├─ Session Execution Data
    │   ├─ Start Timestamp (ISO 8601)
    │   ├─ End Timestamp
    │   ├─ Complete Stimuli Count
    │   ├─ Interrupted Stimuli Count
    │   ├─ Trigger Timestamps Array
    │   └─ Session Status
    │
    ├─ Biosignal Data (Optional)
    │   ├─ Raw sEMG @ 215 Hz (CSV)
    │   ├─ Filtered sEMG @ 215 Hz (CSV)
    │   ├─ RMS Envelope @ 215 Hz (CSV)
    │   └─ Data Quality Metrics
    │
    └─ Research Context
        ├─ Researcher ID
        ├─ Volunteer/Patient ID
        ├─ Research Project ID
        ├─ Session Purpose
        └─ Clinical Annotations

              │
              ▼
    HTTPS POST /api/data/submit

    Interoperable Research Node
    ├─ Authenticate (Phase 1-3)
    ├─ Decrypt payload (Phase 4)
    ├─ Validate HL7 FHIR compliance
    ├─ Store in PostgreSQL
    │   └─ Recording Session Table
    │   └─ Biosignal Data Table
    │   └─ Device Usage Audit
    │
    └─ Make available for Federated Queries
        └─ Cross-node data aggregation
        └─ Multi-institution research analysis
```

### 3. Device Registry Integration

**Device Registration in Research Node:**

```
NeuroEstimulator Registration Data:

{
  "DeviceId": "uuid-string",
  "DeviceName": "NeuroEstimulator-001",
  "DeviceType": "sEMG/FES Stimulator",
  "SerialNumber": "ESP32-12345",
  "FirmwareVersion": "v3.1.0",
  "ManufacturerName": "PRISM Biomedical Lab",
  "HardwareComponents": {
    "Microcontroller": "ESP32 DevKit V1",
    "ADC": "ADS1115 (16-bit, 860 SPS)",
    "BluetoothModule": "HC-05",
    "Sensor": "AD8232 sEMG",
    "IMU": "MPU6050"
  },
  "Capabilities": [
    "FES_STIMULATION",
    "SEMG_ACQUISITION",
    "REAL_TIME_STREAMING",
    "TRIGGER_DETECTION",
    "SESSION_RECORDING"
  ],
  "CalibrationData": {
    "LastCalibrationDate": "2025-10-16",
    "ADCZeroOffset": 0,
    "ADCScaleFactor": 1.0,
    "SensorGain": 1000,
    "FilterCutoffLow": 10.0,
    "FilterCutoffHigh": 40.0
  },
  "SafetyConfiguration": {
    "MaxAmplitude": 15.0,
    "MaxFrequency": 100,
    "MaxPulseWidth": 100,
    "EmergencyStopEnabled": true,
    "VoltageVerificationEnabled": true
  },
  "Status": "ACTIVE",
  "LocationCode": "Lab-A",
  "ResearcherAssignment": "researcher-uuid",
  "RegistrationTimestamp": "2025-10-16T10:30:00Z"
}
```

---

## Data Standards and Compliance

### HL7 FHIR Alignment

Device data maps to FHIR resources:

```
Device Resource
├─ identifier: Device Serial Number
├─ status: ACTIVE
├─ type: CodeableConcept (SNOMED CT)
│   └─ code: "258087007" (Electrical FES stimulation)
├─ manufacturer: "PRISM Biomedical Lab"
└─ deviceName:
    └─ name: "NeuroEstimulator-001"

Observation Resource (Session Recording)
├─ code: "11732-4" (Electrical stimulation response)
├─ subject: Reference(Patient/volunteer-id)
├─ device: Reference(Device/device-uuid)
├─ effectiveDateTime: session-start-timestamp
├─ component:
│   ├─ code: "Stimulus Amplitude"
│   │   └─ value: 3.0 (V)
│   ├─ code: "Stimulus Frequency"
│   │   └─ value: 38 (Hz)
│   ├─ code: "Trigger Detection Count"
│   │   └─ value: 10
│   └─ code: "sEMG RMS"
│       └─ value: [time-series data]

Bundle Resource (Session Data Package)
├─ type: "document"
├─ entry:
│   ├─ resource: Device
│   ├─ resource: Observation (Session Overview)
│   ├─ resource: Observation (sEMG Signals)
│   └─ resource: DiagnosticReport (Session Summary)
```

### SNOMED CT Codes

| Concept | Code | Description |
|---------|------|-------------|
| sEMG Stimulation | 258087007 | Electrical stimulation |
| Surface EMG | 89614008 | Electromyography |
| FES Device | 272372002 | Prosthetic/orthotic device |
| Body Structure (Muscle) | 91485014 | Skeletal muscle |
| Laterality (Left) | 7771000 | Left side |
| Therapeutic Response | 409011002 | Post-treatment status |

---

## Session Data Model

### Complete Session Structure

```
PRISM Session Record
{
  "sessionId": "uuid",
  "deviceId": "uuid",
  "volunteerId": "uuid",
  "researcherId": "uuid",
  "projectId": "uuid",

  // Temporal
  "sessionStartTime": "2025-10-16T14:30:00Z",
  "sessionEndTime": "2025-10-16T14:45:00Z",
  "sessionDurationSeconds": 900,

  // Device Configuration
  "deviceParameters": {
    "amplitude_volts": 3.0,
    "frequency_hz": 38.0,
    "pulse_width_ms": 12.0,
    "difficulty_percent": 50,
    "pulse_duration_sec": 5
  },

  // Session Execution Metrics
  "sessionMetrics": {
    "complete_stimuli_count": 45,
    "interrupted_stimuli_count": 3,
    "total_stimuli_attempted": 48,
    "success_rate_percent": 93.75,
    "trigger_timestamps_ms": [
      1234, 5678, 9012, ...
    ],
    "average_trigger_interval_ms": 18000
  },

  // Biosignal Data (Optional)
  "biosignalData": {
    "sEMG_raw": "s3://bucket/session-uuid-raw.csv",
    "sEMG_filtered": "s3://bucket/session-uuid-filtered.csv",
    "sEMG_rms": "s3://bucket/session-uuid-rms.csv",
    "sampling_rate_hz": 215,
    "sample_count": 193500,
    "data_quality_score": 0.94
  },

  // Clinical Annotations
  "clinicalAnnotations": {
    "muscle_group": "quadriceps",
    "body_side": "left",
    "severity_code": "moderate",
    "pain_scale_before": 6,
    "pain_scale_after": 3,
    "clinical_notes": "Patient showed good response..."
  },

  // Storage Location
  "storageLocation": "PostgreSQL:prism_node_a_registry",
  "dataIntegrity": {
    "checksum_sha256": "abc123...",
    "encryption_method": "AES-256-GCM",
    "signed_by": "node-certificate-fingerprint"
  }
}
```

---

## Safety and Security Considerations

### Multi-Layer Safety Architecture

```
Layer 1: Device Level
├─ FES_MODULE_ENABLE compile-time flag
├─ Voltage verification before each pulse
├─ Emergency stop (multi-level)
└─ Automatic timeout (10 minutes)

Layer 2: Communication Level
├─ Bluetooth SPP encryption (optional)
├─ Message validation (JSON schema)
├─ Command whitelist enforcement
└─ Session timeout tracking

Layer 3: Node Level
├─ Phase 1-3 Handshake validation
├─ Encrypted channel (AES-256-GCM)
├─ User authentication
└─ Device registry approval

Layer 4: Federation Level
├─ Cross-node identity verification
├─ End-to-end data encryption
├─ Audit logging
└─ LGPD/GDPR compliance
```

### Credential and Key Management

**Device Identity:**
```
Physical Device
    ├─ Serial Number (immutable hardware ID)
    ├─ MAC Address (Bluetooth module)
    └─ Embedded Certificate
        ├─ Self-signed X.509 (for future PKI)
        ├─ Private Key (stored in secure region)
        └─ Public Key (shared with research nodes)

Upon Registration with IRN:
    ├─ Device UUID assigned
    ├─ Certificate fingerprint (SHA-256) recorded
    ├─ Device registered in PostgreSQL
    └─ Available for session recording
```

---

## Testing and Validation

### Integration Test Scenarios

**Scenario 1: End-to-End Therapeutic Session**
```
1. Mobile app connects to device (BT SPP)
2. App configures FES parameters
3. App starts session
4. Device detects triggers in real-time
5. FES stimulation activated
6. Session data recorded locally
7. App receives status updates
8. Session completes
9. Data ready for submission to research node
```

**Scenario 2: Real-Time Streaming with Data Capture**
```
1. Mobile app initiates streaming configuration
2. Device streams 215 Hz sEMG data
3. App receives binary packets continuously
4. App exports to CSV format
5. Data quality metrics calculated
6. Verification against HL7 FHIR standards
7. Optional submission to research node
```

**Scenario 3: Cross-Node Data Federation**
```
1. Device Session A → Node A (Institution A)
2. Device Session B → Node B (Institution B)
3. Federated Query: "Aggregate FES response across institutions"
4. Node A + Node B collaborate via encrypted channel
5. Combined analysis results returned
```

---

## Troubleshooting and Maintenance

### Common Issues and Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| Streaming bandwidth exceeded | Baud rate too low for sampling rate | Upgrade HC-05 to 115200 baud or reduce streaming rate |
| Buffer overflow (data loss) | Streaming rate exceeds processing | Reduce streaming rate ≤30 Hz at 9600 baud |
| FES not activating | FES_MODULE_ENABLE=false at compile | Set flag to true and rebuild |
| Trigger detection too sensitive | Difficulty level too low | Increase difficulty (1-100%) via command |
| Session timeout | 10-minute automatic stop | Reconnect and restart streaming/session |

### Maintenance Schedule

- **Weekly**: Verify Bluetooth connectivity and battery levels
- **Monthly**: Validate sEMG signal quality and calibration drift
- **Quarterly**: Full device health check and firmware update compatibility
- **Annually**: Recalibration and safety inspection

---

## Future Enhancements

### Planned Integration Features

1. **Phase 5: Federated Queries** - Device data available for cross-institutional research queries
2. **Real-time Cloud Sync** - Automatic session submission to research nodes
3. **Multi-device Coordination** - Synchronized bilateral stimulation protocols
4. **Advanced Analytics** - Machine learning-based trigger optimization
5. **Wearable Integration** - Data fusion with smartwatch/accelerometer data

---

## References

- **Device Documentation**: `InteroperableResearchsEMGDevice/CLAUDE.md`
- **Backend Integration**: `InteroperableResearchNode/CLAUDE.md`
- **Protocol Specification**: `docs/api/bluetooth-protocol.md`
- **HL7 FHIR**: http://hl7.org/fhir/
- **SNOMED CT**: http://snomed.info/

---

**Document Version**: 1.0
**Last Updated**: 2025-10-16
**Maintained By**: PRISM Biomedical Engineering Lab
