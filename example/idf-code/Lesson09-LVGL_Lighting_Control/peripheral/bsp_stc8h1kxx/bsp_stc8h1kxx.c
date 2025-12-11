/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
// Include the header file for STC8H1KXX board support package
#include "bsp_stc8h1kxx.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

// Declare a static variable to hold the I2C device handle for the STC8H1KXX device
static i2c_master_dev_handle_t stc8_handle = NULL;

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

// Initialize the I2C communication with the STC8H1KXX chip
esp_err_t stc8_i2c_init()
{
    // Register the I2C device with the STC8 slave address and store the handle
    stc8_handle = i2c_dev_register(STC8_I2C_SLAVE_DEV_ADDR);
    if (stc8_handle == NULL)
    {
        // Log an error message if the registration fails
        STC8H1KXX_ERROR("stc8 i2c register fail");
        return ESP_FAIL;
    }
    return ESP_OK; // Return success if initialization completes successfully
}

// Get battery information via I2C communication
esp_err_t stc8_battery_info_get(Battery_info_t *bat_info)
{
    esp_err_t err = ESP_FAIL;

    // The following block was the original I2C read method (currently commented out)
    // err = i2c_write_read(stc8_handle, STC8_REG_ADDR_BATTERY, (uint8_t*)bat_info, sizeof(Battery_info_t), 1000);
    // if (ESP_OK != err)
    // {
    //     STC8H1KXX_ERROR("stc8 read battery info fail");
    //     return err;
    // }

    // Read battery data byte by byte from the I2C registers
    for (int i = 0; i < sizeof(Battery_info_t); i++)
    {
        // Read one byte from each consecutive register address
        err = i2c_read_reg(stc8_handle, STC8_REG_ADDR_BATTERY + i, (uint8_t*)bat_info + i, 1);
        if (ESP_OK != err)
        {
            // Log an error message if any byte read fails
            STC8H1KXX_ERROR("stc8 read battery info fail");
            return err;
        }
    }
    return err; // Return success or the last error code
}

// Get the input level (HIGH or LOW) of a GPIO pin on STC8H1KXX
esp_err_t stc8_gpio_get_level(int gpio_num, uint8_t* level)
{
    esp_err_t err;

    // Validate the GPIO number for input pins
    if (STC8_GPIO_IN_MAX <= gpio_num) {
        STC8H1KXX_ERROR("stc8 can't get gpio=%d", gpio_num);
        return ESP_FAIL;
    }

    // Read the GPIO level from the corresponding I2C register
    err = i2c_read_reg(stc8_handle, STC8_REG_ADDR_GET_GPIO + gpio_num, level, 1);
    if (ESP_OK != err)
    {
        // Log an error if reading the GPIO level fails
        STC8H1KXX_ERROR("stc8 get gpio=%d fail", gpio_num);
        return err;
    }
    return err; // Return the result of the I2C read operation
}

// Set the output level (HIGH or LOW) of a GPIO pin on STC8H1KXX
esp_err_t stc8_gpio_set_level(int gpio_num, uint8_t level)
{
    esp_err_t err;

    // Validate the GPIO number for output pins
    if (STC8_GPIO_OUT_MAX <= gpio_num) {
        STC8H1KXX_ERROR("stc8 can't set gpio=%d", gpio_num);
        return ESP_FAIL;
    }

    // Write the GPIO level value to the corresponding I2C register
    err = i2c_write_reg(stc8_handle, STC8_REG_ADDR_SET_GPIO + gpio_num, level);
    if (ESP_OK != err)
    {
        // Log an error if setting the GPIO level fails
        STC8H1KXX_ERROR("stc8 set gpio=%d fail", gpio_num);
        return err;
    }
    return err; // Return the result of the I2C write operation
}

// Set the PWM duty cycle of a PWM output pin on STC8H1KXX
esp_err_t stc8_set_pwm_duty(int pwm_num, uint8_t duty)
{
    esp_err_t err;

    // Validate the PWM channel number
    if (STC8_PWM_MAX <= pwm_num) {
        STC8H1KXX_ERROR("stc8 don't have pwm=%d", pwm_num);
        return false;
    }

    // Write the PWM duty cycle value to the corresponding I2C register
    err = i2c_write_reg(stc8_handle, STC8_REG_ADDR_SET_PWM + pwm_num, duty);
    if (ESP_OK != err)
    {
        // Log an error if setting PWM duty fails
        STC8H1KXX_ERROR("stc8 set pwm=%d fail", pwm_num);
        return err;
    }
    return err; // Return the result of the I2C write operation
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
