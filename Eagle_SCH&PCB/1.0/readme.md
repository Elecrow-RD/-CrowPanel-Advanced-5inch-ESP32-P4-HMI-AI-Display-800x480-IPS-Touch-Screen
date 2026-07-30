# ESP32-P4 5.0-Inch HMI Product Hardware Driver Guide

| Document Attribute | Content |
|---|---|
| Document Version | V1.0 |
| Applicable Hardware | ESP32-P4 Display 5.0 inch V1.0 |
| Date | 2026-07-29 |
| Author | OpenAI Codex (compiled from the project schematics and verified sample code) |
| Document Status | Driver cross-validation baseline for maintenance, porting, and onboarding handoff |

## 1. Document Purpose and Rules for Adopting Conclusions

This document cross-checks `1.0/ESP32-P4 Display 5.0 inch V1.0.sch/.pdf/.brd` against the verified ESP-IDF examples under `idf-code`, covering onboard components, onboard interfaces, and external modules validated by the examples.

Conclusions are prioritized as follows:

1. **Actual configuration in successfully tested code**: Highest priority and used as the porting baseline.
2. **EAGLE schematic nets and component values**: Used to verify electrical connections, power supplies, and multiplexing relationships.
3. **Code comments, README files, or general component defaults**: Used only as supplementary information and must not override the actual code.

Evidence levels:

- **A (Consistent)**: The code matches the schematic connections.
- **B (Code Baseline)**: The code has been verified, but the component is located on an expansion interface/module, or the schematic does not identify the chip inside the module.
- **C (Design Information)**: Present only in the schematic, with no active driver provided by the project; verify against the physical hardware and component datasheet during maintenance.

> Note: The project is a collection of multiple independent course projects. It does not imply that all peripherals can operate concurrently without conflicts in the same firmware. Section 6 lists the multiplexing conflicts that must be addressed.

## 2. Software and Hardware Baseline

- Main MCU: ESP32-P4NRW32, dual-core RISC-V; code target `CONFIG_IDF_TARGET=esp32p4`.
- Software framework: ESP-IDF + FreeRTOS; primarily uses the newer `driver/i2c_master.h`, `driver/i2s_std.h`, `driver/i2s_pdm.h`, `esp_lcd`, SDMMC, TinyUSB, ESP-Hosted, esp-video/V4L2, LVGL, and RadioLib.
- External Flash: W25Q128JVSIQ, 16 MB; the project is configured for 16 MB with the QIO option (the generated configuration may display the string `dio`; use the actual build output of the current project when flashing).
- PSRAM: 32 MB PSRAM integrated into the ESP32-P4NRW32 package; the project enables HEX mode at 200 MHz. LCD double buffering and camera frame buffers depend on PSRAM.
- Wireless coprocessor: ESP32-C6-MINI-1-N4, communicating with the P4 over 4-bit SDIO; ESP-Hosted provides Wi-Fi/Bluetooth capabilities.
- Board-control MCU: STC8H1K17-36I, providing backlight and audio enable control, touch/camera reset, button/mode status, and battery information to the P4 over I²C address `0x2F`.

## 3. Peripheral Overview

