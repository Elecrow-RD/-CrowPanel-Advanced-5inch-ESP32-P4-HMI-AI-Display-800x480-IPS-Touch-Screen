#ifndef _BSP_STC8H1KXX_H_             // Prevent recursive inclusion of this header file
#define _BSP_STC8H1KXX_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <string.h>                    // Include standard string library
#include <stdint.h>                    // Include standard integer type definitions
#include "freertos/FreeRTOS.h"         // Include FreeRTOS main header
#include "freertos/task.h"             // Include FreeRTOS task management
#include "esp_log.h"                   // Include ESP logging library
#include "esp_err.h"                   // Include ESP error handling definitions
#include "driver/uart.h"               // Include UART driver
#include "bsp_i2c.h"                   // Include I2C bus driver for STC communication
#include "driver/gpio.h"               // Include GPIO driver
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

#ifdef __cplusplus
extern "C"
{
#endif

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define STC8H1KXX_TAG "STC8H1KXX"                         // Tag name used for logging
#define STC8H1KXX_INFO(fmt, ...)  ESP_LOGI(STC8H1KXX_TAG, fmt, ##__VA_ARGS__)   // Information log macro
#define STC8H1KXX_DEBUG(fmt, ...) ESP_LOGD(STC8H1KXX_TAG, fmt, ##__VA_ARGS__)   // Debug log macro
#define STC8H1KXX_ERROR(fmt, ...) ESP_LOGE(STC8H1KXX_TAG, fmt, ##__VA_ARGS__)   // Error log macro

typedef uint8_t   	u8;                                   // Define 8-bit unsigned integer type
typedef uint16_t  	u16;                                  // Define 16-bit unsigned integer type
typedef uint32_t  	u32;                                  // Define 32-bit unsigned integer type

/**************** STC I2C register address and control command ****************/
#define STC8_I2C_SLAVE_DEV_ADDR     0x2F                 // STC8 I2C slave device address

typedef enum
{
    STC8_REG_ADDR_BATTERY   = 0x00, // Get battery information
    STC8_REG_ADDR_GET_GPIO  = 0x10, // Get GPIO input level
    STC8_REG_ADDR_SET_GPIO  = 0x18, // Set GPIO output level
    STC8_REG_ADDR_SET_PWM   = 0x20, // Set PWM duty cycle
}EM_STC8_REG_ADDR;                  // STC8 register address enumeration

typedef struct{
  	u32 adc_voltage;    // Collected ADC voltage (in mV)
	u32 bat_voltage;    // Battery voltage after resistor division calculation (in mV)
	u8 bat_level;       // Battery level percentage
    u8 bat_state;       // Battery charging state
    u8 led_state;	    // LED indicator state
}Battery_info_t;                     // Structure for battery information

typedef enum
{
	BAT_CHARGE_IDLE = 0,             // Idle state (not charging)
	BAT_CHARGE_CHARGING,             // Charging in progress
	BAT_CHARGE_FULLY_CHARGED,	    // Fully charged
	BAT_CHARGE_NO_CHARGE,		    // Not charging
	BAT_CHARGE_ERROR,			    // Error state
}EM_BAT_CHARGE_STATE;                // Enumeration of battery charge states

typedef enum
{
	LED_IDLE = 0,			        // LED idle state
	LED_CHARGING,		            // Charging: red light
	LED_FULLY_CHARGED,	            // Fully charged: green light
	LED_NO_CHARGE,		            // Not charging
	LED_LOW_POWER,		            // Low power: red LED blinking at 0.5 Hz
}EM_LED_STATE;                       // Enumeration of LED indicator states

typedef enum
{
    STC8_GPIO_IN_SW_SPI_UART = 0, 	// Switch detection pin for SPI/UART mode selection
    
    STC8_GPIO_IN_MAX
}EM_STC8_GPIO_IN;                    // Enumeration of STC8 input GPIO pins

typedef enum
{
    STC8_GPIO_OUT_TP_RST = 0,  	    // Touch panel reset pin
    STC8_GPIO_OUT_CSI_RST,    	    // Camera reset pin
    STC8_GPIO_OUT_AUDIO_SD,    	    // Audio amplifier enable pin
    STC8_GPIO_OUT_LCD_BL_POWER,	    // LCD backlight power control pin
    
    STC8_GPIO_OUT_MAX,               // Maximum value indicator for output GPIOs
}EM_STC8_GPIO_OUT;                   // Enumeration of STC8 output GPIO pins

typedef enum
{
    STC8_PWM_LCD_BL_EN = 0,    	    // LCD backlight PWM pin

    STC8_PWM_MAX,
}EM_STC8_PWM;                        // Enumeration of STC8 PWM channels
/**************** STC I2C register address and control command ****************/

esp_err_t stc8_i2c_init();                                   // Initialize STC8 I2C communication
esp_err_t stc8_battery_info_get(Battery_info_t *bat_info);   // Get battery information via I2C
esp_err_t stc8_gpio_get_level(int gpio_num, uint8_t* level); // Get GPIO input level from STC8
esp_err_t stc8_gpio_set_level(int gpio_num, uint8_t level);  // Set GPIO output level on STC8
esp_err_t stc8_set_pwm_duty(int pwm_num, uint8_t duty);      // Set PWM duty cycle for STC8 output

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif  // _BSP_STC8H1KXX_H_
