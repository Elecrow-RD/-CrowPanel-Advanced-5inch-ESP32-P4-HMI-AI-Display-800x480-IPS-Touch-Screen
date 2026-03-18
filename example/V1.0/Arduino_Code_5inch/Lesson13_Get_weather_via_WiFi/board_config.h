#pragma once

#define FIRMWARE_VERSION_V1_0

/*********************** Pin define ***********************/
/**
 * SDIO Interface Pins for ESP-Hosted-MCU
 * Used for high-speed communication between ESP32-P4 (Host) and ESP32-C6 (Slave)
 */
#if defined(FIRMWARE_VERSION_V1_0)

#define WIFI_HOSTED_SDIO_PIN_CMD            (54) // SDIO Command/Response line
#define WIFI_HOSTED_SDIO_PIN_CLK            (53) // SDIO Serial Clock
#define WIFI_HOSTED_SDIO_PIN_D0             (52) // SDIO Data line 0
#define WIFI_HOSTED_SDIO_PIN_D1             (51) // SDIO Data line 1
#define WIFI_HOSTED_SDIO_PIN_D2             (50) // SDIO Data line 2
#define WIFI_HOSTED_SDIO_PIN_D3             (49) // SDIO Data line 3 (4-bit mode)
#define WIFI_HOSTED_SDIO_PIN_RESET          (20) // Hardware Reset for the ESP32-C6 co-processor

#endif
// GPIO pins for GT911 touch panel
#define Touch_GPIO_RST      (36)    // Reset pin
#define Touch_GPIO_INT      (42)    // Interrupt pin

// GPIO pins for I2C, has touch chip GT911
#define I2C_GPIO_SCL        (46)    // GPIO number used for I2C SCL (clock) line
#define I2C_GPIO_SDA        (45)    // GPIO number used for I2C SDA (data) line

// display size
#define H_size              (800)   // Horizontal resolution (X-axis)
#define V_size              (480)   // Vertical resolution (Y-axis)

// panel parameters
// Refresh Rate = 18000000/(4+8+8+800)/(4+16+16+480) = 42Hz
#define LCD_CLK_MHZ         (18)
#define LCD_HPW             ( 4)
#define LCD_HBP             ( 8)
#define LCD_HFP             ( 8)
#define LCD_VPW             ( 4)
#define LCD_VBP             (16)
#define LCD_VFP             (16)

// RGB interface Pin
#define LCD_GPIO_RST        (-1)    // LCD reset GPIO
#define RGB_PIN_NUM_DISP_EN (-1)
#define RGB_PIN_NUM_HSYNC   (40)
#define RGB_PIN_NUM_VSYNC   (41)
#define RGB_PIN_NUM_DE      ( 2)
#define RGB_PIN_NUM_PCLK    ( 3)

#define RGB_PIN_NUM_DATA0   ( 8)
#define RGB_PIN_NUM_DATA1   ( 7)
#define RGB_PIN_NUM_DATA2   ( 6)
#define RGB_PIN_NUM_DATA3   ( 5)
#define RGB_PIN_NUM_DATA4   ( 4)
#define RGB_PIN_NUM_DATA5   (14)
#define RGB_PIN_NUM_DATA6   (13)
#define RGB_PIN_NUM_DATA7   (12)
#define RGB_PIN_NUM_DATA8   (11)
#define RGB_PIN_NUM_DATA9   (10)
#define RGB_PIN_NUM_DATA10  ( 9)
#define RGB_PIN_NUM_DATA11  (19)
#define RGB_PIN_NUM_DATA12  (18)
#define RGB_PIN_NUM_DATA13  (17)
#define RGB_PIN_NUM_DATA14  (16)
#define RGB_PIN_NUM_DATA15  (15)
/*********************** Pin define ***********************/