| Category | Peripheral/Component | Interface and Main Pins | Driver Status | Level |
|---|---|---|---|---|
| Main Controller | ESP32-P4NRW32 | On-chip peripheral matrix | Main controller for all course projects | A |
| Storage | W25Q128JVSIQ 16 MB | Dedicated Flash bus | Managed by Bootloader/ESP-IDF | A |
| Storage | In-package 32 MB PSRAM | Dedicated HEX PSRAM | ESP-IDF, 200 MHz | A |
| Board Control | STC8H1K17-36I | I²C0 GPIO45/46, address 0x2F | Custom register-based BSP | A |
| Display | 5.0-inch 800×480 RGB LCD | RGB565 GPIO2–19, 40, 41 | `esp_lcd` + LVGL | A |
| Backlight | MT9201 boost converter + STC8 PWM | STC8 P1.1/P3.7 | Indirect I²C control | A |
| Touch | GT911 touch module (via FPC2) | I²C0 GPIO45/46, RST=36, INT=42 | `esp_lcd_touch_gt911` | B |
| Storage | MicroSD | SDMMC GPIO43/44/39, 1-bit | FATFS/SDMMC | A |
| Sensor | DHT20 (external I²C) | I²C0 GPIO45/46, address 0x38 | Custom BSP | B |
| Sensor | SC2336 camera module | MIPI CSI 2-lane; SCCB GPIO33/34 | esp-video/V4L2 | B |
| Audio Input | MMICT5838 PDM microphone | CLK=24, DATA=25 | I²S PDM RX | A |
| Audio Output | Dual NS4168 amplifiers | BCLK=22, LRCK=21, SD=23 | I²S STD TX | A |
| Wireless | ESP32-C6-MINI-1-N4 | SDIO GPIO49–54, RESET=20 | ESP-Hosted | A |
| RF Expansion | SX1262 module | SPI GPIO26/47/48; GPIO29–32 | RadioLib | B |
| RF Expansion | nRF24L01 module | SPI GPIO26/47/48; GPIO29/31/32 | RadioLib | B |
| USB | USB1 Type-C + CH340K | USB-UART download/logging | Automatic chip/serial-port control | A |
| USB | USB2 Type-C | P4 USB DP/DM | TinyUSB HID Device | A |
| Serial Port | Expansion UART | UART2 GPIO47/48, 115200 8N1 | ESP-IDF UART | A |
| Expansion Interface | UART_IN | GPIO27/28, through MOS level shifting | Not used by the current UART example | C |
| Expansion Interface | I²C_OUT | GPIO45/46, through BSS138 bidirectional level shifting | Shares the system I²C bus | A |
| Expansion Interface | SPI/wireless socket | GPIO26, 47, 48, 29–32 | Verified in Courses 14/15 | B |
| HMI Input | RESET and BOOT buttons | CHIP_PU, SPI_BOOT | Hardware boot control | A |
| Indicator | GPIO demo LED/external load | Verified on GPIO48 | Push-pull output | B |
| Indicator | Charging/power RGB LED | TP4059/STC8 P3.5/P3.6 | Managed by hardware/STC8 | C |
| Power | TP4059, MT3406, ME6211, MT9201 | 5 V, VBAT, 3.3/2.8/1.8/1.2 V | Primarily hardware-managed | C |

## 4. ESP32-P4 GPIO/Function Summary

| GPIO | Actual Function | Electrical/Multiplexing Notes |
|---:|---|---|
| 0, 1 | 32.768 kHz crystal | Not used as general-purpose GPIOs |
| 2 | LCD DE | RGB output |
| 3 | LCD PCLK | RGB output, 18 MHz, sampled on falling edge |
| 4–8 | LCD B7–B3 | RGB565 blue data |
| 9–14 | LCD G7–G2 | RGB565 green data |
| 15–19 | LCD R7–R3 | RGB565 red data |
| 20 | ESP32-C6 EN/RESET | Configured by ESP-Hosted as active-high reset; do not use as a general-purpose GPIO |
| 21 | I²S LRCK/WS | Shared by both amplifiers |
| 22 | I²S BCLK | Shared by both amplifiers |
| 23 | I²S SDOUT | Shared by both amplifiers |
| 24 | PDM MIC CLK | Output |
| 25 | PDM MIC DATA | Input |
| 26 | Expansion SPI SCK | Shared by SX1262/nRF24 |
| 27, 28 | UART_IN TX/RX | Connected to J10 through level shifting; not used by the current UART example |
| 29–32 | Wireless expansion control/UART2 | Function depends on the module type; see 4.11 |
| 33, 34 | Camera SCCB SDA/SCL | I²C1, level-shifted onboard through BSS138 |
| 36 | GT911 RST | Directly controlled by GPIO in the code; the schematic also reserves an STC8 reset path |
| 39, 43, 44 | MicroSD D0/CLK/CMD | SDMMC 1-bit |
| 40, 41 | LCD HSYNC/VSYNC | RGB output |
| 42 | GT911 INT | Input, managed by the touch library |
| 45, 46 | System I²C0 SDA/SCL | Shared by GT911, STC8, DHT20/expansion devices; open-drain, 400 kHz |
| 47, 48 | SPI MISO/MOSI or UART TX/RX | Multiplexed by the SGM3005 analog switch and STC8 mode detection; cannot be used concurrently |
| 49–54 | ESP32-C6 SDIO D3–D0/CLK/CMD | Dedicated, 4-bit, 40 MHz |

## 5. Peripheral Driver Details

### 5.1 ESP32-P4, Flash, and PSRAM

**Hardware Connections**

- U7 is the ESP32-P4NRW32; the 40 MHz main crystal and 32.768 kHz RTC crystal are connected as shown in the schematic.
- IC4 is the W25Q128JVSIQ, with a capacity of 128 Mbit (16 MB), connected to the P4’s dedicated Flash pins.
- The ESP32-P4NRW32 includes 32 MB of PSRAM, which is not driven by the project as a standard GPIO peripheral.

