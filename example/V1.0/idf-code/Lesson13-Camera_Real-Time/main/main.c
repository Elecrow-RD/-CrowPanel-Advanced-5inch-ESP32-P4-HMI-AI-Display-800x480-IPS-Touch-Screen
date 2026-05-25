/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "main.h"   // Include the main header file containing necessary definitions and declarations
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
// function declaration
void init_fail(const char *name, esp_err_t err);   // Function declaration for initialization failure handling
void Init(void);   // Function declaration for system initialization
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
void init_fail(const char *name, esp_err_t err)   // Function to handle initialization failures
{
    static bool state = false;   // Flag to avoid repeated error logging
    while (1)   // Stay in infinite loop after failure
    {
        if (!state)   // Print error message only once
        {
            MAIN_ERROR("%s init  [ %s ]", name, esp_err_to_name(err));   // Log initialization failure with error name
            state = true;   // Update state to prevent repeated logs
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);   // Wait 1 second before looping again
    }
}

void Init(void)   // System initialization function
{
    static esp_err_t err = ESP_OK;   // Variable to store function return values

    err = i2c_init();
    if (err != ESP_OK)
        init_fail("i2c", err);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    err = stc8_i2c_init();
    if (err != ESP_OK)
        init_fail("stc8h1kxx", err);

    MAIN_INFO("I2C and stc8 init success");  // Print success log

    err = set_lcd_blight(100);  // Enable backlight with 100% brightness
    if (err != ESP_OK) {  // Check error
        init_fail("LCD Backlight", err);  // Handle failure
    }
    MAIN_INFO("LCD backlight opened (brightness: 100)");  // Log success message for backlight

    err = display_init();   // Initialize LCD display
    if (err != ESP_OK)   // Check for error
        init_fail("display", err);   // Handle initialization failure

    err = camera_video_init();   // Initialize camera module
    if (err != ESP_OK)   // Check for error
        init_fail("camera", err);   // Handle initialization failure
    int video_node = camera_work();
    if (video_node == -1)
        init_fail("camera", ESP_FAIL);
}

void app_main(void)   // Main application entry point
{
    MAIN_INFO("----------Camera task----------\r\n");   // Print start log message

    Init();   // Call system initialization function

    vTaskDelay(pdMS_TO_TICKS(300)); // Wait for camera data
    if (lvgl_port_lock(0))
    {
        set_camera_img_display(true);
        lvgl_port_unlock();
    }
    
    MAIN_INFO("----------The screen is displaying.----------\r\n");   // Log that the screen is now displaying camera output
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
