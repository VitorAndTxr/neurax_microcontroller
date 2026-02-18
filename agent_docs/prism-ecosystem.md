# PRISM Project - Master Architecture Overview

**PRISM** (Project Research Interoperability and Standardization Model) is a comprehensive federated framework for biomedical research data management, designed to break down data silos and enable secure, standardized collaboration across research institutions.

## Ecosystem Components

The PRISM framework consists of four interconnected components:

1. **InteroperableResearchNode** (Backend): Core backend server implementing federated research data exchange with 4-phase cryptographic handshake protocol, PostgreSQL/Redis persistence, and 28-table clinical data model. See `../InteroperableResearchNode/CLAUDE.md`.

2. **InteroperableResearchsEMGDevice** (Embedded - **This Project**): ESP32-based hardware device for biosignal acquisition and therapeutic stimulation.

3. **InteroperableResearchInterfaceSystem** (Interface): TypeScript/Node.js middleware for protocol translation and communication orchestration. See `../InteroperableResearchInterfaceSystem/CLAUDE.md`.

4. **neurax_react_native_app** (Mobile): React Native application for research data collection and device control.

## PRISM Model Abstraction

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

## Key Design Principles

1. **Separation of Concerns**: Device (capture) ≠ Application (context) ≠ Node (storage/federation)
2. **Standardization**: HL7 FHIR + SNOMED CT for interoperability
3. **Real-time Processing**: Dual-core ESP32 architecture for time-critical signal processing
4. **Bluetooth Protocol**: JSON-based command-response pattern for mobile app communication
5. **Safety**: Multi-layer protections for therapeutic stimulation (emergency stops, voltage verification, timeouts)

## Data Flow (Complete Research Session)

```
1. Researcher configures via Mobile App → Bluetooth JSON commands
2. ESP32 Device acquires biosignals → 860 Hz sampling, Butterworth filtering
3. Trigger detection → sEMG threshold exceeded
4. FES stimulation → Biphasic pulses via H-bridge
5. Real-time streaming → Binary packets to Mobile App (215 Hz)
6. Data submission → Mobile App sends to Research Node (future)
```

## Navigation for AI Assistants

1. **Device Firmware** → You are here (`InteroperableResearchsEMGDevice/CLAUDE.md`)
2. **Backend/Node Development** → See `../InteroperableResearchNode/CLAUDE.md`
3. **Interface System** → See `../InteroperableResearchInterfaceSystem/CLAUDE.md`
4. **Master Overview** → See root `../CLAUDE.md` for cross-component context