**Driver Method and Parameters**

- Boot, Flash cache, and PSRAM initialization are all handled by the ESP-IDF bootloader/SOC layer.
- The project targets ESP32-P4 with 16 MB Flash; PSRAM uses HEX mode at 200 MHz with startup memory testing enabled.
- The LCD uses two PSRAM frame buffers, and the camera uses two PSRAM video buffers. PSRAM must not be disabled when porting.

**Software Dependencies**: ESP-IDF bootloader, SPI Flash, heap_caps, FreeRTOS.

### 5.2 System I²C0 and STC8H1K17 Board-Control MCU

**Connections and Electrical Characteristics**

- P4 GPIO45=SDA and GPIO46=SCL; I²C0 master.
- The schematic uses BSS138 bidirectional level shifting to connect the 3.3 V-side interface. The code allows internal pull-ups, and the board also includes 4.7 kΩ pull-ups.
- I²C is an open-drain bus; never configure GPIO45/46 as push-pull outputs.

**Code Baseline**

```c
i2c_master_bus_config_t bus = {
    .i2c_port = 0, .sda_io_num = 45, .scl_io_num = 46,
    .glitch_ignore_cnt = 7, .flags.enable_internal_pullup = true,
};
// For each slave: 7-bit address, SCL = 400000 Hz
```

- STC8 7-bit address: `0x2F`.
- Register regions: `0x00` battery information; `0x10+n` input GPIO; `0x18+n` output GPIO; `0x20+n` PWM duty cycle.
- The P4 reads the battery structure byte by byte to prevent a single continuous read from failing with the current STC8 firmware.
- The I²C API timeout is typically 1000 ms.

**STC8 Logical Mapping**

| BSP Index | STC8 Pin/Schematic Function | Purpose |
|---|---|---|
| Input 0 | P1.7 / `SW_SPI_UART_P17` | SPI/UART mode detection |
| Output 0 | P1.2 / `TP_RST_P12` | Reserved touch reset path |
| Output 1 | P1.3 / `CSI_RESET_P13` | Camera reset |
| Output 2 | P1.0 / `AUDIO_OUT_SD_P10` | Amplifier shutdown; see the audio section for the active-low relationship |
| Output 3 | P3.7 / `LCD_BK_POWER_P37` | Backlight power switch |
| PWM 0 | P1.1 / `LCD_BK_EN_P11` | Backlight PWM |
| ADC | P3.2/ADC10 | Battery divider sampling |
| LED | P3.5/P3.6 | Power red/green LEDs |

**Software Dependencies**: ESP-IDF `driver/i2c_master.h`, custom `bsp_i2c`, `bsp_stc8h1kxx`.

### 5.3 5.0-Inch RGB LCD and Backlight

**RGB Data Connections**

- Resolution: 800×480; 16-bit RGB565.
- B[3:7] = GPIO8,7,6,5,4; G[2:7] = GPIO14,13,12,11,10,9; R[3:7] = GPIO19,18,17,16,15.
- PCLK=GPIO3, DE=GPIO2, HSYNC=GPIO40, VSYNC=GPIO41; there is no separate `DISP_EN` GPIO.

**Key Timing Parameters**

| Parameter | Value |
|---|---:|
| Pixel clock | 18 MHz |
| H active / HSYNC / HBP / HFP | 800 / 4 / 8 / 8 |
| V active / VSYNC / VBP / VFP | 480 / 4 / 16 / 16 |
| PCLK edge | `pclk_active_neg=true`, sampled on falling edge |
| Frame buffers | 2, located in PSRAM |
| DMA burst | 64 bytes |

Based on the current code parameters, the calculated refresh rate is approximately `18 MHz / 820 / 516 ≈ 42.6 Hz`. An older comment in the header file uses a different set of porch values; **use the actual macros and `panel_config.timings` as authoritative**.

**Initialization Sequence**

1. Initialize the system I²C and STC8.
2. Enable backlight power through the STC8; an initial PWM value of 0 is recommended.
3. Create `esp_lcd_new_rgb_panel()`, then reset/initialize the LCD.
4. Register the LVGL display, and gradually increase the backlight after double buffering has started.

**Backlight Hardware**: STC8 P3.7 controls backlight power, while P1.1 outputs PWM to drive the MT9201 boost constant-current chain. The code’s `set_lcd_blight(0..100)` function actually writes STC8 PWM0 and does not use the P4 LEDC; `LCD_GPIO_BLIGHT=-1` is the correct configuration.

