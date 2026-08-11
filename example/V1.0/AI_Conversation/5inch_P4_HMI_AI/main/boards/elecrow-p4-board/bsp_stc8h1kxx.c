/*---------------------------------------------------------------
 * Teaching module overview: Board support
 * This file groups the bsp_stc8h1kxx responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#include "bsp_stc8h1kxx.h"

#include "sdkconfig.h"

#include <string.h>

#include <esp_log.h>

#define TAG "STC8H1KXX"

// Align to demo: register device handle via bsp_i2c wrapper
static i2c_master_dev_handle_t s_stc8_handle = NULL;

/**
 * @brief Register the STC8 board-control device on I2C.
 * @param None.
 * @return ESP_OK on success, otherwise the registration error.
 * @note Called once after the shared I2C bus is initialized.
 */
esp_err_t stc8_i2c_init(void) {
    if (s_stc8_handle != NULL) return ESP_OK;

#ifdef CONFIG_BSP_I2C_ENABLED
    esp_err_t err = i2c_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_init failed: %s", esp_err_to_name(err));
        return err;
    }
    s_stc8_handle = i2c_dev_register(STC8_I2C_SLAVE_DEV_ADDR);
    if (s_stc8_handle == NULL) {
        ESP_LOGE(TAG, "stc8 i2c register fail");
        return ESP_FAIL;
    }
    return ESP_OK;
#else
    ESP_LOGE(TAG, "CONFIG_BSP_I2C_ENABLED is not enabled");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Set one STC8-managed output level.
 * @param gpio_num STC8 output identifier.
 * @param level Logical output level.
 * @return ESP_OK on success, otherwise the I2C transaction error.
 * @note Called for amplifier, backlight, and other extended power controls.
 */
esp_err_t stc8_gpio_set_level(int gpio_num, uint8_t level) {
    // Align to demo bsp_stc8h1kxx.c: write to register (STC8_REG_ADDR_SET_GPIO + gpio_num)
    if (gpio_num < 0 || gpio_num >= (int)STC8_GPIO_OUT_MAX) {
        ESP_LOGE(TAG, "stc8 can't set gpio=%d", gpio_num);
        return ESP_ERR_INVALID_ARG;
    }
    if (s_stc8_handle == NULL) {
        esp_err_t err = stc8_i2c_init();
        if (err != ESP_OK) return err;
    }
    return i2c_write_reg(s_stc8_handle, (uint8_t)(STC8_REG_ADDR_SET_GPIO + gpio_num), level);
}

/**
 * @brief Read one STC8-managed input level.
 * @param gpio_num STC8 input identifier.
 * @param level Receives the logical input level.
 * @return ESP_OK on success, otherwise the I2C transaction error.
 * @note Called when board logic needs an extended input state.
 */
esp_err_t stc8_gpio_get_level(int gpio_num, uint8_t* level) {
    if (level == NULL) return ESP_ERR_INVALID_ARG;
    // Align to demo bsp_stc8h1kxx.c: read from register (STC8_REG_ADDR_GET_GPIO + gpio_num)
    if (gpio_num < 0 || gpio_num >= (int)STC8_GPIO_IN_MAX) {
        ESP_LOGE(TAG, "stc8 can't get gpio=%d", gpio_num);
        return ESP_ERR_INVALID_ARG;
    }
    if (s_stc8_handle == NULL) {
        esp_err_t err = stc8_i2c_init();
        if (err != ESP_OK) return err;
    }
    return i2c_read_reg(s_stc8_handle, (uint8_t)(STC8_REG_ADDR_GET_GPIO + gpio_num), level, 1);
}

/**
 * @brief Set an STC8 PWM channel duty cycle.
 * @param pwm_num STC8 PWM channel identifier.
 * @param duty Requested duty value from 0 to 100.
 * @return ESP_OK on success, otherwise the I2C transaction error.
 * @note Called whenever backlight brightness changes.
 */
esp_err_t stc8_set_pwm_duty(int pwm_num, uint8_t duty) {
    // Align to demo bsp_stc8h1kxx.c: write to register (STC8_REG_ADDR_SET_PWM + pwm_num)
    if (pwm_num < 0 || pwm_num >= (int)STC8_PWM_MAX) {
        ESP_LOGE(TAG, "stc8 don't have pwm=%d", pwm_num);
        return ESP_ERR_INVALID_ARG;
    }
    if (s_stc8_handle == NULL) {
        esp_err_t err = stc8_i2c_init();
        if (err != ESP_OK) return err;
    }
    return i2c_write_reg(s_stc8_handle, (uint8_t)(STC8_REG_ADDR_SET_PWM + pwm_num), duty);
}

/**
 * @brief Read the battery measurements collected by the STC8.
 * @param bat_info Destination for the decoded battery information.
 * @return ESP_OK on success, otherwise the I2C transaction error.
 * @note Called by power-status features when fresh battery data is needed.
 */
esp_err_t stc8_battery_info_get(Battery_info_t *bat_info) {
    if (bat_info == NULL) return ESP_ERR_INVALID_ARG;
    // Align to demo bsp_stc8h1kxx.c: read battery struct byte-by-byte starting at STC8_REG_ADDR_BATTERY
    for (int i = 0; i < (int)sizeof(Battery_info_t); i++) {
        if (s_stc8_handle == NULL) {
            esp_err_t e = stc8_i2c_init();
            if (e != ESP_OK) return e;
        }
        esp_err_t err = i2c_read_reg(s_stc8_handle, (uint8_t)(STC8_REG_ADDR_BATTERY + i), ((uint8_t*)bat_info) + i, 1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "stc8 read battery info fail: %s", esp_err_to_name(err));
            return err;
        }
    }
    return ESP_OK;
}

