### 1, Product picture

<img width="1000" height="1000" alt="image" src="https://github.com/user-attachments/assets/b4c666d3-e77e-4067-925b-83729addf4cb" />


### 2, Product version number

|      | Hardware | Software | Remark |
| ---- | -------- | -------- | ------ |
| 1    | V1.0     | V1.0     | latest |

### 3, product information

| **Main Chip-ESP32-P4NRW32**                  |                                                              |
| -------------------------------------------- | ------------------------------------------------------------ |
| CPU/SoC                                      | **ESP32-P4**RISC-V 32-bit dual-core processor for HP systems, running at up to 400 MHz;RISC-V 32-bit single-core processor for LP systems, running at up to 40 MHz |
| System Memory                                | 768 KB L2MEM (HP) 32 KB SRAM (LP) 8 KB TCM 32 MB PSRAM       |
| Memory                                       | 128 KB ROM (HP) 16 KB ROM (LP) 16 MB Flash                   |
| Development Environment                      | ESP-IDF、Arduino IDE                                         |
| **Screen**                                   |                                                              |
| Size                                         | 5.0 inch                                                     |
| Resolution                                   | 800*480                                                     |
| Display Panel                                | IPS Panel                                                    |
| Touch Panel                                  | Capacitive Touch, Single/5-point Touch                       |
| Viewing Angle                                | 178°                                                         |
| Brightness                                   | 400 cd/m²(Typ.)                                              |
| Color Depth                                  | 16.7M (8-bit)                                                |
| **Wireless Communication - Onboard Antenna** |                                                              |
| WiFi                                         | Support 2.4GHz(Wi-Fi6), 802.11a/b/g/n                        |
| Bluetooth                                    | Support Bluetooth 5.3 and BLE                                |
| Other                                        | Zigbee, LoRa, nRF2401, Matter, Thread (**Optional**)         |
| **Interface/Function**                       |                                                              |
| Interface                                    | USB2.0, UART, I2C, GPIO female headers, SD card holder, battery socket, speaker jack, camera header, module female headers, etc. |
| Function                                     | Audio amplifier, battery charge management, USB to UART, dual microphones, dual speakers etc. |
| **Button/LED Indicator**                     |                                                              |
| Reset Button                                 | Yes, press to reset the device                               |
| Boot Button                                  | Yes, press and hold the power button to burn the program     |
| Power Button                                 | Switch On/Off                                                |
| PWR                                          | Device power on/off indication                               |
| CHG                                          | Lithium battery charging status, Low battery state           |
| **Other**                                    |                                                              |
| Installation method                          | All around mounting holes(M3 3.2mm), embedded, shell assembly |
| Operating temperature                        | -20~70 °C                                                    |
| Storage temperature                          | -30~80 °C                                                    |
| Power Input                                  | 5V/2A, USB or UART terminal                                  |
| Active Area                                  | 155mm*87mm                                                   |
| Dimensions                                   | 180*105mm                                                    |

### Functional description of the product's internal interfaces:

| Pin Name | Description                                                  | Connector Type |
| :------- | :----------------------------------------------------------- | :------------- |
| SPK      | Output audio signals to connect to speakers. The main board comes with a power amplifier chip circuit. | PH2.0-2P       |
| PWR      | Power LED.                                                   |                |
| RST      | Reset button. Press it to reset the system.                  |                |
| boot     |                                                              |                |
| UART1    | Builds communication between Logic modules, including the serial communication module and the print module. | HY2.0-4P       |
| I2C      | Builds communication between Logic modules, including the serial communication module and the print module. | HY2.0-4P       |
| UART3-IN | Input power supply and serial communication functionality    | XH2.54-4P      |
| BAT      | Connect the lithium battery. (with battery charging circuit) | PH2.0-2P       |





### 4, Use the driver module

| Name | dependency library |
| ---- | ------------------ |
| LVGL | lvgl/lvgl@8.3.11   |

### 5,Quick Start


##### ESP-IDF starts