**Software Dependencies**: ESP-IDF `esp_lcd` RGB panel, PSRAM, `esp_lvgl_port`/LVGL, custom STC8 BSP.

### 5.4 GT911 Capacitive Touch

**Connections**

- The touch controller is located on the external display/touch module and connected through FPC2. The board-level schematic shows only the FPC and must not be misinterpreted as indicating an onboard GT911.
- I²C0: SDA=45, SCL=46, 400 kHz.
- RST=GPIO36, active low; INT=GPIO42, configured level 0.
- Touch coordinates are 800×480; XY is not swapped, and neither axis is mirrored.

**Address and Initialization**

- First attempt the driver component macro `ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS` (typically 0x5D).
- If initialization fails, try `ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP` (typically 0x14).
- The I²C command width is 16 bit, with the control phase disabled.

```c
esp_lcd_touch_config_t tp = {
    .x_max=800, .y_max=480, .rst_gpio_num=36, .int_gpio_num=42,
    .levels={.reset=0, .interrupt=0},
};
```

**Difference Note**: The schematic also retains a hardware path from STC8 P1.2 to TP_RST, while the verified code directly uses P4 GPIO36. Use GPIO36 as the maintenance and porting baseline unless a new board revision explicitly switches reset control to the STC8.

**Software Dependencies**: `esp_lcd_panel_io_i2c`, `esp_lcd_touch`, `esp_lcd_touch_gt911`, system I²C BSP.

### 5.5 MicroSD

**Connection and Operating Mode**

- SDMMC Host slot 0, 1-bit mode: CLK=GPIO43, CMD=GPIO44, D0=GPIO39.
- The schematic also includes pull-ups/reserved connections associated with D1/D2/D3 and CS, but the verified code explicitly sets `slot_config.width=1`; do not change it to 4-bit mode without validation during porting.
- The internal pull-up flag is used, and the board also includes 10 kΩ pull-ups from CMD/D0/CS to 3.3 V.

**Key Parameters**

- Maximum bus frequency: 10 MHz.
- Mount point: `/sdcard`; FATFS; no automatic formatting on failure.
- Maximum simultaneously open files: 5; allocation unit: 16 KiB.

**Initialization Example**

```c
host.max_freq_khz = 10000;
slot.clk = 43; slot.cmd = 44; slot.d0 = 39;
slot.width = 1;
slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
```

**Software Dependencies**: ESP-IDF SDMMC, FATFS, VFS.

### 5.6 DHT20 Temperature and Humidity Module

**Connection**: External I²C module connected to system I²C0 GPIO45/46; 7-bit address `0x38`; 400 kHz. This component is not included in the mainboard BOM and is an interface-validation module.

**Initialization/Measurement Timing**

- Read-status command `0x71`; the calibration status must satisfy `(status & 0x18)==0x18`.
- If the device is not ready, reset registers `0x1B`, `0x1C`, and `0x1E` in sequence.
- Measurement command `{0xAC,0x33,0x00}`; wait at least 80 ms.
- The busy flag is status bit 7; total timeout is 1000 ms.
- Read 7 bytes; CRC-8 initial value `0xFF`, polynomial `0x31`.

**Software Dependencies**: System I²C BSP, FreeRTOS delays, `esp_timer`.

### 5.7 SC2336 MIPI CSI Camera

**Hardware Connections**

- The camera module connects through 24-pin FPC3 using 2-lane MIPI CSI: D0±, D1±, CLK±.
- SCCB/I²C1: SDA=GPIO33, SCL=GPIO34, level-shifted to the module side through BSS138.
- RESET is provided by STC8 P1.3 through level shifting; the P4 code manages it through the board-control MCU/hardware chain.

**Verified Configuration**

- Sensor: SC2336; MIPI interface detected automatically.
- Format: MIPI RAW8, 1024×600, 30 fps; ISP pipeline enabled.
- XCLK uses the ESP clock router rather than LEDC.
- The device layer opens `ESP_VIDEO_MIPI_CSI_DEVICE_NAME` and uses V4L2 `VIDIOC_*` operations to request/queue two PSRAM buffers and start streaming.
- SCCB port 1, GPIO33/34; the default SCCB transfer timeout is 500 ms.

