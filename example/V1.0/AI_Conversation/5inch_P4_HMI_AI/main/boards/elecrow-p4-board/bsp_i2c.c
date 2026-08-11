/*---------------------------------------------------------------
 * Teaching module overview: Board support
 * This file groups the bsp_i2c responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#include "bsp_i2c.h"

#include "sdkconfig.h"

#include <esp_log.h>

i2c_master_bus_handle_t i2c_bus_handle = NULL;

/**
 * @brief Create the shared board-control I2C bus.
 *
 * @param None.
 * @return ESP_OK when the bus is ready, otherwise an ESP-IDF driver error.
 *
 * Called once during board startup before registering STC8 devices.
 */
esp_err_t i2c_init(void) {
#if defined(CONFIG_BSP_I2C_ENABLED) && CONFIG_BSP_I2C_ENABLED
    if (i2c_bus_handle != NULL) {
        return ESP_OK;
    }

    const int I2C_MASTER_PORT = CONFIG_I2C_PORT_NUM;
    const int I2C_GPIO_SCL = CONFIG_I2C_GPIO_SCL;
    const int I2C_GPIO_SDA = CONFIG_I2C_GPIO_SDA;

    i2c_master_bus_config_t conf = {
        .i2c_port = I2C_MASTER_PORT,
        .sda_io_num = I2C_GPIO_SDA,
        .scl_io_num = I2C_GPIO_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
#ifdef CONFIG_I2C_GPIO_PULLUP
            .enable_internal_pullup = true,
#else
            .enable_internal_pullup = false,
#endif
        },
    };

    esp_err_t err = i2c_new_master_bus(&conf, &i2c_bus_handle);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(I2C_TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        return err;
    }
    return ESP_OK;
#else
    ESP_LOGE(I2C_TAG, "BSP I2C is disabled (CONFIG_BSP_I2C_ENABLED=n)");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Register one device on the shared I2C bus.
 *
 * @param dev_device_address Seven-bit peripheral address.
 * @return Device handle on success, or NULL if registration fails.
 *
 * Called by a board peripheral after i2c_init() has completed.
 */
i2c_master_dev_handle_t i2c_dev_register(uint16_t dev_device_address) {
    if (i2c_bus_handle == NULL) {
        if (i2c_init() != ESP_OK) {
            return NULL;
        }
    }

    i2c_master_dev_handle_t dev_handle = NULL;
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_device_address,
        .scl_speed_hz = 400000,
    };
    esp_err_t err = i2c_master_bus_add_device(i2c_bus_handle, &cfg, &dev_handle);
    if (err == ESP_OK) {
        return dev_handle;
    }
    return NULL;
}

/**
 * @brief Read a raw byte sequence from an I2C device.
 * @param i2c_dev Registered device handle.
 * @param read_buffer Destination buffer.
 * @param read_size Number of bytes to receive.
 * @return ESP_OK on success, otherwise the transaction error.
 * @note Called by peripheral drivers that do not require a register address.
 */
esp_err_t i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size) {
    return i2c_master_receive(i2c_dev, read_buffer, read_size, 1000);
}

/**
 * @brief Write a raw byte sequence to an I2C device.
 * @param i2c_dev Registered device handle.
 * @param write_buffer Bytes to transmit.
 * @param write_size Number of bytes to transmit.
 * @return ESP_OK on success, otherwise the transaction error.
 * @note Called by peripheral drivers for command-style transfers.
 */
esp_err_t i2c_write(i2c_master_dev_handle_t i2c_dev, uint8_t *write_buffer, size_t write_size) {
    return i2c_master_transmit(i2c_dev, write_buffer, write_size, 1000);
}

/**
 * @brief Select a register and read its response after an optional delay.
 * @param i2c_dev Registered device handle.
 * @param read_reg Register address to select.
 * @param read_buffer Destination buffer.
 * @param read_size Number of bytes to receive.
 * @param delayms Delay between the write and read phases in milliseconds.
 * @return ESP_OK on success, otherwise the transaction error.
 * @note Called when a peripheral needs processing time before returning data.
 */
esp_err_t i2c_write_read(i2c_master_dev_handle_t i2c_dev, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms) {
    esp_err_t err = i2c_master_transmit(i2c_dev, &read_reg, 1, delayms);
    if (err != ESP_OK) return err;
    err = i2c_master_receive(i2c_dev, read_buffer, read_size, delayms);
    return err;
}

/**
 * @brief Read one or more bytes from a device register.
 * @param i2c_dev Registered device handle.
 * @param reg_addr Register address.
 * @param read_buffer Destination buffer.
 * @param read_size Number of bytes to receive.
 * @return ESP_OK on success, otherwise the transaction error.
 * @note Called by board peripheral accessors during normal operation.
 */
esp_err_t i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size) {
    return i2c_master_transmit_receive(i2c_dev, &reg_addr, 1, read_buffer, read_size, 1000);
}

/**
 * @brief Write one byte to a device register.
 * @param i2c_dev Registered device handle.
 * @param reg_addr Register address.
 * @param data Value to write.
 * @return ESP_OK on success, otherwise the transaction error.
 * @note Called by board peripheral accessors during normal operation.
 */
esp_err_t i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = { reg_addr, data };
    return i2c_master_transmit(i2c_dev, write_buf, sizeof(write_buf), 1000);
}

