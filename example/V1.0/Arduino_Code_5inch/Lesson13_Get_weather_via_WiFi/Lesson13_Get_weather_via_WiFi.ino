/**
 * @file Lesson13_Get_weather_via_WiFi.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson13_Get_weather_via_WiFi course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Lesson13 Get Weather Via Wi Fi Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

/**
 * IMPORTANT:
 * This code requires the "ESP32_Display_Panel" library.
 * Before uploading, you MUST configure the following file in your libraries folder:
 * ./esp_panel_board_custom_conf.h
 * 
 * 1. Enable RGB:           #define ESP_PANEL_BOARD_LCD_BUS_TYPE        (ESP_PANEL_BUS_TYPE_RGB)
 * 2. Set LCD Driver:       #define ESP_PANEL_BOARD_LCD_CONTROLLER      ST7262
 * 3. Set Touch:            #define ESP_PANEL_BOARD_TOUCH_CONTROLLER    GT911
 * 4. Disable backlight:    #define ESP_PANEL_BOARD_USE_BACKLIGHT       (0)          // because backlight controlled by STC8H1KXX MCU
 */
/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"   // board pin define
#include <Arduino.h>        // Arduino core library. Must be placed at the very top to ensure recognition of Arduino APIs

#include <string.h>         // C string lib
#include <esp_log.h>        // ESP-IDF logging library
#include <esp_err.h>        // ESP-IDF error codes

#include "esp_panel_board_custom_conf.h"
#include "ESP_Panel_Library.h"

#include <lvgl.h>
#include "lvgl_v8_port.h"

#include "bsp_i2c.h"        // i2c driver interface
/*
 *  By using this header file, one can obtain battery information, GPIO levels, 
 *  and set GPIO levels as well as the PWM duty cycle of the screen backlight.
 */
#include "bsp_stc8h1kxx.h"  

/* Wireless Connectivity (WiFi) */
#include <WiFi.h>                   // Standard ESP32 WiFi library
#include "weather.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */
#define TAG     "Get_weather_via_wifi"
/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
/* This is the handle for touch, display, and backlight. */
Board *board = nullptr;

const char* sta_ssid     = "yanfa_software";
const char* sta_password = "yanfa-123456";

char temp_text[32];
char weather_text[64];
char date_str[64];
char week_str[64];
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

/**
 * @brief Perform the weather display operation used by this lesson.
 *
 * Called from the application flow when this operation is required.
 *
 * @return Nothing.
 */
