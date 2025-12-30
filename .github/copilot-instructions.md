# GitHub Copilot Instructions for ifpc3000

## Repository Overview

This repository contains firmware for the EC200U LTE Module integrated with cloud services via MQTT. The system is designed for IoT applications including street light control, energy metering, and data logging.

## Architecture

The system is based on the Quectel EC200U LTE module SDK (LTE01R03A03_C_SDK_U_1.0.8.12) with a modular architecture:

- **Entry Point**: `ql_init.c` > `appimg_enter()` initializes the system
- **Main Thread**: `ql_init_demo_thread()` spawns subsystem tasks
- **Subsystem Tasks**:
  - MQTT Task: Cloud communication (MQTT protocol)
  - GPIO Task: General purpose I/O (e.g., street lights)
  - I2C Task: I2C communication (RTC, LCD)
  - RS485 Task: Energy meter communication
  - SPI Flash Task: Data logging and storage

## Build System

### Building the Project

The project uses CMake as its build system:

```bash
# Build script location
cd LTE01R03A03_C_SDK_U_1.0.8.12

# Linux/Mac
./build_all.sh new EC200UCN_AA <version_label> [VOLTE] [DSIM] [debug/release]

# Windows
build_all.bat new EC200UCN_AA <version_label> [VOLTE] [DSIM] [debug/release]

# Clean build
./build_all.sh clean
```

### Build Configuration

- CMake configuration files are in `cmake/` directory
- Component configurations in `components/ql-config/`
- Application features configured via `components/ql-application/ql_app_feature_config.cmake`

## Code Conventions

### Language and Style

- **Primary Language**: C (embedded C for ARM Cortex-M)
- **Compiler**: ARM GCC (arm-none-eabi-gcc)
- **Standard**: Follow embedded C best practices for resource-constrained environments

### Naming Conventions

- **Prefix conventions**:
  - `ql_` prefix for Quectel SDK APIs
  - Task functions typically named with `_thread` suffix
  - Hardware-specific prefixes (e.g., `gpio_`, `i2c_`, `spi_`)

### Code Organization

- Application code resides in `LTE01R03A03_C_SDK_U_1.0.8.12/components/ql-application/`
- Each subsystem has its own directory (mqtt, gpio, i2c, spi, etc.)
- SDK components are in `components/` with their respective subdirectories

## Key Development Guidelines

### Embedded Systems Constraints

1. **Memory Management**:
   - Be mindful of stack sizes (typical task stack: 4096 bytes)
   - Avoid dynamic memory allocation where possible
   - Use static allocation for predictable memory usage

2. **Task Priority**:
   - Use `APP_PRIORITY_NORMAL` for standard application tasks
   - Consider real-time requirements when setting task priorities

3. **Watchdog Configuration**:
   - Watchdog enabled in release builds
   - Watchdog disabled in debug builds
   - Ensure long-running operations feed the watchdog

### Hardware Interaction

1. **GPIO Configuration**:
   - Initialize pins via `ql_pin_cfg_init()` using pin configuration tables
   - Document pin assignments in code comments

2. **Communication Protocols**:
   - MQTT for cloud communication (host: configurable, default port 1883)
   - I2C for sensors and displays
   - RS485 for industrial devices (energy meters)
   - SPI for flash memory

### Threading and Concurrency

- Use RTOS task primitives (`ql_rtos_task_create`)
- Design tasks to be modular and independent
- Document inter-task communication mechanisms
- Use appropriate synchronization primitives for shared resources

## Security Considerations

1. **Network Security**:
   - Use secure MQTT connections when available (TLS/SSL)
   - Validate all incoming data from network sources
   - Implement proper authentication mechanisms

2. **Data Protection**:
   - Sanitize sensitive data before logging
   - Secure credential storage (avoid hardcoded credentials)
   - Implement secure boot mechanisms where applicable

3. **Input Validation**:
   - Validate all external inputs (sensor data, network data)
   - Check buffer boundaries to prevent overflows
   - Implement bounds checking for array accesses

## Testing and Validation

### Testing Approach

- Test on target hardware (EC200U module) when possible
- Validate communication protocols with mock servers
- Test edge cases and error conditions
- Verify watchdog behavior in long-running scenarios

### Debug vs Release

- Debug builds: Watchdog disabled, verbose logging enabled
- Release builds: Watchdog enabled, optimized performance
- Use conditional compilation (`#ifdef QL_APP_FEATURE_RELEASE`)

## Documentation

- Document system architecture changes in `ifpc_doc.txt`
- Include inline comments for complex algorithms
- Document pin configurations and hardware connections
- Maintain clear commit messages describing changes

## Common Development Tasks

### Adding a New Task

1. Create task function with signature: `void task_name_thread(void *param)`
2. Register task in `ql_init_demo_thread()` using `ql_rtos_task_create()`
3. Set appropriate stack size and priority
4. Implement task cleanup and error handling

### Adding New Hardware Support

1. Configure pins in pin configuration table
2. Initialize hardware in appropriate task or init function
3. Implement driver/protocol handling
4. Add error handling and validation
5. Document hardware connections and requirements

### Modifying MQTT Communication

1. MQTT client configuration in MQTT task
2. Define topics for publish/subscribe
3. Implement message handlers
4. Add reconnection logic
5. Handle network failures gracefully

## Code Review Guidelines

When reviewing code changes:

1. Verify memory safety (no buffer overflows, proper bounds checking)
2. Check for resource leaks (memory, file handles, etc.)
3. Validate error handling paths
4. Ensure thread safety for shared resources
5. Verify compliance with embedded system constraints
6. Check for proper initialization sequences
7. Validate hardware configuration changes

## Additional Resources

- SDK Release Notes: `Quectel_LTE01_C_SDK_U_SDK_Release_Notes_V0303.pdf`
- System Architecture: `ifpc_doc.txt`
- Quectel SDK Documentation: Located in SDK directories
- Component README files: Various SDK component directories

## Notes for Copilot

- This is an embedded IoT system with strict resource constraints
- Changes should maintain compatibility with the EC200U LTE module hardware
- Consider real-time requirements and task scheduling impacts
- Always validate changes can build successfully with the provided build scripts
- Be cautious with SDK modifications; prefer application-level changes
- Document hardware dependencies and configuration requirements
