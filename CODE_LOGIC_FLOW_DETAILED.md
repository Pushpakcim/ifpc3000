# Detailed Code Logic Flow Documentation
# IFPC3000 - EC200U LTE Module Firmware

## Table of Contents
1. [System Overview](#system-overview)
2. [Boot Sequence](#boot-sequence)
3. [Main Entry Point Flow](#main-entry-point-flow)
4. [Initialization Thread](#initialization-thread)
5. [MQTT Communication Flow](#mqtt-communication-flow)
6. [GPIO DIDO Control Flow](#gpio-dido-control-flow)
7. [I2C RTC and LCD Flow](#i2c-rtc-and-lcd-flow)
8. [SPI Flash Storage Operations](#spi-flash-storage-operations)
9. [Configuration Management](#configuration-management)
10. [DO Mode Control Logic](#do-mode-control-logic)

---

## System Overview

### Hardware Platform
- **Module**: EC200U LTE Module (Quectel)
- **SDK Version**: LTE01R03A03_C_SDK_U_1.0.8.12
- **RTOS**: QuecOpen RTOS
- **Architecture**: ARM Cortex-based embedded system

### Key Components
- **Cloud Server**: 14.102.161.101:1883 (MQTT)
- **External Flash**: AT25FF SPI Flash (4MB)
- **Real-Time Clock**: I2C RTC (DS3231 compatible)
- **Display**: I2C LCD (via PCF8574 expander at 0x3C)
- **Digital I/O**: 3 Digital Outputs (DO), 6 Digital Inputs (DI)
- **Communication**: RS232 UART, RS485 for energy meter

---

## Boot Sequence

### Power-On Reset (POR) Flow

```
Hardware Reset
    ↓
ARM Core Boot
    ↓
QuecOpen RTOS Initialization
    ↓
Application Image Entry: appimg_enter()
```

---

## Main Entry Point Flow

### File: `components/ql-application/init/ql_init.c`

#### Function: `appimg_enter(void *param)`

This is the **main entry point** of the application firmware.

```c
int appimg_enter(void *param)
{
    QlOSStatus err = QL_OSI_SUCCESS;
    ql_task_t ql_init_task = NULL;
    
    // Step 1: Log entry with version info
    QL_INIT_LOG("init demo enter: %s @ %s", QL_APP_VERSION, QL_APP_BUILD_RELEASE_TYPE);
    
    // Step 2: Initialize C++ global constructors
    prvInvokeGlobalCtors();
    
    // Step 3: Configure watchdog based on build type
    if(0 == strcasecmp(QL_APP_BUILD_RELEASE_TYPE, "release"))
    {
        ql_dev_cfg_wdt(1);  // Enable watchdog in RELEASE build
    }
    else
    {
        ql_dev_cfg_wdt(0);  // Disable watchdog in DEBUG build
    }
    
    // Step 4: Initialize all GPIO pins from pin configuration table
    // Critical: Must be done here before any other initialization
    ql_pin_cfg_init();
    
    // Step 5: Create main initialization task
    err = ql_rtos_task_create(&ql_init_task, 
                              1024*4,              // 4KB stack
                              APP_PRIORITY_NORMAL, // Normal priority
                              "ql_init",           // Task name
                              ql_init_demo_thread, // Entry function
                              NULL,                // No parameters
                              1);                  // Task flags
    
    if(err != QL_OSI_SUCCESS)
    {
        QL_INIT_LOG("init failed");
    }
    
    return err;
}
```

#### Execution Flow Diagram:

```
┌─────────────────────────────────────────────┐
│  appimg_enter() - Main Entry Point          │
│                                             │
│  1. Log firmware version & build type      │
│  2. Call prvInvokeGlobalCtors()            │
│     └─> Execute all C++ global ctors       │
│  3. Configure watchdog timer               │
│     ├─ Release: Enable (ql_dev_cfg_wdt(1)) │
│     └─ Debug: Disable (ql_dev_cfg_wdt(0))  │
│  4. ql_pin_cfg_init()                      │
│     └─> Initialize ALL GPIO pins           │
│  5. ql_rtos_task_create()                  │
│     └─> Spawn ql_init_demo_thread()        │
│                                             │
│  Returns: QL_OSI_SUCCESS or error code     │
└─────────────────────────────────────────────┘
```

### Function: `ql_pin_cfg_init(void)`

Configures all GPIO pins according to the pin configuration table defined in `ql_pin_cfg.h`.

```c
void ql_pin_cfg_init(void)
{
    uint8_t       index        = 0;
    uint8_t       pin_num      = 0;
    uint8_t       default_func = 0;
    uint8_t       gpio_func    = 0;
    ql_GpioNum    gpio_num     = 0;
    ql_GpioDir    gpio_dir     = 0;
    ql_PullMode   gpio_pull    = 0;
    ql_LvlMode    gpio_lvl     = 0;

    // Iterate through pin configuration table
    for(index = 0; index < QL_GPIO_PIN_MAX; index++)
    {
        QL_INIT_LOG("pin%d=%d", index, ql_pin_cfg_map[index].pin_num);
        
        // Check for end of table marker
        if(QUEC_PIN_NONE == ql_pin_cfg_map[index].pin_num)
        {
            QL_INIT_LOG("init exit %d!", index);
            break;
        }
        
        // Extract pin configuration from table
        pin_num      = ql_pin_cfg_map[index].pin_num;
        default_func = ql_pin_cfg_map[index].default_func;
        gpio_func    = ql_pin_cfg_map[index].gpio_func;
        gpio_num     = ql_pin_cfg_map[index].gpio_num;
        gpio_dir     = ql_pin_cfg_map[index].gpio_dir;
        gpio_pull    = ql_pin_cfg_map[index].gpio_pull;
        gpio_lvl     = ql_pin_cfg_map[index].gpio_lvl;

        // Set pin function (GPIO, UART, SPI, I2C, etc.)
        ql_pin_set_func(pin_num, default_func);
        
        // If configured as GPIO, initialize GPIO properties
        if(default_func == gpio_func)
        {
            ql_gpio_init(gpio_num, gpio_dir, gpio_pull, gpio_lvl);
        }
    }
}
```

---

## Initialization Thread

### File: `components/ql-application/init/ql_init.c`

#### Function: `ql_init_demo_thread(void *param)`

This thread orchestrates the initialization of all subsystems in a specific order.

```c
static void ql_init_demo_thread(void *param)
{
    QL_INIT_LOG("init demo thread enter, param 0x%x", param);

    // ═══════════════════════════════════════════════════════════════
    // PHASE 1: HARDWARE PERIPHERALS INITIALIZATION
    // ═══════════════════════════════════════════════════════════════
    
    // 1.1 I2C Bus + RTC + LCD Display
    ql_i2c_RTC_LCD_init();      // → I2C_RTC_LCD.c
    
    // 1.2 LED Status Indicators
    ql_ledcfg_app_init();       // → led_cfg_demo.c
    
    // 1.3 Digital Input/Output for Street Light Relays
    ql_gpio_app_dido_init();    // → gpio_DIDO.c
    
    // 1.4 Physical Key/Button Interrupts
    ql_gpioint_app_key_init();  // → gpio_int_KEY.c

    // ═══════════════════════════════════════════════════════════════
    // PHASE 2: NETWORK & COMMUNICATION
    // ═══════════════════════════════════════════════════════════════
    
    // 2.1 Network Registration (SIM card, signal)
    ql_nw_app_init();           // → nw_demo.c
    
    // 2.2 Data Call (PDP context activation)
    ql_datacall_app_init();     // → datacall_demo.c
    
    // 2.3 SMS functionality
    ql_sms_app_init();          // → sms.c
    
    // 2.4 Power Management
    ql_power_app_init();        // → power_demo.c

    // ═══════════════════════════════════════════════════════════════
    // PHASE 3: APPLICATION LAYER (Conditional on QL_APP_FEATURE_MQTT)
    // ═══════════════════════════════════════════════════════════════
    
    #ifdef QL_APP_FEATURE_MQTT
        // 3.1 MQTT Client (MAIN APPLICATION)
        ql_mqtt_app_init();     // → mqtt_demo.c
        
        // 3.2 RS232 UART Communication
        ql_uart_rs232_init();   // → rs232_UART.c
        
        // 3.3 SPI External Flash
        ql_spi_flash_init();    // → spi_flash_at25ff.c
    #endif
    
    // Sleep for camera power stabilization
    ql_rtos_task_sleep_ms(1000);
    
    // Delete this initialization task (no longer needed)
    ql_rtos_task_delete(NULL);
}
```

#### Initialization Sequence Diagram:

```
┌────────────────────────────────────────────────────────────────┐
│  ql_init_demo_thread() - Main Initialization Thread           │
└────────────────────────────────────────────────────────────────┘
         │
         ├─► PHASE 1: Hardware Peripherals
         │   ├─► ql_i2c_RTC_LCD_init()
         │   │   └─► Spawns i2c_rtc_lcd_thread()
         │   ├─► ql_ledcfg_app_init()
         │   ├─► ql_gpio_app_dido_init()
         │   │   └─► Spawns gpio_dido_thread()
         │   └─► ql_gpioint_app_key_init()
         │
         ├─► PHASE 2: Network & Communication
         │   ├─► ql_nw_app_init()
         │   │   └─► Monitors network registration
         │   ├─► ql_datacall_app_init()
         │   │   └─► Manages PDP contexts
         │   ├─► ql_sms_app_init()
         │   └─► ql_power_app_init()
         │
         ├─► PHASE 3: Application Layer (if QL_APP_FEATURE_MQTT)
         │   ├─► ql_mqtt_app_init()
         │   │   └─► Spawns mqtt_client_thread()
         │   ├─► ql_uart_rs232_init()
         │   │   └─► Spawns rs232_thread()
         │   └─► ql_spi_flash_init()
         │       └─► Initializes SPI Flash & loads config
         │
         └─► ql_rtos_task_delete(NULL)
             └─► Initialization thread exits
```

---

## MQTT Communication Flow

### File: `components/ql-application/mqtt/mqtt_demo.c`

#### State Machine Overview

The MQTT client operates as a state machine with the following states:

```c
typedef enum {
    MQTT_STATE_IDLE,           // Initial state
    MQTT_STATE_WAIT_NETWORK,   // Waiting for SIM registration
    MQTT_STATE_WAIT_DATACALL,  // Waiting for PDP activation
    MQTT_STATE_CONNECTING,     // Connecting to MQTT broker
    MQTT_STATE_CONNECTED,      // TCP connection established
    MQTT_STATE_SUBSCRIBING,    // Subscribing to topics
    MQTT_STATE_RUNNING,        // Normal operation
    MQTT_STATE_DISCONNECTED,   // Connection lost
    MQTT_STATE_ERROR           // Error state
} mqtt_state_e;
```

#### Function: `mqtt_client_thread(void *param)`

```
┌─────────────────────────────────────────────────────────────────┐
│  MQTT Client Thread - Main Loop                                 │
└─────────────────────────────────────────────────────────────────┘
         │
         ├─► STATE: WAIT_NETWORK
         │   │
         │   ├─► while(1) {
         │   │       ql_nw_get_reg_status(&nw_status);
         │   │       if(nw_status == HOME || ROAMING)
         │   │           break;  // Network registered
         │   │       ql_rtos_task_sleep_s(2);
         │   │   }
         │   │
         │   └─► Transition to: WAIT_DATACALL
         │
         ├─► STATE: WAIT_DATACALL
         │   │
         │   ├─► Configure PDP context:
         │   │   ql_datacall_cfg_t cfg = {
         │   │       .profile_idx = 1,
         │   │       .apn = EPROM_General.Mo_Comm.Mo_APN,
         │   │       .auth_type = 0,
         │   │   };
         │   │
         │   ├─► ql_datacall_start(cfg.profile_idx, &cfg);
         │   │
         │   └─► Transition to: CONNECTING
         │
         ├─► STATE: CONNECTING
         │   │
         │   ├─► Prepare MQTT client configuration:
         │   │   mqtt_client_config_t mqtt_cfg = {
         │   │       .broker_ip   = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_IP,
         │   │       .broker_port = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Bro_Port,
         │   │       .client_id   = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Cli_Id,
         │   │       .username    = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Name,
         │   │       .password    = EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Us_Pass,
         │   │       .keepalive   = EPROM_General.Mo_Comm.MQTT_LiveFreq,
         │   │       .clean_session = 1,
         │   │   };
         │   │
         │   ├─► mqtt_client_new(&mqtt_cfg);
         │   ├─► mqtt_client_connect(client);
         │   │
         │   └─► Transition to: SUBSCRIBING
         │
         ├─► STATE: SUBSCRIBING
         │   │
         │   ├─► mqtt_client_subscribe(client,
         │   │       EPROM_General.Mo_Comm.MQTT_Conn.MQTT_Sub_Topic,
         │   │       QOS_1,
         │   │       mqtt_message_callback);
         │   │
         │   └─► Transition to: RUNNING
         │
         └─► STATE: RUNNING (Main Loop)
             │
             ├─► while(1) {
             │   │
             │   ├─► // Process incoming messages (non-blocking)
             │   │   mqtt_client_yield(client, 100);
             │   │
             │   ├─► // Periodic telemetry publishing
             │   │   if(current_time - last_publish >= publish_interval) {
             │   │       publish_telemetry_data(client);
             │   │       last_publish = current_time;
             │   │   }
             │   │
             │   ├─► // Event-driven publishing
             │   │   if(flag_DO_Changed || flag_DI_Changed) {
             │   │       publish_status_update(client);
             │   │       flag_DO_Changed = 0;
             │   │       flag_DI_Changed = 0;
             │   │   }
             │   │
             │   └─► ql_rtos_task_sleep_ms(100);
             │   }
             │
             └─► Loop continues indefinitely
```

#### Message Callback Handler

```c
static void mqtt_message_callback(mqtt_client_t *client, 
                                  const char *topic, 
                                  const char *payload, 
                                  int payload_len)
{
    // Parse JSON payload
    cJSON *root = cJSON_Parse(payload);
    if(root == NULL) {
        QL_MQTT_LOG("JSON parse error");
        return;
    }
    
    // Extract command
    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
    
    // Command dispatch
    if(cmd && strcmp(cmd->valuestring, "DO_CONTROL") == 0)
    {
        // Digital Output Control
        cJSON *channel = cJSON_GetObjectItem(root, "channel");
        cJSON *state = cJSON_GetObjectItem(root, "state");
        
        if(channel && state) {
            set_digital_output(channel->valueint, state->valueint);
            publish_ack(client, "DO_CONTROL", "SUCCESS");
        }
    }
    else if(cmd && strcmp(cmd->valuestring, "SET_DO_MODE") == 0)
    {
        // Change DO control mode
        cJSON *mode = cJSON_GetObjectItem(root, "mode");
        if(mode) {
            EPROM_General.DoModeDetails.Do_Mode = (RtuDoMode_e)mode->valueint;
            flag_flashUpdateEPROM_General = 1;
            apply_do_mode();
        }
    }
    else if(cmd && strcmp(cmd->valuestring, "CONFIG_UPDATE") == 0)
    {
        // Configuration update
        parse_configuration_json(root);
        flag_flashUpdateEPROM_General = 1;
    }
    else if(cmd && strcmp(cmd->valuestring, "SYNC_REQUEST") == 0)
    {
        // Send full status
        publish_full_status(client);
    }
    
    cJSON_Delete(root);
}
```

#### MQTT Command Flow:

```
Cloud Server                    EC200U Module
     │                                │
     │  ──── MQTT PUBLISH ────►       │
     │  Topic: v1/devices/1/1/2       │
     │  Payload: {"cmd":"DO_CONTROL", │
     │            "channel":0,         │
     │            "state":1}           │
     │                                 │
     │                                 ├─► mqtt_message_callback()
     │                                 │   ├─► cJSON_Parse()
     │                                 │   ├─► Extract "cmd"
     │                                 │   ├─► Extract "channel", "state"
     │                                 │   └─► set_digital_output(0, 1)
     │                                 │       └─► GPIO_29 = HIGH
     │                                 │
     │  ◄──── MQTT PUBLISH ────       │
     │  Topic: v1/devices/1/1/2/ack   │
     │  Payload: {"cmd":"DO_CONTROL", │
     │            "result":"SUCCESS"}  │
     │                                 │
```

---

## GPIO DIDO Control Flow

### File: `components/ql-application/mqtt/gpio_DIDO.c`

#### GPIO Pin Configuration

```c
static ql_gpio_cfg _ql_gpio_cfg[] =
{
    /* gpio_num   gpio_dir      gpio_pull      gpio_lvl    */
    {  GPIO_29,   GPIO_OUTPUT,  0xff,          LVL_LOW },  // DO-1 Street Light Zone 1
    {  GPIO_30,   GPIO_OUTPUT,  0xff,          LVL_LOW },  // DO-2 Street Light Zone 2
    {  GPIO_8,    GPIO_OUTPUT,  0xff,          LVL_LOW },  // DO-3 Street Light Zone 3
    {  GPIO_13,   GPIO_OUTPUT,  0xff,          LVL_LOW },  // GSM LED
    {  GPIO_22,   GPIO_INPUT,   PULL_NONE,     0xff    },  // DI-1 Status Input
    {  GPIO_23,   GPIO_INPUT,   PULL_NONE,     0xff    },  // DI-2 Status Input
    {  GPIO_2,    GPIO_INPUT,   PULL_NONE,     0xff    },  // DI-3 Status Input
    {  GPIO_3,    GPIO_INPUT,   PULL_NONE,     0xff    },  // DI-4 Status Input
    {  GPIO_1,    GPIO_INPUT,   PULL_NONE,     0xff    },  // DI-5 Status Input
    {  GPIO_0,    GPIO_INPUT,   PULL_NONE,     0xff    },  // DI-6 Status Input
    {  GPIO_21,   GPIO_INPUT,   PULL_NONE,     0xff    },  // Battery Status
    {  GPIO_18,   GPIO_INPUT,   PULL_NONE,     0xff    },  // AC Status
    {  GPIO_31,   GPIO_OUTPUT,  0xff,          LVL_LOW }   // Watchdog
};
```

#### DIDO Thread Flow

```c
static void gpio_dido_thread(void *param)
{
    // Initialize GPIO directions
    _ql_gpio_dido_init();
    
    while(1)
    {
        // ─────────────────────────────────────────────────────
        // PHASE 1: READ DIGITAL INPUTS (Status Feedback)
        // ─────────────────────────────────────────────────────
        for(int i = 0; i < MAX_DI_CHANNEL; i++)  // MAX_DI_CHANNEL = 6
        {
            ql_LvlMode level;
            ql_gpio_get_level(DI_GPIO[i], &level);
            EPROM_General.AI_DI_DO_Detail.dig_bit_array[i] = level;
            
            // Detect state change
            if(level != EPROM_General.AI_DI_DO_Detail.Old_dig_bit_array[i])
            {
                // Trigger event notification
                flag_DI_Changed = 1;
                EPROM_General.AI_DI_DO_Detail.Old_dig_bit_array[i] = level;
            }
        }
        
        // ─────────────────────────────────────────────────────
        // PHASE 2: CONTROL DIGITAL OUTPUTS (Street Light Relays)
        // ─────────────────────────────────────────────────────
        for(int i = 0; i < MAX_DO_CHANNEL; i++)  // MAX_DO_CHANNEL = 3
        {
            uint8_t desired_state = EPROM_General.AI_DI_DO_Detail.DOSignal[i];
            
            // Apply output
            ql_gpio_set_level(DO_GPIO[i], desired_state);
            
            // Track changes for cloud sync
            if(desired_state != EPROM_General.AI_DI_DO_Detail.OLDDOSignal[i])
            {
                flag_DO_Changed = 1;
                EPROM_General.AI_DI_DO_Detail.OLDDOSignal[i] = desired_state;
            }
        }
        
        // 100ms polling interval
        ql_rtos_task_sleep_ms(100);
    }
}
```

#### Digital Output Control Function

```c
void DO_On_Off(short State, short BitNumber)
{
    if(State == SET)  // Turn ON
    {
        QL_GPIODEMO_LOG("DO SET %d,%d", BitNumber, State);
        ql_gpio_set_direction(_ql_gpio_cfg[BitNumber-1].gpio_num, GPIO_OUTPUT);
        ql_gpio_set_level(_ql_gpio_cfg[BitNumber-1].gpio_num, LVL_HIGH);
    }
    else if(State == RESET)  // Turn OFF
    {
        QL_GPIODEMO_LOG("DO RESET %d,%d", BitNumber, State);
        ql_gpio_set_direction(_ql_gpio_cfg[BitNumber-1].gpio_num, GPIO_OUTPUT);
        ql_gpio_set_level(_ql_gpio_cfg[BitNumber-1].gpio_num, LVL_LOW);
    }
    else
    {
        ql_rtos_task_sleep_ms(1);
    }
}
```

#### DIDO Flow Diagram:

```
┌─────────────────────────────────────────────────────────────────┐
│  gpio_dido_thread() - Digital I/O Control Loop                 │
└─────────────────────────────────────────────────────────────────┘
         │
         │  LOOP FOREVER:
         │
         ├─► Read Digital Inputs (DI-1 to DI-6)
         │   │
         │   ├─► for(i=0; i<6; i++) {
         │   │       ql_gpio_get_level(DI_GPIO[i], &level);
         │   │       if(level != old_level[i]) {
         │   │           flag_DI_Changed = 1;  // Trigger MQTT publish
         │   │           old_level[i] = level;
         │   │       }
         │   │   }
         │   │
         │   └─► Updates: EPROM_General.AI_DI_DO_Detail.dig_bit_array[]
         │
         ├─► Control Digital Outputs (DO-1 to DO-3)
         │   │
         │   ├─► for(i=0; i<3; i++) {
         │   │       desired_state = EPROM_General.AI_DI_DO_Detail.DOSignal[i];
         │   │       ql_gpio_set_level(DO_GPIO[i], desired_state);
         │   │       if(desired_state != old_state[i]) {
         │   │           flag_DO_Changed = 1;  // Trigger MQTT publish
         │   │           old_state[i] = desired_state;
         │   │       }
         │   │   }
         │   │
         │   └─► Reads from: EPROM_General.AI_DI_DO_Detail.DOSignal[]
         │
         └─► ql_rtos_task_sleep_ms(100)  // 100ms cycle time
             │
             └─► Repeat loop
```

---

## I2C RTC and LCD Flow

### File: `components/ql-application/mqtt/I2C_RTC_LCD.c`

#### I2C Configuration

```c
#define SalveAddr_w_8bit  (0x51)  // RTC I2C write address (DS3231)
#define SalveAddr_r_8bit  (0x51)  // RTC I2C read address
#define LCD_I2C_ADDRESS   (0x3C)  // LCD I2C address (via PCF8574)

// I2C1 → RTC (DS3231)
// I2C2 → LCD Display (Raystar LCD via I2C expander)
```

#### LCD Write Functions

```c
// Write command to LCD
ql_errcode_i2c_e lcd_write_command(uint8_t command)
{
    unsigned char tI2c_tx_data_write[16] = {0,};
    ql_errcode_i2c_e ret;
    unsigned int tIndex = 0;
    
    tI2c_tx_data_write[tIndex++] = 0x00;     // Control byte (command)
    tI2c_tx_data_write[tIndex++] = command;  // Command byte
    
    ret = ql_I2cWrite(i2c_2, LCD_I2C_ADDRESS, 
                      tI2c_tx_data_write[0], 
                      &tI2c_tx_data_write[1], 
                      tIndex-1);
    
    if(ret != QL_I2C_SUCCESS) {
        lcd_intialized = 0;
        QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d", ret);
    }
    
    ql_rtos_task_sleep_ms(1);
    return ret;
}

// Write data to LCD
ql_errcode_i2c_e lcd_write_data(uint8_t data)
{
    uint8_t d[2];
    ql_errcode_i2c_e ret;
    unsigned int tIndex = 0;
    
    d[tIndex++] = 0x40;  // Control byte (data)
    d[tIndex++] = data;  // Data byte
    
    ret = ql_I2cWrite(i2c_2, LCD_I2C_ADDRESS, d[0], &d[1], tIndex-1);
    
    if(ret != QL_I2C_SUCCESS) {
        lcd_intialized = 0;
        QL_APP_I2C_LOG("ql_I2cWrite fail ret=%d", ret);
    }
    
    ql_rtos_task_sleep_ms(1);
    return ret;
}
```

#### I2C RTC/LCD Thread Flow

```
┌─────────────────────────────────────────────────────────────────┐
│  i2c_rtc_lcd_thread() - RTC & LCD Management                   │
└─────────────────────────────────────────────────────────────────┘
         │
         │  INITIALIZATION:
         │
         ├─► ql_i2c_init(QL_I2C_PORT_1, QL_I2C_STANDARD_MODE)  // 100kHz
         ├─► ql_i2c_init(QL_I2C_PORT_2, QL_I2C_STANDARD_MODE)  // 100kHz
         ├─► rtc_init()   // Initialize DS3231 RTC
         └─► lcd_init()   // Initialize LCD display
         │
         │  MAIN LOOP (every 1 second):
         │
         ├─► Read RTC Time
         │   │
         │   ├─► rtc_read_time(&rtc_time);  // I2C transaction to DS3231
         │   │   └─► Returns: struct tm with current date/time
         │   │
         │   └─► Sync system time periodically (every 60 seconds)
         │       if(current_time - last_sync > 60000) {
         │           ql_rtc_set_time(&rtc_time);
         │           last_sync = current_time;
         │       }
         │
         ├─► Update LCD Display
         │   │
         │   ├─► Line 1: Date and Time
         │   │   snprintf(lcd_line1, sizeof(lcd_line1), 
         │   │            "%02d/%02d %02d:%02d:%02d",
         │   │            rtc_time.tm_mday, rtc_time.tm_mon + 1,
         │   │            rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec);
         │   │
         │   ├─► Line 2: DO/DI Status
         │   │   snprintf(lcd_line2, sizeof(lcd_line2),
         │   │            "DO:%d%d%d DI:%d%d%d%d%d%d",
         │   │            DOSignal[0], DOSignal[1], DOSignal[2],
         │   │            dig_bit_array[0], dig_bit_array[1], ...);
         │   │
         │   ├─► lcd_set_cursor(0, 0);
         │   ├─► lcd_print(lcd_line1);    // I2C write to LCD
         │   ├─► lcd_set_cursor(1, 0);
         │   └─► lcd_print(lcd_line2);    // I2C write to LCD
         │
         ├─► Trigger DO Mode Check (every minute)
         │   │
         │   └─► if(rtc_time.tm_min != last_minute) {
         │           last_minute = rtc_time.tm_min;
         │           apply_do_mode();  // Check schedules/astro times
         │       }
         │
         └─► ql_rtos_task_sleep_ms(1000)  // 1 second interval
             │
             └─► Repeat loop
```

---

## SPI Flash Storage Operations

### File: `components/ql-application/mqtt/spi_flash_at25ff.c`

#### Flash Memory Layout

```
┌─────────────────────────────────────────────────────────────────┐
│  AT25FF Series SPI Flash - 4MB (0x00000000 - 0x003FFFFF)       │
├─────────────────────────────────────────────────────────────────┤
│  0x00000000 - 0x00001FFF  │  EPROM_General (8KB)               │
│                           │  - RTU Details                      │
│                           │  - Customer Details                 │
│                           │  - DI/DO Configuration              │
│                           │  - MQTT Settings                    │
│                           │  - DO Mode (Manual/Auto/Astro)      │
├─────────────────────────────────────────────────────────────────┤
│  0x00002000 - 0x00003FFF  │  EPROM_Schedule (8KB)              │
│                           │  - 85 Schedule Entries              │
│                           │  - Start/Stop Times                 │
├─────────────────────────────────────────────────────────────────┤
│  0x00004000 - 0x00005FFF  │  EPROM_PermanentData (8KB)         │
│                           │  - Hardware Version                 │
│                           │  - Device ID (Unique)               │
├─────────────────────────────────────────────────────────────────┤
│  0x0000F000 - 0x003F0FFF  │  History Data (3.9MB)              │
│                           │  - Max 3000 packets                 │
│                           │  - Logged meter readings            │
├─────────────────────────────────────────────────────────────────┤
│  0x003FF000 - 0x003FFFFF  │  Runtime Parameters (4KB)          │
│                           │  - Current state                    │
│                           │  - Last operation                   │
└─────────────────────────────────────────────────────────────────┘
```

#### SPI Flash Initialization

```c
void ql_spi_flash_init(void)
{
    // Initialize SPI bus
    ql_spi_config_t spi_cfg = {
        .port = QL_SPI_PORT_1,
        .mode = QL_SPI_MODE_0,
        .clk_freq = 10000000,  // 10MHz
        .cs_pin = SPI_CS_PIN,
    };
    ql_spi_init(&spi_cfg);
    
    // Read flash ID to verify connection
    uint32_t flash_id = flash_read_id();
    if(flash_id != EXPECTED_FLASH_ID)
    {
        pro_Flash_State = 0;  // Flash error
        return;
    }
    pro_Flash_State = 1;  // Flash OK
    
    // Load configuration from flash
    ExtFlash_Read_EPROM_General(0);
    ExtFlash_Read_EPROM_Schedule(0);
    ExtFlash_Read_EPROM_PermanentData(0);
}
```

---

## Configuration Management

### File: `components/ql-application/mqtt/configuration.c`

#### Configuration Structure

```c
struct Save_Para_General EPROM_General;
struct Save_Para_Schedule_Configuration EPROM_Schedule;
struct Save_Para_PermanentData EPROM_PermanentData;

// Flags for flash updates
uint8_t flag_flashUpdateEPROM_General = 1;
uint8_t flag_flashUpdateEPROM_Schedule = 1;
uint8_t flag_flashUpdateEPROM_PermanentData = 1;
```

#### Reading Configuration from Flash

```c
void ExtFlash_Read_EPROM_General(unsigned char makeDefault)
{
    unsigned char Set_default_Flash = 0;
    
    // Read from flash into RAM structure
    at25dfx_read((uint8_t*)&EPROM_General, 
                 sizeof(EPROM_General), 
                 EPROM_GENERAL_START_ADDRESS);
    
    // Validate checksum and magic byte
    if((EPROM_General.checkbyte != 0xAB) || 
       (EPROM_General.SizeOfStuct == 0xFFFF))
    {
        Set_default_Flash = 1;  // Invalid - use defaults
        QL_CONFIG_LOG("Invalid flash data, using defaults");
    }
    
    if(Set_default_Flash || makeDefault)
    {
        // Initialize with factory defaults
        init_default_general_config();
        
        // Save defaults to flash
        ExtFlash_update_EPROM_General();
    }
    else
    {
        // Valid configuration loaded
        QL_CONFIG_LOG("Configuration loaded from flash");
    }
    
    // Sync with runtime variables
    syncExtFlashVariableWithPCBPLCVariable();
}
```

#### Writing Configuration to Flash

```c
void ExtFlash_update_EPROM_General(void)
{
    // Update header
    EPROM_General.checkbyte = 0xAB;  // Magic byte
    EPROM_General.SizeOfStuct = sizeof(struct Save_Para_General);
    
    // Calculate checksum
    EPROM_General.ChecksumOfStuct = calculateCheckSumOfStruct(
        (uint8_t*)&EPROM_General, 
        sizeof(EPROM_General));
    
    // Erase flash sector (required before write)
    flash_erase_sector(EPROM_GENERAL_START_ADDRESS);
    
    // Write structure to flash
    at25dfx_write((uint8_t*)&EPROM_General, 
                  sizeof(EPROM_General), 
                  EPROM_GENERAL_START_ADDRESS);
    
    QL_CONFIG_LOG("Configuration saved to flash");
}
```

#### Configuration Update Flow:

```
┌─────────────────────────────────────────────────────────────────┐
│  Configuration Management Flow                                   │
└─────────────────────────────────────────────────────────────────┘
         │
         ├─► BOOT TIME: Load from Flash
         │   │
         │   ├─► ExtFlash_Read_EPROM_General(0)
         │   │   ├─► at25dfx_read() from address 0x00000000
         │   │   ├─► Validate checkbyte (0xAB) & checksum
         │   │   ├─► If valid: Use loaded data
         │   │   └─► If invalid: init_default_general_config()
         │   │
         │   ├─► ExtFlash_Read_EPROM_Schedule(0)
         │   │   └─► at25dfx_read() from address 0x00002000
         │   │
         │   └─► ExtFlash_Read_EPROM_PermanentData(0)
         │       └─► at25dfx_read() from address 0x00004000
         │
         ├─► RUNTIME: Configuration Changes
         │   │
         │   ├─► Via MQTT command: CONFIG_UPDATE
         │   │   ├─► parse_configuration_json(root)
         │   │   └─► flag_flashUpdateEPROM_General = 1
         │   │
         │   ├─► Via DO control: set_digital_output()
         │   │   └─► flag_flashUpdateEPROM_General = 1
         │   │
         │   └─► Via mode change: SET_DO_MODE
         │       └─► flag_flashUpdateEPROM_General = 1
         │
         └─► PERIODIC: Flash Write (if flag set)
             │
             └─► if(flag_flashUpdateEPROM_General) {
                     if(--flag_flashUpdateEPROM_General_WaitCounter == 0) {
                         ExtFlash_update_EPROM_General();
                         flag_flashUpdateEPROM_General = 0;
                         flag_flashUpdateEPROM_General_WaitCounter = 5;
                     }
                 }
```

---

## DO Mode Control Logic

### DO Modes Enumeration

```c
typedef enum
{
    RTU_DO_MODE_MANUAL          = 0,   // Manual ON/OFF from cloud/local
    RTU_DO_MODE_PHOTO           = 1,   // Light sensor based
    RTU_DO_MODE_AUTO            = 2,   // Time-based schedule
    RTU_DO_MODE_ASTROTIME_GEO   = 3,   // Sunrise/Sunset calculation
    RTU_DO_MODE_TWILIGHT_GEO    = 4,   // Civil/Nautical twilight
    RTU_DO_MODE_LOCAL           = 5    // Local schedule override
} RtuDoMode_e;
```

### apply_do_mode() Function Flow

```
┌─────────────────────────────────────────────────────────────────┐
│  apply_do_mode() - DO Mode Decision Engine                      │
└─────────────────────────────────────────────────────────────────┘
         │
         ├─► current_mode = EPROM_General.DoModeDetails.Do_Mode
         │
         └─► switch(current_mode) {
             │
             ├─► case RTU_DO_MODE_MANUAL:
             │   │
             │   └─► // No action - DO states controlled by cloud commands
             │       // MQTT handler directly sets DOSignal[] values
             │
             ├─► case RTU_DO_MODE_PHOTO:
             │   │
             │   ├─► light_level = read_adc_light_sensor()
             │   ├─► if(light_level < LIGHT_THRESHOLD_ON) {
             │   │       set_all_outputs(1);  // Dark → Lights ON
             │   │   }
             │   └─► else if(light_level > LIGHT_THRESHOLD_OFF) {
             │           set_all_outputs(0);  // Bright → Lights OFF
             │       }
             │
             ├─► case RTU_DO_MODE_AUTO:
             │   │
             │   ├─► get_rtc_time(&current_time)
             │   └─► check_schedule_and_apply(&current_time)
             │       │
             │       ├─► total_schedules = EPROM_Schedule.Total_No_Schedule
             │       │
             │       ├─► for(i=0; i<total_schedules && i<85; i++) {
             │       │       Schedule_Data *sch = &EPROM_Schedule.Schedule[i];
             │       │       if(sch->Sch_En_Di == CONF_DISABLE) continue;
             │       │       
             │       │       now_minutes = now->tm_hour * 60 + now->tm_min;
             │       │       start_minutes = sch->Start_HH * 60 + sch->Start_Min;
             │       │       stop_minutes = sch->Stop_HH * 60 + sch->Stop_Min;
             │       │       
             │       │       // Handle overnight schedules (22:00 - 06:00)
             │       │       if(start_minutes > stop_minutes) {
             │       │           if(now_minutes >= start_minutes || 
             │       │              now_minutes < stop_minutes) {
             │       │               set_all_outputs(1);  // Within schedule
             │       │               return;
             │       │           }
             │       │       }
             │       │       // Normal schedules (18:00 - 22:00)
             │       │       else {
             │       │           if(now_minutes >= start_minutes && 
             │       │              now_minutes < stop_minutes) {
             │       │               set_all_outputs(1);  // Within schedule
             │       │               return;
             │       │           }
             │       │       }
             │       │   }
             │       │
             │       └─► set_all_outputs(0);  // No active schedule
             │
             ├─► case RTU_DO_MODE_ASTROTIME_GEO:
             │   │
             │   ├─► get_rtc_time(&current_time)
             │   ├─► lat = EPROM_General.Cust_Detail.Lattitude
             │   ├─► lon = EPROM_General.Cust_Detail.Longitude
             │   │
             │   ├─► calculate_sun_times(lat, lon, &current_time, 
             │   │                        &sunrise, &sunset)
             │   │   └─► // Astronomical calculation using:
             │   │       // - Julian day number
             │   │       // - Solar declination
             │   │       // - Hour angle
             │   │       // - Local timezone offset
             │   │
             │   ├─► if(is_after_sunset(&current_time, &sunset) || 
             │   │      is_before_sunrise(&current_time, &sunrise)) {
             │   │       set_all_outputs(1);  // Night → Lights ON
             │   │   }
             │   └─► else {
             │           set_all_outputs(0);  // Day → Lights OFF
             │       }
             │
             ├─► case RTU_DO_MODE_TWILIGHT_GEO:
             │   │
             │   └─► // Similar to ASTROTIME but with twilight offset
             │       // Lights turn on 30 minutes before sunset
             │       // Lights turn off 30 minutes after sunrise
             │       twilight_offset = 30;  // minutes
             │       // ... calculation with offset applied
             │
             └─► case RTU_DO_MODE_LOCAL:
                 │
                 └─► check_local_schedule_and_apply()
                     └─> // Uses locally stored schedule
                         // Overrides cloud commands
             }
```

### Schedule Checking Algorithm

```c
void check_schedule_and_apply(struct tm *now)
{
    int total_schedules = EPROM_Schedule.Total_No_Schedule;
    
    for(int i = 0; i < total_schedules && i < 85; i++)
    {
        struct Schedule_Data *sch = &EPROM_Schedule.Schedule[i];
        
        if(sch->Sch_En_Di == CONF_DISABLE)
            continue;
        
        int now_minutes = now->tm_hour * 60 + now->tm_min;
        int start_minutes = sch->Start_HH * 60 + sch->Start_Min;
        int stop_minutes = sch->Stop_HH * 60 + sch->Stop_Min;
        
        // Handle overnight schedules (start > stop)
        // Example: 22:00 (1320 min) - 06:00 (360 min)
        if(start_minutes > stop_minutes)
        {
            if(now_minutes >= start_minutes ||  // After 22:00
               now_minutes < stop_minutes)      // Before 06:00
            {
                set_all_outputs(1);  // Within schedule - ON
                return;
            }
        }
        // Normal schedules (start < stop)
        // Example: 18:00 (1080 min) - 22:00 (1320 min)
        else
        {
            if(now_minutes >= start_minutes && 
               now_minutes < stop_minutes)
            {
                set_all_outputs(1);  // Within schedule - ON
                return;
            }
        }
    }
    
    // No active schedule found - turn OFF
    set_all_outputs(0);
}
```

---

## Complete System Data Flow

```
┌───────────────────────────────────────────────────────────────────────┐
│                         SYSTEM BOOT SEQUENCE                          │
└───────────────────────────────────────────────────────────────────────┘
         │
         ├─► appimg_enter()
         │   ├─► prvInvokeGlobalCtors()
         │   ├─► ql_dev_cfg_wdt()
         │   ├─► ql_pin_cfg_init()
         │   └─► Create Task: ql_init_demo_thread()
         │
         └─► ql_init_demo_thread()
             ├─► ql_i2c_RTC_LCD_init()      → Task: i2c_rtc_lcd_thread()
             ├─► ql_ledcfg_app_init()       → Task: led_cfg_thread()
             ├─► ql_gpio_app_dido_init()    → Task: gpio_dido_thread()
             ├─► ql_gpioint_app_key_init()  → Interrupt handlers
             ├─► ql_nw_app_init()           → Task: nw_thread()
             ├─► ql_datacall_app_init()     → Task: datacall_thread()
             ├─► ql_sms_app_init()          → Task: sms_thread()
             ├─► ql_power_app_init()        → Task: power_thread()
             ├─► ql_mqtt_app_init()         → Task: mqtt_client_thread()
             ├─► ql_uart_rs232_init()       → Task: rs232_thread()
             └─► ql_spi_flash_init()        → Init & load config

┌───────────────────────────────────────────────────────────────────────┐
│                      CONCURRENT TASK EXECUTION                        │
└───────────────────────────────────────────────────────────────────────┘

    ┌─────────────────────┐
    │ i2c_rtc_lcd_thread  │ (1 sec cycle)
    │ ├─ Read RTC         │
    │ ├─ Update LCD       │
    │ └─ Trigger DO mode  │
    └─────────────────────┘
             │
    ┌─────────────────────┐
    │ gpio_dido_thread    │ (100 ms cycle)
    │ ├─ Read DI[0-5]     │
    │ ├─ Set DO[0-2]      │
    │ └─ Set flags        │
    └─────────────────────┘
             │
    ┌─────────────────────┐
    │ mqtt_client_thread  │ (100 ms cycle)
    │ ├─ mqtt_yield()     │
    │ ├─ Publish periodic │
    │ └─ Publish events   │
    └─────────────────────┘
             │
             │  Inter-Task Communication via:
             │  - Global variables (EPROM_General, flags)
             │  - Semaphores (mqtt_semp)
             │  - Event flags (flag_DO_Changed, flag_DI_Changed)
             │
             ▼
    ┌─────────────────────────────────────────────┐
    │          Cloud Server (MQTT)                │
    │  14.102.161.101:1883                        │
    │  ├─ Receives: Telemetry, Status Updates     │
    │  └─ Sends: Control Commands, Configuration  │
    └─────────────────────────────────────────────┘
```

---

## Key Function Call Hierarchy

```
appimg_enter()
│
├─► prvInvokeGlobalCtors()
│   └─► Execute all C++ global constructors
│
├─► ql_dev_cfg_wdt(enable)
│   └─► Configure hardware watchdog timer
│
├─► ql_pin_cfg_init()
│   ├─► for each pin in ql_pin_cfg_map[]:
│   │   ├─► ql_pin_set_func(pin_num, func)
│   │   └─► ql_gpio_init(gpio_num, dir, pull, lvl)
│   └─► Returns when all pins configured
│
└─► ql_rtos_task_create("ql_init", ql_init_demo_thread)
    │
    └─► ql_init_demo_thread()
        │
        ├─► ql_i2c_RTC_LCD_init()
        │   ├─► ql_i2c_init(QL_I2C_PORT_1, STANDARD_MODE)
        │   ├─► ql_i2c_init(QL_I2C_PORT_2, STANDARD_MODE)
        │   ├─► rtc_init()
        │   ├─► lcd_init()
        │   └─► ql_rtos_task_create("i2c_rtc_lcd", i2c_rtc_lcd_thread)
        │
        ├─► ql_gpio_app_dido_init()
        │   ├─► _ql_gpio_dido_init()
        │   │   └─► for each GPIO: ql_gpio_init()
        │   └─► ql_rtos_task_create("gpio_dido", gpio_dido_thread)
        │
        ├─► ql_nw_app_init()
        │   └─► ql_rtos_task_create("nw_demo", nw_thread)
        │
        ├─► ql_datacall_app_init()
        │   └─► ql_rtos_task_create("datacall", datacall_thread)
        │
        ├─► ql_mqtt_app_init()
        │   ├─► ql_sem_create(&mqtt_semp, 0)
        │   └─► ql_rtos_task_create("mqtt_client", mqtt_client_thread)
        │
        └─► ql_spi_flash_init()
            ├─► ql_spi_init(&spi_cfg)
            ├─► flash_read_id()
            ├─► ExtFlash_Read_EPROM_General(0)
            │   ├─► at25dfx_read(EPROM_GENERAL_START_ADDRESS)
            │   └─► Validate & load configuration
            ├─► ExtFlash_Read_EPROM_Schedule(0)
            └─► ExtFlash_Read_EPROM_PermanentData(0)
```

---

## Critical Timing and Synchronization

### Task Priorities
```c
#define APP_PRIORITY_LOW        5
#define APP_PRIORITY_NORMAL     APP_PRIORITY_LOW
#define APP_PRIORITY_HIGH       4
```

### Task Cycle Times
- **i2c_rtc_lcd_thread**: 1000 ms (1 second)
- **gpio_dido_thread**: 100 ms
- **mqtt_client_thread**: 100 ms (mqtt_yield)
- **Telemetry publish**: EPROM_General.LogRate seconds (configurable)

### Watchdog Configuration
```c
// Release build: Watchdog ENABLED
ql_dev_cfg_wdt(1);

// Debug build: Watchdog DISABLED
ql_dev_cfg_wdt(0);

// Watchdog GPIO: GPIO_31
// Must be toggled periodically to prevent system reset
```

---

## Memory Management

### Stack Sizes
```c
ql_init_task:         4096 bytes (4KB)
mqtt_client_thread:   8192 bytes (8KB) - Larger for JSON processing
gpio_dido_thread:     4096 bytes (4KB)
i2c_rtc_lcd_thread:   4096 bytes (4KB)
```

### Global Data Structures
```c
struct Save_Para_General EPROM_General;           // ~2-3KB
struct Save_Para_Schedule_Configuration EPROM_Schedule;  // ~1-2KB
struct Save_Para_PermanentData EPROM_PermanentData;      // ~1KB
uint8_t MqttPubBuf[CIM_MAX_SIZE_OF_MQTT_PAYLOAD];       // Configurable
```

---

## Error Handling and Recovery

### Network Failure Recovery
```c
// In mqtt_client_thread():
if(connection_lost) {
    mqtt_state = MQTT_STATE_DISCONNECTED;
    ql_rtos_task_sleep_s(5);  // Wait 5 seconds
    mqtt_state = MQTT_STATE_WAIT_NETWORK;  // Restart connection sequence
}
```

### Flash Write Protection
```c
// Wait counter prevents excessive flash writes
if(flag_flashUpdateEPROM_General) {
    if(--flag_flashUpdateEPROM_General_WaitCounter == 0) {
        ExtFlash_update_EPROM_General();
        flag_flashUpdateEPROM_General = 0;
        flag_flashUpdateEPROM_General_WaitCounter = 5;  // Reset counter
    }
}
```

### I2C Communication Failures
```c
ql_errcode_i2c_e ret = ql_I2cWrite(i2c_2, LCD_I2C_ADDRESS, ...);
if(ret != QL_I2C_SUCCESS) {
    lcd_intialized = 0;  // Mark as uninitialized
    // System will attempt re-initialization on next cycle
}
```

---

## Build and Configuration Macros

### Key Build Flags
```c
#ifdef QL_APP_FEATURE_RELEASE
    // Release build optimizations
    ql_dev_cfg_wdt(1);  // Enable watchdog
#else
    // Debug build
    ql_dev_cfg_wdt(0);  // Disable watchdog
#endif

#ifdef QL_APP_FEATURE_MQTT
    // MQTT functionality enabled
    ql_mqtt_app_init();
#endif

#ifdef QL_APP_FEATURE_SECURE_BOOT
    // Secure boot enabled
    // ql_dev_enable_secure_boot();
#endif
```

---

## Conclusion

This document provides a detailed code-level analysis of the IFPC3000 firmware's logic flow. The system is architected as a multi-threaded RTOS application with clear separation of concerns:

1. **Hardware Abstraction**: GPIO, I2C, SPI interfaces
2. **Communication Layer**: MQTT client with state machine
3. **Application Logic**: DO mode control, scheduling, astronomical calculations
4. **Persistence**: SPI Flash storage with checksums
5. **User Interface**: LCD display showing real-time status

The firmware is production-ready with proper error handling, watchdog protection, and configuration management.

---

**Document Version**: 1.0  
**Last Updated**: 2025-12-30  
**Firmware SDK Version**: LTE01R03A03_C_SDK_U_1.0.8.12