1.Right-click on an empty space in the project folder and select "Open with VS Code" to open the project.
![4](https://github.com/user-attachments/assets/a842ad62-ed8b-49c0-bfda-ee39102da467)



2.In the IDF plug-in, select the port, then compile and flash

<img width="1363" height="721" alt="image" src="https://github.com/user-attachments/assets/0b3b7ebd-80c6-410e-bce5-740170a6e510" />






### 6,Folder structure.
|--3D file： Contains 3D model files (.stp) for the hardware. These files can be used for visualization, enclosure design, or integration into CAD software.

|--Datasheet: Includes datasheets for components used in the project, providing detailed specifications, electrical characteristics, and pin configurations.

|--Eagle_SCH&PCB: Contains **Eagle CAD** schematic (`.sch`) and PCB layout (`.brd`) files. These are used for circuit design and PCB manufacturing.

|--example: Provides example code and projects to demonstrate how to use the hardware and libraries. These examples help users get started quickly.

|--factory_firmware: Stores pre-compiled factory firmware that can be directly flashed onto the device. This ensures the device runs the default functionality.

|--factory_sourcecode:  Contains the source code for the factory firmware, allowing users to modify and rebuild the firmware as needed.

|--libraries: Includes necessary libraries required for compiling and running the project. These libraries provide drivers and additional functionalities for the hardware.


### 7,Pin definition
ESP32-P4 5 inch and IPS Display Wiring Pins:

<img width="657" height="778" alt="image" src="https://github.com/user-attachments/assets/676326bc-5676-49b5-a047-4412eb6f06b8" />


RGB Pin connection


// Refresh Rate = 18000000/(1+40+20+800)/(1+10+5+480) = 42Hz

#define RGB_LCD_PIXEL_CLOCK_HZ     (18 * 1000 * 1000)

#define RGB_LCD_H_RES              H_size

#define RGB_LCD_V_RES              V_size

#define RGB_LCD_HSYNC              4

#define RGB_LCD_HBP                8

#define RGB_LCD_HFP                8

#define RGB_LCD_VSYNC              4

#define RGB_LCD_VBP                16

#define RGB_LCD_VFP                16

#define RGB_PIN_NUM_DISP_EN        -1

#define RGB_PIN_NUM_HSYNC          40

#define RGB_PIN_NUM_VSYNC          41

#define RGB_PIN_NUM_DE             2

#define RGB_PIN_NUM_PCLK           3

//B

#define RGB_PIN_NUM_DATA0          8

#define RGB_PIN_NUM_DATA1          7

#define RGB_PIN_NUM_DATA2          6

#define RGB_PIN_NUM_DATA3          5

#define RGB_PIN_NUM_DATA4          4

//G

#define RGB_PIN_NUM_DATA5          14

#define RGB_PIN_NUM_DATA6          13

#define RGB_PIN_NUM_DATA7          12

#define RGB_PIN_NUM_DATA8          11

#define RGB_PIN_NUM_DATA9          10

#define RGB_PIN_NUM_DATA10         9

//R

#define RGB_PIN_NUM_DATA11         19

#define RGB_PIN_NUM_DATA12         18

#define RGB_PIN_NUM_DATA13         17

#define RGB_PIN_NUM_DATA14         16

#define RGB_PIN_NUM_DATA15         15

#### ESP32-P4 and Touch Driver Wiring：

i2c address: 0x5D/0x14.(The INT pin level during reset of the GT911 touch chip determines the device address.)

INT Low Level(0x5D);

INT High Level(0x14).

<img width="749" height="452" alt="image" src="https://github.com/user-attachments/assets/93447f39-9268-4e71-bdb2-cea6d2ac844c" />


Pin connection

I2C1_SCL(IO46)

I2C1_SDA(IO45)

TP_INT_IO42(IO42)

TP_RST(IO36)

TP_RST_P12(STC8P1.2)

#### ESP32-P4 and wireless module wiring pins：
Output voltage: 3.3V Output current: 1A max. Use: The power supply communicates with the wireless module.

<img width="1027" height="593" alt="image" src="https://github.com/user-attachments/assets/8e1fd67b-45ef-4cbb-b0ce-336bb0f622dc" />


Pin connection


#define RADIO_GPIO_CLK 26

#define RADIO_GPIO_MISO 47

#define RADIO_GPIO_MOSI 48

#ifdef CONFIG_BSP_SX1262_ENABLED

#define SX1262_GPIO_BUSY 29

#define SX1262_GPIO_IRQ 31

#define SX1262_GPIO_NRST 32

#define SX1262_GPIO_NSS 30

#ifdef CONFIG_BSP_NRF2401_ENABLED

#define NRF24_GPIO_IRQ 29

#define NRF24_GPIO_CE 31

#define NRF24_GPIO_CS 32

#### ESP32-P4 and Audio out：

<img width="418" height="285" alt="image" src="https://github.com/user-attachments/assets/97d29343-41d2-4484-888c-21afcf0aaf00" />


Pin connection


I2S_LRCK(IO21);

I2S_SCLK(IO22);

I2S_SDOUT(IO23);  
#### Function Selection
When the DIP switch is set to position 1, the UART1 function is enabled. When switched to NO, the wireless module function is enabled.

<img width="284" height="147" alt="image" src="https://github.com/user-attachments/assets/4e1076ef-36fb-44da-8537-09debcb073fd" />


1	ON
UART1	Wireless Module


