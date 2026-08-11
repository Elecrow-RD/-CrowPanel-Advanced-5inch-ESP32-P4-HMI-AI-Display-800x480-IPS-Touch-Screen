/*---------------------------------------------------------------
 * Teaching module overview: Board support
 * This file groups the bsp_i2c responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#ifndef _BSP_I2C_H_
#define _BSP_I2C_H_

/*
 * BSP I2C wrapper (scheme B) aligned to user's demo bsp_i2c.h/.c.
 * Uses ESP-IDF I2C master v2 APIs and Kconfig options:
 * - CONFIG_BSP_I2C_ENABLED
 * - CONFIG_I2C_PORT_NUM / CONFIG_I2C_GPIO_SCL / CONFIG_I2C_GPIO_SDA / CONFIG_I2C_GPIO_PULLUP
 */

#include <stdint.h>
#include <stdbool.h>

#include "sdkconfig.h"
#include "esp_err.h"
#include "driver/i2c_master.h"

#define I2C_TAG "I2C"

#ifdef __cplusplus
extern "C" {
#endif

// Always provide API declarations to avoid upper-layer build failures when sdkconfig disables I2C
esp_err_t i2c_init(void);
i2c_master_dev_handle_t i2c_dev_register(uint16_t dev_device_address);
esp_err_t i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size);
esp_err_t i2c_write(i2c_master_dev_handle_t i2c_dev, uint8_t *write_buffer, size_t write_size);
esp_err_t i2c_write_read(i2c_master_dev_handle_t i2c_dev, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms);
esp_err_t i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size);
esp_err_t i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data);

extern i2c_master_bus_handle_t i2c_bus_handle;

#ifdef __cplusplus
}
#endif

#endif // _BSP_I2C_H_

