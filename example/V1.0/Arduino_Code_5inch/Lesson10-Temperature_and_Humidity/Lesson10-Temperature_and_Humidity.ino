/**
 * @file Lesson10-Temperature_and_Humidity.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson10-Temperature_and_Humidity course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Lesson10-Temperature And Humidity Module
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
#include "esp_display_panel.hpp"

#include <lvgl.h>
#include "lvgl_v9_port.h"

#include "bsp_i2c.h"        // i2c driver interface
/*
 *  By using this header file, one can obtain battery information, GPIO levels, 
 *  and set GPIO levels as well as the PWM duty cycle of the screen backlight.
 */
#include "bsp_stc8h1kxx.h"  
#include "bsp_dht20.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */
#define PRINTF_ORIGINAL(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__);
#define PRINTF_PRINT(fmt, ...)    Serial.print(fmt);
#define PRINTF_LN(fmt, ...)       Serial.println(fmt);

#define PRINTF_ERROR(fmt, ...)      do { \
                                        Serial.print("ERROR: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_WARN(fmt, ...)       do { \
                                        Serial.print("WARN: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_INFO(fmt, ...)       do { \
                                        Serial.print("INFO: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)  
#define PRINTF_DEBUG(fmt, ...)      do { \
                                        Serial.print("DEBUG: "); \
                                        Serial.printf(fmt, ##__VA_ARGS__); \
                                        Serial.print("\r\n"); \
                                    } while(0)

#define MAIN_INFO(fmt, ...)         PRINTF_INFO(fmt, ##__VA_ARGS__)   // Info level log macro
#define MAIN_ERROR(fmt, ...)        PRINTF_ERROR(fmt, ##__VA_ARGS__)  // Error level log macro

#define LV_COLOR_RED        lv_color_make(0xFF, 0x00, 0x00) // LVGL Red
#define LV_COLOR_GREEN      lv_color_make(0x00, 0xFF, 0x00) // LVGL Green
#define LV_COLOR_BLUE       lv_color_make(0x00, 0x00, 0xFF) // LVGL Blue
#define LV_COLOR_WHITE      lv_color_make(0xFF, 0xFF, 0xFF) // LVGL White
#define LV_COLOR_BLACK      lv_color_make(0x00, 0x00, 0x00) // LVGL Black
#define LV_COLOR_GRAY       lv_color_make(0x80, 0x80, 0x80) // LVGL gray
#define LV_COLOR_YELLOW     lv_color_make(0xFF, 0xFF, 0x00) // LVGL yellow
/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
// --- Declare the panel pointer globally ---
Board *panel = nullptr;

static lv_obj_t *dht20_data = NULL;
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

/**
 * @brief Perform the dht20 display operation used by this lesson.
 *
 * Called from the application flow when this operation is required.
 *
 * @return Nothing.
 */
void dht20_display()
{
    if (lvgl_port_lock(-1))
    {
        dht20_data = lv_label_create(lv_screen_active()); /*Create a label object*/
        static lv_style_t label_style;
        lv_style_init(&label_style);                                                  /*Initialize a style*/
        lv_style_set_bg_opa(&label_style, LV_OPA_TRANSP);                             /*Set the style LVGL background color*/
        lv_obj_add_style(dht20_data, &label_style, LV_PART_MAIN);                     /*Add a style to an object*/
        lv_obj_set_style_text_color(dht20_data, LV_COLOR_WHITE, LV_PART_MAIN);        /*Set the style LVGL text color*/
        lv_obj_set_style_text_font(dht20_data, &lv_font_montserrat_30, LV_PART_MAIN); /*Set the style LVGL text font*/
        lv_obj_center(dht20_data);                                                    /*Align an object to the center on its parent*/
        lv_obj_set_style_bg_color(lv_screen_active(), LV_COLOR_BLACK, LV_PART_MAIN);        /*Set the screen's LVGL background color*/
        lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);            /*Set the screen's LVGL background transparency*/
        lv_label_set_text(dht20_data, "Temperature = 0.0 C  Humidity = 0.0 %%");      /*Set a new text for a label*/
        lvgl_port_unlock();
    }
}

/**
 * @brief Apply the requested update dht20 value operation to the associated device or service.
 *
 * Called when the application needs to update that feature.
 *
 * @param temperature Value supplied by the caller for this operation.
 *
 * @param humidity Value supplied by the caller for this operation.
 *
 * @return Nothing.
 */
void update_dht20_value(float temperature, float humidity)
{
    if (dht20_data)
    {
        char buffer[60];
        snprintf(buffer, sizeof(buffer), "Temperature = %.1f C  Humidity = %.1f %%", temperature, humidity); /*Format the data into a string*/
        lv_label_set_text(dht20_data, buffer);                                                               /*Set a new text for a label*/
    }
}

/**
 * @brief Run the dht20 read task worker independently from the Arduino main loop.
 *
 * Called by the FreeRTOS scheduler after the task is created.
 *
 * @param param Value supplied by the caller for this operation.
 *
 * @return Nothing.
 */
void dht20_read_task(void *param)
{
    static dht20_data_t measurements;
    while (1)
    {
        /*The function for determining whether the DHT20 sensor is ready or not*/
        if (dht20_is_calibrated() == ESP_OK) {
            MAIN_INFO("is calibrated....");
        } else {
            MAIN_INFO("is NOT calibrated....");

            /*Reinitialize the DHT20 sensor*/
            if (dht20_begin() != ESP_OK) {
                MAIN_ERROR("dht20 init again false");
                vTaskDelay(100 / portTICK_PERIOD_MS);
                continue;
            }
        }

        /*Read the temperature and humidity data from the DHT20 sensor*/
        if (dht20_read_data(&measurements) != ESP_OK) {
            if (lvgl_port_lock(-1)) {
                lv_label_set_text(dht20_data, "dht20 read data error"); /*Read failure message displayed*/
                lvgl_port_unlock();
            }
            MAIN_ERROR("dht20 read data error");
        }
        /*Read successfully*/
        else {
            if (lvgl_port_lock(-1)) {
                update_dht20_value(measurements.temperature, measurements.humidity); /*Update the DHT20 data displayed on the screen*/
                lvgl_port_unlock();
            }
            MAIN_INFO("Temperature:\t%.1fC", measurements.temperature);
            MAIN_INFO("Humidity:   \t%.1f%%", measurements.humidity);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
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
    
    dht20_begin();

    Serial.println("Creating UI");

    dht20_display();

    delay(100);  // Wait lvgl run, Prevent the screen from flickering
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
    
    dht20_read_task(nullptr);
}