void weather_display()
{
    LV_IMG_DECLARE(image_both);

    lv_obj_t *ui_home = NULL;
    lv_obj_t *temperature_label_ = NULL;
    lv_obj_t *weather_label_ = NULL;
    lv_obj_t *date_label_ = NULL;
    lv_obj_t *week_label_ = NULL;

    if (lvgl_port_lock(-1)) {
        
        ui_home = lv_img_create(lv_scr_act());
        lv_img_set_src(ui_home, &image_both);
        lv_obj_align(ui_home, LV_ALIGN_CENTER, 0, 0);  // Full-screen alignment
        lv_obj_set_size(ui_home, LV_HOR_RES, LV_VER_RES); // Full-screen size

        lv_obj_clear_flag(ui_home, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM));
        lv_obj_set_style_bg_opa(ui_home, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(ui_home, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(ui_home, LV_TEXT_ALIGN_RIGHT, 0); 

        // ========== 1. Temperature label ==========
        temperature_label_ = lv_label_create(ui_home);
        lv_obj_set_width(temperature_label_, LV_HOR_RES);
        lv_obj_set_height(temperature_label_, LV_SIZE_CONTENT);
        lv_obj_align(temperature_label_, LV_ALIGN_TOP_RIGHT, -50, 80); // Offset to the upper right corner
        lv_label_set_text(temperature_label_, temp_text); // for example "25.4℃"
        // Font size maximum
        lv_obj_set_style_text_font(temperature_label_, &lv_font_montserrat_48, 0); // Increase the font size
        lv_obj_set_style_text_color(temperature_label_, lv_color_hex(0xFFFFFF), 0); // White is more eye-catching.


        // ========== 2. Weather label (below the temperature, with a slightly smaller font size) ==========
        weather_label_ = lv_label_create(ui_home);
        lv_obj_set_width(weather_label_, LV_HOR_RES);
        lv_obj_set_height(weather_label_, LV_SIZE_CONTENT);
        lv_obj_align(weather_label_, LV_ALIGN_TOP_RIGHT, -50, 140); 
        lv_label_set_text(weather_label_, weather_text); // for example "Partly Cloudy"
        lv_obj_set_style_text_font(weather_label_, &lv_font_montserrat_30, 0); // Font size is smaller than temperature.
        lv_obj_set_style_text_color(weather_label_, lv_color_hex(0xFFFFFF), 0);

        // ========== 3. Date label (below the weather section) ==========
        date_label_ = lv_label_create(ui_home);
        lv_obj_set_width(date_label_, LV_HOR_RES);
        lv_obj_set_height(date_label_, LV_SIZE_CONTENT);
        lv_obj_align(date_label_, LV_ALIGN_TOP_RIGHT, -50, 180); 
        lv_label_set_text(date_label_, date_str); // for example "2025/12/17"
        lv_obj_set_style_text_font(date_label_, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(date_label_, lv_color_hex(0xFFFFFF), 0);

        // ========== 4. Week label (below the date) ==========
        week_label_ = lv_label_create(ui_home); 
        lv_obj_set_width(week_label_, LV_HOR_RES);
        lv_obj_set_height(week_label_, LV_SIZE_CONTENT);
        lv_obj_align(week_label_, LV_ALIGN_TOP_RIGHT, -50, 220); 
        lv_label_set_text(week_label_, week_str); // for example "Wednesday"
        lv_obj_set_style_text_font(week_label_, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(week_label_, lv_color_hex(0xFFFFFF), 0);

        lvgl_port_unlock();
    }
}

/**
 * @brief Initialize display touch LVGL init and leave it ready for later operations.
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
void display_touch_lvgl_init() 
{
    stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN, 0);     // set backlight (0~100)
    // --- Initialize Display and Touch Panel ---
    Board *board = new Board();
    // Initialize the bus (RGB) and the devices (ST7265 & GT911)
    Serial.println("Initializing Panel (ST7265 + GT911)...");
    assert(board->init());
#if LVGL_PORT_AVOID_TEARING_MODE
    LCD *lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#endif
    assert(board->begin());
    Serial.println("Display and Touch system online.");

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());

    /* print LCD config */
    // Bus *lcd_bus = lcd->getBus();
    // static_cast<BusRGB *>(lcd_bus)->getConfig().printRefreshPanelConfig();
}

/**
 * @brief Initialize the hardware and services required by this lesson.
 *
 * Called once by the Arduino runtime after power-up or reset.
 *
 * @return Nothing.
 */
void setup() {
    // put your setup code here, to run once:

    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);

#if (1 == ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST)
    i2c_init(I2C_GPIO_SCL, I2C_GPIO_SDA);
#endif

    display_touch_lvgl_init();

    /* ESP-Hosted-MCU SDIO Interface Pins for WiFi */
    WiFi.setPins(
        WIFI_HOSTED_SDIO_PIN_CLK, 
        WIFI_HOSTED_SDIO_PIN_CMD, 
        WIFI_HOSTED_SDIO_PIN_D0, 
        WIFI_HOSTED_SDIO_PIN_D1,
        WIFI_HOSTED_SDIO_PIN_D2, 
        WIFI_HOSTED_SDIO_PIN_D3, 
        WIFI_HOSTED_SDIO_PIN_RESET
    );

    // Set mode to STA
    WiFi.mode(WIFI_STA);
    // Connect to Router
    WiFi.begin(sta_ssid, sta_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\n");
    Serial.print("[STA] Connected. IP: ");
    Serial.println(WiFi.localIP());

    weather_t* weather_handle = weather_create();
    double temp_c = 0.0;
    int timestamp = 0;
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
    }

    weather_display();

    delay(200);  // Wait lvgl run, Prevent the screen from flickering
    stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN, 100);     // set backlight (0~100)
}

/**
 * @brief Run the lesson's recurring application work.
 *
 * Called repeatedly by the Arduino runtime after setup completes.
 *
 * @return Nothing.
 */
void loop() {
    // put your main code here, to run repeatedly:
  
    delay(1000);
}
