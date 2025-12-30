# IFPC3000 - Street Lighting Control System
## EC200U LTE Module Firmware Documentation

[![Platform](https://img.shields.io/badge/Platform-EC200U%20LTE-blue)]()
[![SDK](https://img.shields.io/badge/SDK-LTE01R03A03__C__SDK__U__1.0.8.12-green)]()
[![RTOS](https://img.shields.io/badge/RTOS-QuecOpen-orange)]()

## 📚 Documentation Index

This repository contains comprehensive documentation for the IFPC3000 street lighting control system firmware.

### Available Documentation

1. **[System Architecture & Overview](ifpc_doc.txt)**
   - High-level system architecture
   - Component descriptions
   - Data flow diagrams
   - Configuration structures
   - DO mode control specifications
   - External flash memory layout

2. **[Detailed Code Logic Flow](CODE_LOGIC_FLOW_DETAILED.md)**
   - Boot sequence analysis
   - Function-level code flow
   - MQTT state machine details
   - GPIO control implementation
   - I2C RTC/LCD operations
   - SPI flash storage operations
   - Configuration management
   - Error handling and recovery
   - Complete call hierarchies

## 🏗️ System Overview

### Hardware Components
- **Main Module**: EC200U LTE Module (Quectel)
- **Microcontroller**: ARM Cortex-based
- **External Flash**: AT25FF SPI Flash (4MB)
- **Real-Time Clock**: I2C DS3231 RTC
- **Display**: I2C LCD (via PCF8574 expander)
- **Digital I/O**: 3 Digital Outputs (DO), 6 Digital Inputs (DI)
- **Communication**: 
  - LTE (4G/Cat-1)
  - MQTT over TCP/IP
  - RS232 UART
  - RS485 (Energy Meter)

### Software Architecture
- **RTOS**: QuecOpen RTOS (FreeRTOS-based)
- **Protocol**: MQTT v3.1.1
- **JSON Parser**: cJSON library
- **Cloud Server**: 14.102.161.101:1883

## 🚀 Quick Start

### Key Entry Points

```c
// Main application entry point
int appimg_enter(void *param)
    └─► ql_init_demo_thread()
        ├─► Hardware initialization
        ├─► Network setup
        └─► Application tasks
```

### Main Subsystems

1. **MQTT Client** (`mqtt_demo.c`)
   - Cloud connectivity
   - Command processing
   - Telemetry publishing

2. **GPIO Control** (`gpio_DIDO.c`)
   - Digital output control (street lights)
   - Digital input monitoring
   - 100ms polling cycle

3. **I2C Management** (`I2C_RTC_LCD.c`)
   - RTC time synchronization
   - LCD status display
   - 1-second update cycle

4. **Flash Storage** (`spi_flash_at25ff.c`)
   - Configuration persistence
   - History data logging
   - Wear-leveling support

## 📖 Reading Guide

### For New Developers
Start with:
1. [ifpc_doc.txt](ifpc_doc.txt) - Understand the overall architecture
2. [CODE_LOGIC_FLOW_DETAILED.md](CODE_LOGIC_FLOW_DETAILED.md) - Dive into implementation details

### For System Integrators
Focus on:
- Configuration structures in [ifpc_doc.txt](ifpc_doc.txt)
- MQTT command protocol in [CODE_LOGIC_FLOW_DETAILED.md](CODE_LOGIC_FLOW_DETAILED.md)
- DO mode control logic

### For Firmware Developers
Essential sections:
- Boot sequence in [CODE_LOGIC_FLOW_DETAILED.md](CODE_LOGIC_FLOW_DETAILED.md)
- Task initialization flow
- Function call hierarchies
- Error handling patterns

## 🎯 Key Features

### Street Light Control Modes
- **Manual Mode**: Direct control via cloud commands
- **Photo Mode**: Light sensor-based automation
- **Auto Mode**: Time-based scheduling (up to 85 schedules)
- **Astro Mode**: Sunrise/sunset calculation
- **Twilight Mode**: Civil twilight timing
- **Local Mode**: Local schedule override

### Cloud Communication
- **Protocol**: MQTT v3.1.1
- **QoS**: Configurable (QoS 0, 1)
- **Keep-Alive**: Configurable interval
- **Auto-Reconnect**: Network failure recovery
- **Commands Supported**:
  - `DO_CONTROL` - Digital output control
  - `SET_DO_MODE` - Change control mode
  - `CONFIG_UPDATE` - Configuration updates
  - `SYNC_REQUEST` - Status synchronization

### Data Persistence
- **External Flash**: 4MB AT25FF SPI Flash
- **Configuration**: Checksum-protected
- **History Logging**: Up to 3000 data packets
- **Wear Leveling**: Automatic sector rotation

## 🔧 Development

### Build Configuration
```c
// Key build flags
#define QL_APP_FEATURE_MQTT      // Enable MQTT
#define QL_APP_FEATURE_RELEASE   // Release build mode
```

### Memory Layout
```
Stack Sizes:
- ql_init_task:       4KB
- mqtt_client_thread: 8KB (JSON processing)
- gpio_dido_thread:   4KB
- i2c_rtc_lcd_thread: 4KB
```

### Flash Memory Map
```
0x00000000 - 0x00001FFF: General Configuration (8KB)
0x00002000 - 0x00003FFF: Schedule Data (8KB)
0x00004000 - 0x00005FFF: Permanent Data (8KB)
0x0000F000 - 0x003F0FFF: History Data (3.9MB)
0x003FF000 - 0x003FFFFF: Runtime Parameters (4KB)
```

## 📊 System Timing

- **MQTT Yield**: 100ms
- **GPIO Polling**: 100ms
- **RTC/LCD Update**: 1 second
- **Telemetry Publish**: Configurable (LogRate parameter)
- **Event Publishing**: Immediate on state change

## 🛡️ Error Handling

- **Watchdog Timer**: Enabled in release builds
- **Network Recovery**: Automatic reconnection
- **Flash Write Protection**: Delayed writes with counters
- **I2C Bus Recovery**: Automatic re-initialization
- **Checksum Validation**: Configuration integrity checks

## 📝 Document Versions

- **System Architecture**: v1.0 (ifpc_doc.txt)
- **Code Logic Flow**: v1.0 (CODE_LOGIC_FLOW_DETAILED.md)
- **SDK Version**: LTE01R03A03_C_SDK_U_1.0.8.12
- **Last Updated**: 2025-12-30

## 🤝 Contributing

When updating documentation:
1. Maintain consistency between both documents
2. Update version numbers and dates
3. Keep code examples accurate
4. Test all documented procedures

## 📄 License

Copyright (c) 2020-2025 Quectel Wireless Solution, Co., Ltd.
All Rights Reserved. Proprietary and Confidential.

---

**For detailed code-level documentation, see [CODE_LOGIC_FLOW_DETAILED.md](CODE_LOGIC_FLOW_DETAILED.md)**

**For system architecture and high-level overview, see [ifpc_doc.txt](ifpc_doc.txt)**
