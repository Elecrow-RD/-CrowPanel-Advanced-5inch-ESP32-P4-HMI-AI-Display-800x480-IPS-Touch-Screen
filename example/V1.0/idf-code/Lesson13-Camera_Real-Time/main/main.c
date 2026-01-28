/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "main.h"   // Include the main header file containing necessary definitions and declarations
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

TaskHandle_t lvgl_camera;   // Task handle for LVGL camera display task
TaskHandle_t camera_read;

// static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;

// function declaration
void init_fail(const char *name, esp_err_t err);   // Function declaration for initialization failure handling
void Init(void);   // Function declaration for system initialization
void camera_display_task(void *param);   // Function declaration for camera display task
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
void camera_task(void *param)
{
    int cnt = 0;
    while (1)
    {
        esp_cam_ctlr_receive(cam_handle, &my_trans, ESP_CAM_CTLR_MAX_DELAY);
        cnt++;
        if (cnt >= 5)
        {
            cnt = 0;
            vTaskDelete(NULL);
        }
    }
}

void camera_display_task(void *param)   // Task function to continuously refresh the camera display
{
    while (1)   // Infinite loop for periodic refreshing
    {
        // Directly refresh the camera display without the need for status check
        if (lvgl_port_lock(0))   // Lock LVGL port for safe access (timeout = 0)
        {
            camera_display_refresh();   // Refresh camera display content
            lvgl_port_unlock();   // Unlock LVGL port after refresh
        }
        vTaskDelay(23 / portTICK_PERIOD_MS);   // Delay approximately 23 ms between refreshes
    }
}

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

    esp_ldo_channel_config_t ldo3_cof = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);
    if (err != ESP_OK)
        init_fail("ldo3", err);
    //esp_ldo_channel_config_t ldo4_cof = {
    //    .chan_id = 4,
    //    .voltage_mv = 3300,
    //};
    //err = esp_ldo_acquire_channel(&ldo4_cof, &ldo4);
    //if (err != ESP_OK)
    //    init_fail("ldo4", err);

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

    err = camera_init();   // Initialize camera module
    if (err != ESP_OK)   // Check for error
        init_fail("camera", err);   // Handle initialization failure

    camera_display();   // Initialize camera display output
}

void app_main(void)   // Main application entry point
{
    MAIN_INFO("----------Camera task----------\r\n");   // Print start log message

    Init();   // Call system initialization function

    // Create and start the camera display task on Core 1 with priority (max - 4)
    xTaskCreatePinnedToCore(camera_display_task, "camera_display", 4096, NULL, configMAX_PRIORITIES - 5, &lvgl_camera, 1);
    xTaskCreatePinnedToCore(camera_task, "camera", 4096, NULL, configMAX_PRIORITIES - 4, &camera_read, 0);
    
    MAIN_INFO("----------The screen is displaying.----------\r\n");   // Log that the screen is now displaying camera output
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