**Difference Note**: The schematic defines only a generic CSI interface and cannot identify the sensor model. The project’s verified configuration explicitly uses the SC2336, so the porting baseline must use the SC2336 and `sc2336_custom.json`. When replacing the camera module, Kconfig, the sensor driver, IPA JSON, lane/format settings, and power sequencing must all be updated.

**Software Dependencies**: ESP-IDF camera controller/ISP, esp-video, V4L2, SC2336 sensor driver, PSRAM, LVGL.

### 5.8 PDM Digital Microphone

**Connection**: U175 (identified in the schematic library as part of the MMICT5838 series), CLK=GPIO24, DATA=GPIO25, with 3.3 V filtered through a ferrite bead; the hardware L/R strap determines the channel.

**Configuration**

- ESP-IDF I²S PDM RX; sample rate 16 kHz; fixed 16 bit.
- Mono, LEFT slot; downsample `I2S_PDM_DSR_8S`.
- GPIO24 outputs the PDM clock, while GPIO25 receives data; DMA is managed by the I²S channel.

**Software Dependencies**: `driver/i2s_pdm.h`, FreeRTOS; the file-recording example also depends on FATFS/SD.

### 5.9 Dual NS4168 I²S Amplifiers and Speakers

**Connections**

- GPIO22=BCLK, GPIO21=LRCK/WS, GPIO23=SDATA; both NS4168 amplifiers share the bus.
- The left and right amplifiers are selected by their respective hardware channel configurations and drive two PH2.0 speaker connectors.
- Amplifier SD/enable is controlled by STC8 output 2 (P1.0). The code calls `stc8_gpio_set_level(..., !state)`, meaning the BSP “enable” parameter corresponds to a low level on the hardware control pin.

**Configuration**

- I²S STD TX, 16 kHz, 16-bit, stereo, both slots.
- MCLK multiple 256, but MCLK is not output through a GPIO; BCLK/WS are not inverted.

**Note**: `AUDIO_GPIO_CTRL=-1` is correct because the P4 does not directly control amplifier enable. When stopping playback, mute/assert shutdown before stopping the clock to avoid audible pops.

**Software Dependencies**: `driver/i2s_std.h`, STC8 BSP; the local music example additionally depends on ESP32-audioI2S and SD/FATFS.

### 5.10 ESP32-C6 Wireless Coprocessor

**Connections**

- 4-bit SDIO: D3=GPIO49, D2=50, D1=51, D0=52, CLK=53, CMD=54.
- RESET/EN=GPIO20, configured as active-high reset.
- The C6 module includes 4 MB Flash and a 2.4 GHz antenna; do not place copper or shielding metal in the antenna area.

**ESP-Hosted Configuration**

- Host=P4, slave target=ESP32-C6; SDIO slot 1, 4-bit, 40 MHz.
- The host may reset the slave each time it starts; a typical reset delay is 1500 ms.
- The upper layer uses the standard `esp_wifi_*` / Bluedroid APIs, forwarded through the ESP-Hosted RPC/transport layer.

**Maintenance Requirements**: The P4 host component version must match the C6 slave firmware version. If Wi-Fi malfunctions, first verify the C6 firmware, GPIO20 reset, and SDIO GPIO49–54 signals rather than troubleshooting only the TCP/IP layer.

**Software Dependencies**: `esp_hosted`, ESP-IDF Wi-Fi, esp_netif, event loop, NVS, optional Bluedroid.

### 5.11 SX1262 and nRF24L01 Wireless Expansion

The two types of RF modules use the same expansion socket/shared SPI and are mutually exclusive in assembly or functionality.


**Shared SPI**: SPI3_HOST; SCK=GPIO26, MISO=47, MOSI=48; 8 MHz; RadioLib custom ESP-IDF HAL.

| Module | GPIO29 | GPIO30 | GPIO31 | GPIO32 |
|---|---|---|---|---|
| SX1262 | BUSY | NSS/CS | DIO1/IRQ | NRST |
| nRF24L01 | IRQ | Unused | CE | CS |

**Verified SX1262 Parameters**

- `begin(915.0, 125.0, 7, 7, PRIVATE_SYNC_WORD, 22, 8, 1.6)`.
- That is, 915 MHz, BW 125 kHz, SF7, CR 4/7, 22 dBm, preamble 8, and TCXO 1.6 V; DIO1 interrupt callbacks are used for transmit and receive.
- **Regulatory risk**: 915 MHz/22 dBm may not comply with spectrum and transmit-power requirements in mainland China. Before mass production, these settings must be changed to parameters permitted in the target region, and the required certification must be completed.

**Verified nRF24L01 Parameters**

