/**
 * @file bsp_i2c.c
 * @brief Teaching source for 5inch_P4_IDF_06_USB2_0_HID_Mouse.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_i2c.h"   // Include the header file for I2C BSP (Board Support Package)
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

i2c_master_bus_handle_t i2c_bus_handle = NULL;   // Global handle for the I2C master bus, initialized to NULL

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

// Convert a 16-bit integer to its binary string representation
/**
 * @brief Perform the print binary operation.
 *
 * Called by the application when this module operation is required.
 * @param value Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
char *print_binary(uint16_t value)
{
    static char binary_str[17]; // 16 bits + null-terminator
    binary_str[16] = '\0';      // Null-terminate the string

    for (int i = 15; i >= 0; i--)   // Iterate from MSB to LSB
    {
        binary_str[15 - i] = ((value >> i) & 1) ? '1' : '0'; // Extract each bit
    }

    return binary_str;  // Return the binary string
}

// Lookup table for 4-bit binary representations of hexadecimal digits
const char *bit_rep[16] = {
    [0] = "0000",
    [1] = "0001",
    [2] = "0010",
    [3] = "0011",
    [4] = "0100",
    [5] = "0101",
    [6] = "0110",
    [7] = "0111",
    [8] = "1000",
    [9] = "1001",
    [10] = "1010",
    [11] = "1011",
    [12] = "1100",
    [13] = "1101",
    [14] = "1110",
    [15] = "1111",
};

// Convert a byte (8-bit) into formatted binary string (with "0b" prefix and space between nibbles)
/**
 * @brief Perform the print byte operation.
 *
 * Called by the application when this module operation is required.
 * @param byte Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
char *print_byte(uint8_t byte)
{
    static char binbyte[11];   // Buffer to hold formatted string
    sprintf(binbyte, "0b%s %s", bit_rep[byte >> 4], bit_rep[byte & 0x0F]); // Split high nibble and low nibble
    return binbyte;  // Return formatted string
}

// Initialize the I2C master bus
/**
 * @brief Perform the i2c init operation.
 *
 * Called during application startup before the related peripheral is used.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t i2c_init(void)
{
    static esp_err_t err = ESP_OK;   // Error status
    i2c_master_bus_config_t conf = { // I2C bus configuration
        .i2c_port = I2C_MASTER_PORT, // Use defined I2C port
        .sda_io_num = I2C_GPIO_SDA,  // SDA pin
        .scl_io_num = I2C_GPIO_SCL,  // SCL pin
        .clk_source = I2C_CLK_SRC_DEFAULT, // Default clock source
        .glitch_ignore_cnt = 7,            // Glitch filter count
        .flags.enable_internal_pullup = true // Enable internal pull-up resistors
    };
    err = i2c_new_master_bus(&conf, &i2c_bus_handle); // Create new I2C bus
    if (err != ESP_OK)
        return err; // Return error if bus creation fails
    return err;     // Return success
}

// Register a new I2C device on the bus with its address
/**
 * @brief Perform the i2c dev register operation.
 *
 * Called by the application when this module operation is required.
 * @param dev_device_address Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
i2c_master_dev_handle_t i2c_dev_register(uint16_t dev_device_address)
{
    esp_err_t err = ESP_OK;  // Error code
    i2c_master_dev_handle_t dev_handle = NULL;  // Device handle
    i2c_device_config_t cfg = { // Device configuration
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, // 7-bit addressing
        .device_address = dev_device_address,  // I2C device address
        .scl_speed_hz = 400000,                // I2C speed: 400kHz
    };
    err = i2c_master_bus_add_device(i2c_bus_handle, &cfg, &dev_handle); // Add device to bus
    if (err == ESP_OK)
        return dev_handle; // Return valid handle if success
    return 0;              // Return 0 if failure
}

// Read data from I2C device into buffer
/**
 * @brief Perform the i2c read operation.
 *
 * Called by the application when this module operation is required.
 * @param i2c_dev Input or output value used by this operation.
 * @param read_buffer Input or output value used by this operation.
 * @param read_size Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t i2c_read(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_receive(i2c_dev, read_buffer, read_size, 1000); // Timeout = 1000ms
}

// Write data to I2C device
/**
 * @brief Perform the i2c write operation.
 *
 * Called by the application when this module operation is required.
 * @param i2c_dev Input or output value used by this operation.
 * @param write_buffer Input or output value used by this operation.
 * @param write_size Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t i2c_write(i2c_master_dev_handle_t i2c_dev, uint8_t *write_buffer, size_t write_size)
{
    return i2c_master_transmit(i2c_dev, write_buffer, write_size, 1000); // Timeout = 1000ms
}

// Write a register address first, then read data back
/**
 * @brief Perform the i2c write read operation.
 *
 * Called by the application when this module operation is required.
 * @param i2c_dev Input or output value used by this operation.
 * @param read_reg Input or output value used by this operation.
 * @param read_buffer Input or output value used by this operation.
 * @param read_size Input or output value used by this operation.
 * @param delayms Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t i2c_write_read(i2c_master_dev_handle_t i2c_dev, uint8_t read_reg, uint8_t *read_buffer, size_t read_size, uint16_t delayms)
{
    esp_err_t err = ESP_OK;
    err = i2c_master_transmit(i2c_dev, &read_reg, 1, delayms); // Send register address
    if (err != ESP_OK)
        return err;
    err = i2c_master_receive(i2c_dev, read_buffer, read_size, delayms); // Read data
    if (err != ESP_OK)
        return err;
    return err; // Return final status
}

// Read a specific register from an I2C device
/**
 * @brief Perform the i2c read reg operation.
 *
 * Called by the application when this module operation is required.
 * @param i2c_dev Input or output value used by this operation.
 * @param reg_addr Input or output value used by this operation.
 * @param read_buffer Input or output value used by this operation.
 * @param read_size Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t i2c_read_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t *read_buffer, size_t read_size)
{
    return i2c_master_transmit_receive(i2c_dev, &reg_addr, 1, read_buffer, read_size, 1000); // Transmit register, then read
}

// Write data to a specific register of an I2C device
/**
 * @brief Perform the i2c write reg operation.
 *
 * Called by the application when this module operation is required.
 * @param i2c_dev Input or output value used by this operation.
 * @param reg_addr Input or output value used by this operation.
 * @param data Input or output value used by this operation.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t i2c_write_reg(i2c_master_dev_handle_t i2c_dev, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data}; // Register address + data
    return i2c_master_transmit(i2c_dev, write_buf, sizeof(write_buf), 1000); // Send 2 bytes
}


/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
