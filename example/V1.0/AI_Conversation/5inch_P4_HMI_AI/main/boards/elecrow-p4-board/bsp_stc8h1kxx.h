/*---------------------------------------------------------------
 * Teaching module overview: Board support
 * This file groups the bsp_stc8h1kxx responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#ifndef _BSP_STC8H1KXX_H_
#define _BSP_STC8H1KXX_H_

/*
 * STC8H1KXX board support (minimal implementation for this repo).
 * NOTE: This file is derived from the user's demo header, adapted to use ESP-IDF i2c_master v2 APIs.
 */

#include <stdint.h>
#include "esp_err.h"
#include "bsp_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define STC8_I2C_SLAVE_DEV_ADDR     0x2F

typedef enum {
    STC8_REG_ADDR_BATTERY   = 0x00,
    STC8_REG_ADDR_GET_GPIO  = 0x10,
    STC8_REG_ADDR_SET_GPIO  = 0x18,
    STC8_REG_ADDR_SET_PWM   = 0x20,
} EM_STC8_REG_ADDR;

typedef struct {
    u32 adc_voltage;
    u32 bat_voltage;
    u8  bat_level;
    u8  bat_state;
    u8  led_state;
} Battery_info_t;

typedef enum {
    STC8_GPIO_IN_SW_SPI_UART = 0,
    STC8_GPIO_IN_MAX
} EM_STC8_GPIO_IN;

typedef enum {
    STC8_GPIO_OUT_TP_RST = 0,
    STC8_GPIO_OUT_CSI_RST,
    STC8_GPIO_OUT_AUDIO_SD,
    STC8_GPIO_OUT_LCD_BL_POWER,
    STC8_GPIO_OUT_MAX,
} EM_STC8_GPIO_OUT;

typedef enum {
    STC8_PWM_LCD_BL_EN = 0,
    STC8_PWM_MAX,
} EM_STC8_PWM;

esp_err_t stc8_i2c_init(void);
esp_err_t stc8_battery_info_get(Battery_info_t *bat_info);
esp_err_t stc8_gpio_get_level(int gpio_num, uint8_t* level);
esp_err_t stc8_gpio_set_level(int gpio_num, uint8_t level);
esp_err_t stc8_set_pwm_duty(int pwm_num, uint8_t duty);

#ifdef __cplusplus
}
#endif

#endif // _BSP_STC8H1KXX_H_