- `begin(2400, 250, 0, 5)`: 2400 MHz, 250 kbps, 0 dBm, 5-byte address.
- Pipe address `{0x01,0x02,0x11,0x12,0xFF}`; packet reception is driven by an IRQ callback.

**Software dependencies**: RadioLib, ESP-IDF SPI master, GPIO ISR, FreeRTOS.

### 5.12 UART and USB

**USB1 / Download UART**

- J1 Type-C connects to the CH340K for firmware download and serial logging; the automatic download circuit controls BOOT/RESET through the UMH3NTN.
- This path is managed by the USB-UART chip and ROM bootloader, and the application typically requires no additional driver.

**USB2 / Native USB**

- J16 Type-C connects directly to P4 USB DP/DM through series 22 Ω resistors, and the code uses the internal PHY.
- TinyUSB HID Mouse has been verified: device string `Advance-P4 HID Mouse`, IN endpoint `0x81`, 16 bytes, 10 ms polling, declared 100 mA, and remote wakeup.

**Verified Expansion UART Baseline**

- UART2: TX=GPIO47, RX=GPIO48; 115200, 8N1, no flow control; RX buffer 2048 bytes.
- In the schematic, GPIO47/48 are switched between the UART and SPI paths through the SGM3005.

**Discrepancy note**: `bsp_uart.h` also defines GPIO27/28 and contains incorrect/outdated comments, but `uart_init()` actually uses the GPIO47/48 macros. Follow the actual call. GPIO27/28 correspond to another set of `UART_IN` interfaces in the schematic. Enabling them requires a separate UART configuration and verification through actual testing.

### 5.13 Buttons, LEDs, and Expansion Interfaces

- K4: P4 RESET/CHIP_PU, active-low reset; this is a hardware button and is not used as an application input.
- K3: SPI_BOOT; a low level affects the boot mode. The application must not continuously drive the associated net.
- The GPIO demo project configures GPIO48 as a standard push-pull output with no pull-up, pull-down, or interrupt, and toggles an external LED/load. This function is multiplexed with SPI MOSI/UART RX and is suitable only for standalone Lesson02 verification.
- Power/charging LEDs are primarily managed by the TP4059 charging status signals and STC8 P3.5/P3.6. The P4 project has no direct LED control protocol.
- J13 I²C_OUT exposes 3.3 V, SDA, SCL, and GND and shares the bus with the onboard touch controller and STC8.
- Expansion sockets such as J9/J11/J7 expose SPI/UART/wireless-control GPIOs. External modules must use 3.3 V logic, and the risk that the socket power pin may supply 5 V must be checked.

### 5.14 Power, Charging, and Battery Monitoring

**Power Tree (Schematic)**

- The USB/5 V input passes through a Schottky diode and P-MOS power path to form VCC5V/VDD5V.
- TP4059 (U2) handles single-cell lithium battery charging; J3 is the battery connector, and VBAT is also routed through a voltage divider to the STC8 ADC for monitoring.
- MT3406/TLV62569-class DC-DC converters generate rails such as 3.3 V and 1.2 V; ME6211 LDOs generate 1.8 V, 2.8 V, and 3.3 V analog/peripheral power rails.
- MT9201 generates the boosted LCD LED backlight supply; the audio, microphone, and camera power rails are isolated using ferrite beads/analog ground.

**Software Boundaries**

- The P4 does not configure the PMIC directly; battery information is read from the STC8 at address 0x2F, starting at register `0x00`.
- Power switching, charging current limiting, and each DC-DC output are set by hardware resistors and capacitors. Any BOM change requires recalculation and cannot be compensated through P4 drive settings.
- The current repository does not include STC8 firmware source code. The STC8’s internal ADC conversion, PWM frequency, and battery structure ABI are external dependencies. The protocol version must be frozen before revising the board or replacing the STC8 firmware.

## 6. Schematic/Code Discrepancies and Conflict Matrix

### 6.1 Confirmed Discrepancies

