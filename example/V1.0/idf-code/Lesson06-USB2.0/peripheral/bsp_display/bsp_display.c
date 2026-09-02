/**
 * @file bsp_display.c
 * @brief Teaching source for 5inch_P4_IDF_06_USB2_0_HID_Mouse.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_display.h"   // Include the display BSP header
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

// Handle for the GT911 touch panel
static esp_lcd_touch_handle_t tp = NULL;  
// Handle for I2C panel I/O
static esp_lcd_panel_io_handle_t tp_io_handle = NULL;

// Current touch X coordinate
static uint16_t touch_x = 0xffff;  
// Current touch Y coordinate
static uint16_t touch_y = 0xffff;  
// Current touch press state
static bool is_pressed = false; 
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

// Internal function to update stored touch coordinates and press state
/**
 * @brief Perform the set coor operation.
 *
 * Called by the application when this module operation is required.
 * @param x Input or output value used by this operation.
 * @param y Input or output value used by this operation.
 * @param press Input or output value used by this operation.
 * @return None.
 */
static void set_coor(uint16_t x, uint16_t y, bool press)
{
    touch_x = x;       // Update X coordinate
    touch_y = y;       // Update Y coordinate
    is_pressed = press; // Update press state
}

// Public function to get the latest touch coordinates and press state
/**
 * @brief Perform the get coor operation.
 *
 * Called by the application when this module operation is required.
 * @param x Input or output value used by this operation.
 * @param y Input or output value used by this operation.
 * @param press Input or output value used by this operation.
 * @return None.
 */
void get_coor(uint16_t* x, uint16_t* y, bool* press)
{
    *x = touch_x;      // Return X coordinate
    *y = touch_y;      // Return Y coordinate
    *press = is_pressed; // Return press state
}

// Initialize the GT911 touch panel
/**
 * @brief Perform the touch init operation.
 *
 * Called during application startup before the related peripheral is used.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t touch_init(void)
{
    esp_err_t err = ESP_OK;  // Error status

    // I2C panel I/O configuration
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,  // Primary GT911 I2C address
        .control_phase_bytes = 1,                        // Control phase bytes
        .dc_bit_offset = 0,                              // Not used
        .lcd_cmd_bits = 16,                              // Command bit width
        .flags =
            {
                .disable_control_phase = 1,             // Disable control phase
            },
        .scl_speed_hz = 400000,                          // I2C clock speed
    };

    // GT911 touch configuration
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = H_size,          // Max X coordinate
        .y_max = V_size,          // Max Y coordinate
        .rst_gpio_num = Touch_GPIO_RST, // Reset GPIO
        .int_gpio_num = Touch_GPIO_INT, // Interrupt GPIO
        .levels = {
            .reset = 0,           // Reset level
            .interrupt = 0,       // Interrupt level
        },
        .flags = {
            .swap_xy = false,     // Do not swap X/Y
            .mirror_x = false,    // Do not mirror X
            .mirror_y = false,    // Do not mirror Y
        },
    };
    
    tp_cfg.driver_data = (void*)&io_config.dev_addr;
    // Create I2C panel I/O
    err = esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_bus_handle, &io_config, &tp_io_handle);
    if (err != ESP_OK)
        return err;

    // Initialize GT911 touch driver
    err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);
    if (err != ESP_OK)
    {
        // Try backup I2C address if primary fails
        io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
        err = esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_bus_handle, &io_config, &tp_io_handle);
        if (err != ESP_OK)
            return err;
        err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);
        if (err != ESP_OK)
            return err;
    }

    return err;  // Return final status
}

// Read the touch panel data and update coordinates
/**
 * @brief Perform the touch read operation.
 *
 * Called by the application when this module operation is required.
 * @return Result produced by the operation; see the function implementation for success and error values.
 */
esp_err_t touch_read(void)
{
    esp_err_t err = ESP_OK;   // Error status
    esp_lcd_touch_point_data_t touch_data[1] = {0}; // First touch point
    uint8_t touch_cnt = 0;    // Number of touch points detected

    // Read touch data
    err = esp_lcd_touch_read_data(tp);
    if (err != ESP_OK)
    {
        DISPLAY_INFO("GT911 read error"); // Log read error
        return err;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay for stability

    // Get touch coordinates
    err = esp_lcd_touch_get_data(tp, touch_data, &touch_cnt, 1);
    if (err != ESP_OK) {
        DISPLAY_INFO("GT911 get data error");
        return err;
    }

    if (touch_cnt > 0) {
        DISPLAY_INFO("X=%hu Y=%hu strength=%hu cnt=%d", touch_data[0].x, touch_data[0].y, touch_data[0].strength, touch_cnt);
        set_coor(touch_data[0].x, touch_data[0].y, true); // Update coordinates and press state
    }
    else {
        set_coor(0xffff, 0xffff, false); // No touch detected
    }

    return err; // Return status
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
