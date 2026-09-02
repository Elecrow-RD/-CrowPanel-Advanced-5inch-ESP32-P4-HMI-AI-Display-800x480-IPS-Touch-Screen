/**
 * @file board_config.h
 * @brief Defines the hardware and library configuration used by the lesson.
 *
 * This file belongs to the Lesson15_RX_nRF2401_Wireless_RF_Module course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Board Config Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

#pragma once

/*********************** Pin define ***********************/
/*wireless module GPIO pins*/
/* SPI BUS */
#define PIN_SPI_SCK         (26)
#define PIN_SPI_MOSI        (48)
#define PIN_SPI_MISO        (47)
/* LoRa Interfaces */
#define LORA_RESET          (32)    // RST
#define LORA_DIO1           (31)    // IRQ
#define LORA_DIO2           (29)    // BUSY
#define LORA_CS             (30)
/* nRF2401 Interfaces */
#define NRF24_IRQ           (29)    
#define NRF24_CE            (31)
#define NRF24_CS            (32)
/*wireless module GPIO pins*/

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
