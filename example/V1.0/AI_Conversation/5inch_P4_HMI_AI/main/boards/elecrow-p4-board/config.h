/*---------------------------------------------------------------
 * Teaching module overview: Board support
 * This file groups the config responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

/*---------------------------------------------------------------
 * Default Wi-Fi configuration
 * Seed the credential manager when the device has no saved network.
 * Change only these two values for a different course network.
 *--------------------------------------------------------------*/
#define DEFAULT_WIFI_SSID     "elecrow888"
#define DEFAULT_WIFI_PASSWORD "elecrow2014"

/*---------------------------------------------------------------
 * Audio signal configuration
 * Keep capture and playback at the speech service's 16 kHz rate,
 * and map the independent I2S output and PDM input signal groups.
 *--------------------------------------------------------------*/
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

// I2S audio pins
#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_NC  // Master clock pin (not used, set to NC)
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_21  // Word select pin (LRCLK, left/right channels)
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_22  // Bit clock pin (BCLK)
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_23  // Data output pin (SDATA, speaker)

// PDM microphone pins
#define AUDIO_PDM_MIC_CLK  GPIO_NUM_24  // PDM microphone clock pin (CLK)
#define AUDIO_PDM_MIC_DIN  GPIO_NUM_25  // PDM microphone data input pin (SDIN2)

// Audio codec configuration
// Note: This board has no external codec chip (direct I2S connection), use NoAudioCodec
#define AUDIO_CODEC_PA_PIN       GPIO_NUM_NC  // Power amplifier enable pin (CTRL)
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_NC  // I2C data line (no codec chip, set to NC)
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_NC  // I2C clock line (no codec chip, set to NC)
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR  // Codec I2C address (ignored)

/*---------------------------------------------------------------
 * User input configuration
 * Map the boot/function key used for chat control and Wi-Fi recovery.
 *--------------------------------------------------------------*/
#define BOOT_BUTTON_GPIO GPIO_NUM_35  // Boot/function button pin

/*---------------------------------------------------------------
 * Five-inch RGB display geometry
 * These dimensions must match both the physical panel and LVGL.
 *--------------------------------------------------------------*/
#define DISPLAY_WIDTH   800
#define DISPLAY_HEIGHT  480

#define LCD_BIT_PER_PIXEL (16)
// LCD reset pin (use NC if there is no dedicated reset)
#define PIN_NUM_LCD_RST GPIO_NUM_NC

#define DELAY_TIME_MS (3000)
#define LCD_MIPI_DSI_LANE_NUM (2)

#define MIPI_DSI_PHY_PWR_LDO_CHAN (3)
#define MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV (2500)

/*---------------------------------------------------------------
 * RGB panel timing
 * Define the pixel clock and blanking intervals required by the
 * 800 x 480 panel before mapping its control and RGB565 data pins.
 *--------------------------------------------------------------*/
#define RGB_LCD_PIXEL_CLOCK_HZ     (18 * 1000 * 1000)
#define RGB_LCD_HSYNC              4
#define RGB_LCD_HBP                8
#define RGB_LCD_HFP                8
#define RGB_LCD_VSYNC              4
#define RGB_LCD_VBP                16
#define RGB_LCD_VFP                16

// RGB control pins
#define RGB_PIN_NUM_DISP_EN        GPIO_NUM_NC
#define RGB_PIN_NUM_HSYNC          GPIO_NUM_40
#define RGB_PIN_NUM_VSYNC          GPIO_NUM_41
#define RGB_PIN_NUM_DE             GPIO_NUM_2
#define RGB_PIN_NUM_PCLK           GPIO_NUM_3

// RGB data pins (RGB565: 16bit)
#define RGB_PIN_NUM_DATA0          GPIO_NUM_8
#define RGB_PIN_NUM_DATA1          GPIO_NUM_7
#define RGB_PIN_NUM_DATA2          GPIO_NUM_6
#define RGB_PIN_NUM_DATA3          GPIO_NUM_5
#define RGB_PIN_NUM_DATA4          GPIO_NUM_4
#define RGB_PIN_NUM_DATA5          GPIO_NUM_14
#define RGB_PIN_NUM_DATA6          GPIO_NUM_13
#define RGB_PIN_NUM_DATA7          GPIO_NUM_12
#define RGB_PIN_NUM_DATA8          GPIO_NUM_11
#define RGB_PIN_NUM_DATA9          GPIO_NUM_10
#define RGB_PIN_NUM_DATA10         GPIO_NUM_9
#define RGB_PIN_NUM_DATA11         GPIO_NUM_19
#define RGB_PIN_NUM_DATA12         GPIO_NUM_18
#define RGB_PIN_NUM_DATA13         GPIO_NUM_17
#define RGB_PIN_NUM_DATA14         GPIO_NUM_16
#define RGB_PIN_NUM_DATA15         GPIO_NUM_15

// Display orientation configuration
#define DISPLAY_SWAP_XY   false  // Swap X and Y axis or not
#define DISPLAY_MIRROR_X  false  // Mirror on X axis
#define DISPLAY_MIRROR_Y  false  // Mirror on Y axis
#define DISPLAY_OFFSET_X  0      // X offset
#define DISPLAY_OFFSET_Y  0      // Y offset

// Backlight configuration (5-inch demo: backlight GPIO is -1, usually controlled by STC8 PWM/power)
#define DISPLAY_BACKLIGHT_PIN           GPIO_NUM_NC  // Keep NC if not using GPIO PWM control
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

/*---------------------------------------------------------------
 * Optional GT911 touch configuration
 * Reserve the reset, interrupt, and I2C signals used when touch
 * initialization is enabled in the board constructor.
 *--------------------------------------------------------------*/
#define TOUCH_GPIO_RST      GPIO_NUM_36  // Touch panel reset pin
#define TOUCH_GPIO_INT      GPIO_NUM_42  // Touch panel interrupt pin

// Touch panel I2C configuration
#define TOUCH_I2C_SDA_PIN   GPIO_NUM_45  // Touch panel I2C SDA pin
#define TOUCH_I2C_SCL_PIN   GPIO_NUM_46  // Touch panel I2C SCL pin
#define TOUCH_I2C_PORT      I2C_NUM_0    // I2C port number

/*---------------------------------------------------------------
 * STC8 board-control connection
 * Share the configured I2C bus with the controller that manages
 * amplifier shutdown, backlight power, PWM, and extended signals.
 *--------------------------------------------------------------*/
#define STC8_I2C_PORT       TOUCH_I2C_PORT
#define STC8_I2C_SDA_PIN    TOUCH_I2C_SDA_PIN
#define STC8_I2C_SCL_PIN    TOUCH_I2C_SCL_PIN

#endif // _BOARD_CONFIG_H_