| Item | Schematic/Comments | Verified Code | Final Selection | Possible Cause |
|---|---|---|---|---|
| UART pins | GPIO27/28 `UART_IN` exists, and header comments also mention it | UART2 actually uses GPIO47/48 | GPIO47/48 | Confusion between two expansion UART sets; comments were not synchronized |
| Touch reset | The schematic retains paths associated with STC8 P1.2 and P4 GPIO36 | The GT911 driver directly sets RST=36 | GPIO36 | Board-level compatibility/revision provision |
| LCD refresh comment | Header comments specify an old porch combination | Macros are H 4/8/8 and V 4/16/16 | Actual macros, approximately 42.6 Hz | Comments were not updated with the parameters |
| MicroSD width | The schematic card socket supports additional data/CS nets | Only GPIO39/43/44, 1-bit | SDMMC 1-bit | Tradeoff for stability or pin resources |
| Camera model | The schematic only shows a generic CSI FPC | Configured as SC2336 RAW8 1024×600@30 | SC2336 | Module model is determined by assembly and software |
| Backlight PWM | The code header retains P4 `LCD_GPIO_BLIGHT=-1`/30 kHz macros | STC8 PWM0 is actually written | Indirect control through STC8 | The BSP evolved from a direct-control design, leaving obsolete macros behind |
| Flash mode | sdkconfig contains both a QIO option and the string `dio` | Determined by the actual build tools | Check generated parameters before building | Differences caused by sdkconfig version migration/generated settings |

### 6.2 Resource Conflicts

| Conflicting Resource | Function A | Function B | Handling Principle |
|---|---|---|---|
| GPIO47/48 | SPI MISO/MOSI (SX1262/nRF24) | UART2 TX/RX or Lesson02 GPIO48 | All three are mutually exclusive; unload the previous driver and verify the SGM3005/STC8 mode before switching |
| GPIO29~32 | SX1262 control | nRF24 control/wireless UART | Different module configurations are mutually exclusive; do not reuse the pin semantics from the other configuration |
| GPIO45/46 | GT911 + STC8 | DHT20/external I²C | They can share the bus, but addresses must not conflict; standardize on 400 kHz or reduce the frequency to one supported by all devices |
| GPIO33/34 | Camera SCCB | External I²C2 | They must not be reconfigured by another driver while the camera is operating |
| GPIO39/43/44 | MicroSD | General-purpose GPIO | Dedicated while mounted; unmount the file system before hot removal |
| GPIO49~54 | ESP-C6 SDIO | General-purpose GPIO/other SDMMC | Fully dedicated while Wi-Fi/Bluetooth is enabled |
| PSRAM/bandwidth | LCD double buffering | Camera double buffering/ISP | Budget memory and DMA bandwidth when running simultaneously; avoid increasing resolution/buffer count without analysis |

## 7. Risks and Precautions

1. **I²C electrical characteristics**: GPIO45/46 form an open-drain bus with multiple onboard/internal pull-ups. Excessive pull-ups on expansion modules will reduce the effective resistance and cause excessive low-level sink current. For long wiring or many devices, reduce the bus speed to 100 kHz and verify the waveform.
2. **Logic levels and power**: P4 GPIO uses 3.3 V logic; direct 5 V input is prohibited. A 5 V power pin on an expansion socket does not mean that its signal pins are 5 V tolerant.
3. **Backlight power**: The MT9201 drives an LED string. Do not bypass the constant-current/feedback circuitry and drive it directly from a GPIO. Start at low brightness during power-up, verify the display parameters, and then increase it gradually.
4. **Amplifier capability**: The NS4168 provides power output. Speaker impedance, power rating, and stereo wiring must match. Do not ground the differential outputs or connect the left and right channels in parallel.
5. **RF regulations**: The SX1262 example settings of 915 MHz/22 dBm are only verification parameters and must not be treated directly as a mass-production configuration for mainland China.
6. **USB role**: The USB2 example operates as Device/HID and uses the internal PHY. When changing to Host/OTG, recheck the VBUS switch, Type-C role resistors, and power direction.
7. **SD hot-plugging**: The existing code does not implement a card-detect GPIO or a complete hot-plug state machine. Stop file access and unmount FATFS before removing the card.
8. **Camera high-speed signals**: MIPI CSI is a controlled-impedance differential link. Do not add flying wires or arbitrary test points during maintenance. The module voltage, lane count, and clock must match.
9. **STC8 protocol dependency**: The repository does not include the STC8 firmware. The register map can only use the P4 BSP as the protocol baseline. Compatibility testing is required when upgrading the STC8 firmware.
10. **Error handling**: Some course BSPs ignore low-level return values, such as in the GPIO demo and backlight writes. When integrating them into production firmware, add return-value propagation, retries, and graceful degradation.
11. **Course project isolation**: The `sdkconfig` and component versions may differ among Lessons. During migration, merge functions into one main project rather than simply concatenating multiple `sdkconfig` files.

## 8. Recommended Initialization and Shutdown Sequences

**Startup**

