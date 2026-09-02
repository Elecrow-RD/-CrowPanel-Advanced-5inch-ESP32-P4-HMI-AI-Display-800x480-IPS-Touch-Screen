/**
 * @file main.c
 * @brief Teaching source for 5inch_P4_IDF_16_WiFi_Weather.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

/* 
    Wi-Fi Example
*/
#include <errno.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_timer.h>
#include <sys/time.h>

#include "bsp_illuminate.h"
#include "bsp_wifi.h"
#include "weather.h"

#define TAG "MAIN"
#define MAIN_INFO(fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
#define MAIN_DEBUG(fmt, ...) ESP_LOGD(TAG, fmt, ##__VA_ARGS__)
#define MAIN_ERROR(fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)

#define init_fail(fmt, ...) ESP_LOGE(TAG, fmt":%d", ##__VA_ARGS__)

/**
 * @brief Start the lesson application.
 *
 * Called once by ESP-IDF after the system startup sequence completes.
 * @return None.
 */
void app_main(void)
{
    esp_err_t err = ESP_OK;  // Variable to store error codes

    err = i2c_init();
    if (err != ESP_OK)
        init_fail("i2c", err);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    err = stc8_i2c_init();
    if (err != ESP_OK)
        init_fail("stc8h1kxx", err);
    MAIN_INFO("I2C and stc8 init success");  // Print success log

    // Initialize LCD hardware and LVGL (important: must init before enabling backlight)
    err = display_init();  // Initialize LCD display
    if (err != ESP_OK) {
        init_fail("LCD", err);  // Halt if LCD init fails
    }
    MAIN_INFO("LCD init success");  // Log LCD initialization success

    // Turn off LCD backlight (brightness set to 0 = min)
    err = set_lcd_blight(0);
    if (err != ESP_OK) init_fail("LCD Backlight", err);
    MAIN_INFO("LCD backlight closed (brightness: 0)");

    bsp_wifi_init();
    bsp_wifi_sta_init();
    bsp_wifi_connect("yanfa_software", "yanfa-123456");

    weather_t* weather_handle = weather_create();
    char temp_text[32];
    double temp_c = 0.0;
    char weather_text[64];
    int timestamp = 0;
    char date_str[64];
    char week_str[64];
        
    while (1) {
        if (WIFI_CONNECTED == bsp_wifi_get_state()) {
            if (weather_get_weather(weather_handle, &temp_c, weather_text, &timestamp)) {
                snprintf(temp_text, sizeof(temp_text), "%.1lf°C", temp_c);
                struct timeval tv = {
                    .tv_sec = 0,  // second
                    .tv_usec = 0,   // Microsecond (0-999999）
                };
                tv.tv_sec = timestamp;
                settimeofday(&tv, NULL);

                // %D  Month/Day/Year
                // %e In a two-digit format, the day of the month (in decimal representation) for the current month
                // %F Year-Month-Day
                time_t now = time(NULL);
                struct tm *local_time;
                local_time = localtime(&now);  // Convert to local time
                strftime(date_str, sizeof(date_str), "%Y/%m/%d", local_time);
                strftime(week_str, sizeof(week_str), "%A", local_time);
                ESP_LOGI(TAG, "time(NULL): %d", (int)time(NULL));
                break;
            }
        } else {
            MAIN_INFO("WIFI connecting......");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    lv_obj_t *ui_home = NULL;
    lv_obj_t *temperature_label_ = NULL;
    lv_obj_t *weather_label_ = NULL;
    lv_obj_t *date_label_ = NULL;
    lv_obj_t *week_label_ = NULL;

    if (lvgl_port_lock(0)) {
        
        ui_home = lv_image_create(lv_screen_active());
        LV_IMAGE_DECLARE(image_both);
        lv_image_set_src(ui_home, &image_both);
        lv_obj_align(ui_home, LV_ALIGN_TOP_LEFT, 0, 0);  // Full-screen alignment
        lv_obj_set_size(ui_home, lv_pct(100), lv_pct(100)); // Full-screen size
        lv_image_set_inner_align(ui_home, LV_IMAGE_ALIGN_STRETCH); // Scale background to the 800x480 display

        lv_obj_clear_flag(ui_home, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM));
        lv_obj_set_style_bg_opa(ui_home, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(ui_home, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(ui_home, LV_TEXT_ALIGN_RIGHT, 0); 


        // ========== 1. Temperature label ==========
        temperature_label_ = lv_label_create(ui_home);
        lv_obj_set_width(temperature_label_, lv_pct(100));
        lv_obj_set_height(temperature_label_, LV_SIZE_CONTENT);
        lv_obj_align(temperature_label_, LV_ALIGN_TOP_RIGHT, -50, 80); // Offset to the upper right corner
        lv_label_set_text(temperature_label_, temp_text); // for example "25.4℃"
        // Font size maximum
        lv_obj_set_style_text_font(temperature_label_, &lv_font_montserrat_48, 0); // Increase the font size
        lv_obj_set_style_text_color(temperature_label_, lv_color_hex(0xFFFFFF), 0); // White is more eye-catching.


        // ========== 2. Weather label (below the temperature, with a slightly smaller font size) ==========
        weather_label_ = lv_label_create(ui_home);
        lv_obj_set_width(weather_label_, lv_pct(100));
        lv_obj_set_height(weather_label_, LV_SIZE_CONTENT);
        lv_obj_align(weather_label_, LV_ALIGN_TOP_RIGHT, -50, 140); 
        lv_label_set_text(weather_label_, weather_text); // for example "Partly Cloudy"
        lv_obj_set_style_text_font(weather_label_, &lv_font_montserrat_30, 0); // Font size is smaller than temperature.
        lv_obj_set_style_text_color(weather_label_, lv_color_hex(0xFFFFFF), 0);

        // ========== 3. Date label (below the weather section) ==========
        date_label_ = lv_label_create(ui_home);
        lv_obj_set_width(date_label_, lv_pct(100));
        lv_obj_set_height(date_label_, LV_SIZE_CONTENT);
        lv_obj_align(date_label_, LV_ALIGN_TOP_RIGHT, -50, 180); 
        lv_label_set_text(date_label_, date_str); // for example "2025/12/17"
        lv_obj_set_style_text_font(date_label_, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(date_label_, lv_color_hex(0xFFFFFF), 0);

        // ========== 4. Week label (below the date) ==========
        week_label_ = lv_label_create(ui_home); 
        lv_obj_set_width(week_label_, lv_pct(100));
        lv_obj_set_height(week_label_, LV_SIZE_CONTENT);
        lv_obj_align(week_label_, LV_ALIGN_TOP_RIGHT, -50, 220); 
        lv_label_set_text(week_label_, week_str); // for example "Wednesday"
        lv_obj_set_style_text_font(week_label_, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(week_label_, lv_color_hex(0xFFFFFF), 0);

        lvgl_port_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(1000));    // Wait LVGL task flush to buffer
    set_lcd_blight(100);    // At this point, turn on the screen backlight

}
