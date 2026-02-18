# Project Structure

```
InteroperableResearchsEMGDevice/
├── src/
│   ├── main.cpp                    # Entry point, module initialization
│   ├── globals.h                   # Core assignments and mutexes
│   └── modules/
│       ├── adc/                    # ADS1115 ADC driver (860 SPS → 215 Hz)
│       ├── semg/                   # sEMG processing + streaming
│       │   ├── Semg.cpp            # Main implementation
│       │   ├── Semg.h              # API definitions
│       │   └── StreamingProtocol.h # Binary protocol header
│       ├── SemgFilter/             # Butterworth bandpass filters
│       ├── NotchFilter/            # 60 Hz notch filter (power line rejection)
│       ├── fes/                    # FES stimulation control (H-bridge)
│       ├── session/                # Session state machine (Core 0)
│       ├── message_handler/        # Bluetooth command router (Core 1)
│       ├── bluetooth/              # UART2 communication (HC-05/HC-06)
│       ├── gyroscope/              # MPU6050 IMU driver
│       ├── potentiometer/          # Digital potentiometer (FES amplitude)
│       ├── battery_monitor/        # Dual battery monitoring
│       ├── battery/                # Battery abstraction
│       ├── emergency_button/       # Emergency stop ISR
│       ├── led/                    # LED indicators (power, trigger, FES)
│       └── debug/                  # Debug utilities
│
├── lib/
│   └── libFilter/                  # External Butterworth filter (git submodule)
│
├── docs/                           # Documentation
│   ├── README.md                   # Documentation index
│   ├── api/                        # Protocol specifications
│   │   ├── COMPLETE_BLUETOOTH_PROTOCOL.md  # Full protocol reference v2.0
│   │   ├── bluetooth-protocol.md   # Binary streaming protocol
│   │   └── streaming-protocol.md   # 215Hz streaming quick reference
│   ├── architecture/               # System design diagrams
│   ├── development/                # Technical notes (ADC analysis, bugfixes)
│   ├── guides/                     # User documentation (data capture)
│   ├── database/                   # Runtime data model diagrams
│   ├── diagrams/                   # Class/task/hardware diagrams
│   ├── endpoints.md                # Complete message code catalog
│   └── workflows.md                # System workflow diagrams
│
├── agent_docs/                     # Extracted CLAUDE.md sections (lazy-loaded)
│
├── platformio.ini                  # Build configuration (all -D flags)
├── CLAUDE.md                       # AI assistant guidance (compact index)
├── README.md                       # Project overview
├── CHANGELOG.md                    # Version history (v1.0→v3.1)
├── capture_semg_data.py            # Serial sEMG data capture
├── capture_bluetooth_stream.py     # Bluetooth binary stream capture + plots
├── capture_bluetooth_auto.py       # Auto-detection Bluetooth capture
├── capture_bluetooth_simple.py     # Simplified Bluetooth capture
├── capture_serial_stream.py        # Serial stream capture
├── test_bluetooth_native.py        # Bluetooth native API test
├── test_bluetooth_packets.py       # Packet validation test
├── diagnose_bluetooth.py           # Bluetooth diagnostics
├── requirements.txt                # Python dependencies
└── .gitignore                      # Excludes CSV data files
```

**Note**: `include/` and `test/` directories exist but are currently unused.

All module headers define TAG constants for logging (e.g., `TAG_FES`, `TAG_SEMG`). Use `ESP_LOGI(TAG_*, ...)` for consistent log formatting.