1. ESP-IDF completes Flash/PSRAM, NVS, and basic system initialization.
2. Initialize I²C0 (45/46), register the STC8 (0x2F), and read board-control status and battery information.
3. Keep the amplifier disabled and LCD PWM=0; initialize GT911/DHT20 as needed.
4. Initialize the RGB LCD and LVGL. After the first frame stabilizes, enable the backlight power and gradually increase PWM.
5. Mount the SD card and initialize I²S/PDM, the camera, or wireless expansion as needed.
6. When Wi-Fi/BT is required, reset the ESP32-C6 and start ESP-Hosted SDIO in 4-bit 40 MHz mode.

**Shutdown/Low Power**

1. Stop networking, the camera, and file writes, and unmount the SD card.
2. Fade out the audio and use the STC8 to disable the amplifier.
3. Reduce the backlight PWM to 0, then disable the backlight power.
4. Stop LCD DMA/I²S/Camera and release PSRAM buffers.
5. Shut down the C6 or place it in low-power mode according to the product strategy; finally, handle main-controller sleep/power hold.

## 9. Porting Checklist

- [ ] The target chip is ESP32-P4, with Flash=16 MB and PSRAM=32 MB/200 MHz configured correctly.
- [ ] GPIO2~19, 40, and 41 are not occupied by other functions, and LCD timing is configured according to the actual macros.
- [ ] I²C0 uses GPIO45/46 at 400 kHz; STC8 0x2F, GT911 0x5D/0x14, and DHT20 0x38 have no address conflicts.
- [ ] The backlight and amplifier are controlled through the STC8 rather than by incorrectly using the P4’s `-1` control pin.
- [ ] MicroSD remains in 1-bit, 10 MHz mode using GPIO39/43/44.
- [ ] PDM MIC uses 24/25; I²S OUT uses 21/22/23; the sample format matches the application audio data.
- [ ] The camera is SC2336 MIPI RAW8 1024×600@30, SCCB uses 33/34, and the IPA JSON is deployed with the project.
- [ ] ESP-C6 SDIO uses 49~54, RESET uses 20, and 4-bit 40 MHz mode is configured; host/slave firmware versions match.
- [ ] Select only one of SPI wireless, UART2, and the GPIO48 LED; release SPI/UART/GPIO resources before switching drivers.
- [ ] External modules use 3.3 V logic; supply current, antenna clearance, and speaker impedance meet hardware requirements.
- [ ] Fault handling is implemented for all initialization return values, I²C timeouts, SD card removal, C6 reset, and camera stream interruption.

## 10. Evidence Index

| Conclusion | Primary Evidence Files |
|---|---|
| Components, nets, power, and interfaces | `1.0/ESP32-P4 Display 5.0 inch V1.0.sch` and the PDF/BRD files with the same name |
| LCD pins and timing | `idf-code/Lesson07-Turn_on_the_screen/peripheral/bsp_illuminate/` |
| I²C and STC8 protocol | `idf-code/Lesson07-Turn_on_the_screen/peripheral/bsp_i2c/`, `bsp_stc8h1kxx/` |
| GT911 | `idf-code/Lesson05-Touchscreen/peripheral/bsp_display/` |
| TinyUSB HID | `idf-code/Lesson06-USB2.0/peripheral/bsp_usb/` |
| SDMMC | `idf-code/Lesson08-SD_Card_File_Reading/peripheral/bsp_sd/` |
| DHT20 | `idf-code/Lesson10-Temperature_and_Humidity/peripheral/bsp_dht20/` |
| I²S/PDM | `idf-code/Lesson11-Playback_After_Recording/peripheral/bsp_audio/`, `bsp_mic/` |
| Camera | `idf-code/Lesson13-Camera_Real-Time/`, especially `sdkconfig.defaults` and `bsp_camera/` |
| SX1262/nRF24 | `bsp_wireless/` under `idf-code/Lesson14_*` and `idf-code/Lesson15_*` |
| ESP32-C6/ESP-Hosted | `idf-code/Lesson17-Wi-Fi_function/*/sdkconfig.defaults.esp32p4` |
| UART | `idf-code/Lesson04-Serial_port_usage/peripheral/bsp_uart/` |

---

The conclusions in this document apply to the repository’s current V1.0 schematic and existing verified code. Any change to the PCB version, display module, camera module, STC8 firmware, or ESP-IDF/component version requires repeating the three-way cross-validation among “schematic nets—BSP macros—runtime logs/oscilloscope measurements” and updating the version of this document.