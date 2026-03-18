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

/* panel driver */
#include "esp_panel_board_custom_conf.h"
#include "ESP_Panel_Library.h"

/* LVGL and driver */
#include <lvgl.h>
#include "lvgl_v8_port.h"

/* wireless module */
#include "bsp_wireless.h"

#include "bsp_i2c.h"        // i2c driver interface
/*
 *  By using this header file, one can obtain battery information, GPIO levels, 
 *  and set GPIO levels as well as the PWM duty cycle of the screen backlight.
 */
#include "bsp_stc8h1kxx.h"  

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
static lv_obj_t *s_hello_label = NULL;
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

static void lvgl_show_counter_label_init(void)
{
    if (lvgl_port_lock(-1) != true) {
        MAIN_ERROR("LVGL lock failed");
        return;
    }

    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, LV_COLOR_WHITE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    s_hello_label = lv_label_create(screen);
    if (s_hello_label == NULL) {
        MAIN_ERROR("Create LVGL label failed");
        lvgl_port_unlock();
        return;
    }
    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_text_font(&label_style, &lv_font_montserrat_42); 
    lv_style_set_text_color(&label_style, lv_color_black());
    lv_style_set_bg_opa(&label_style, LV_OPA_TRANSP);
    lv_obj_add_style(s_hello_label, &label_style, LV_PART_MAIN);

    lv_label_set_text(s_hello_label, "TX_Hello World:0");
    lv_obj_center(s_hello_label);

    lvgl_port_unlock();
}

static void ui_counter_task(void *param)
{
    char text[48];
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000); // 1 second = 1000ms
    
    for (;;) {
        uint32_t i = sx1262_get_tx_counter();
        int n = snprintf(text, sizeof(text), "TX_Hello World:%lu", (unsigned long)i);
        (void)n;

        if (lvgl_port_lock(-1) == true) {
            if (s_hello_label != NULL) {
                lv_label_set_text(s_hello_label, text);
            }
            lvgl_port_unlock();
        }

        MAIN_INFO("TX msg: %s", text);
        
        // Use absolute time to ensure an exact one-second interval
        vTaskDelayUntil(&last_wake_time, frequency);
    }
}

static void lora_tx_task(void *param)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000); // 1 second = 1000ms
    
    while (1) {
        bool lora_tx_OK = false;
        lora_tx_OK = send_lora_pack_radio();
        if (lora_tx_OK != true) {
            MAIN_ERROR("LoRa TX failed");
        }
        
        vTaskDelayUntil(&last_wake_time, frequency);
    }
}

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

void setup() {
    // put your setup code here, to run once:

    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);

#if (1 == ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST)
    i2c_init(I2C_GPIO_SCL, I2C_GPIO_SDA);
#endif

    display_touch_lvgl_init();

    // Wireless TX init (LoRa sender initialization)
    while (1) {
        uint8_t sw_level;
        stc8_gpio_get_level(STC8_GPIO_IN_SW_SPI_UART, &sw_level);
        if (SW_LEVEL_SELECT_TO_WIRELESS != sw_level)
        {
            MAIN_ERROR("Please switch key to wireless. The initialization of wireless module failed!");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        } else {
            // lora tx init
            esp_err_t err = sx1262_tx_init();      // Initialize SX1262 LoRa sender
            if (err != ESP_OK) {  // Check error
                MAIN_ERROR("The initialization of wireless module failed!");  // Handle failure
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            } else {
                MAIN_INFO("The wireless module TX initialization was successful.");  // Print success log
                break;
            }
        }
    }

    lvgl_show_counter_label_init();
    MAIN_INFO("-------- LVGL Show OK ----------");
    
    // Create tasks and use the same priority to ensure synchronization
    xTaskCreatePinnedToCore(ui_counter_task, "ui_counter", 4096, NULL,
                                configMAX_PRIORITIES - 5, NULL, 0);

    xTaskCreatePinnedToCore(lora_tx_task, "sx1262_tx", 8192, NULL,
                                configMAX_PRIORITIES - 5, NULL, 1);

    MAIN_INFO("Tasks created, starting synchronized transmission...");

    delay(100);  // Wait lvgl run, Prevent the screen from flickering
    stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN, 100);     // set backlight (0~100)
}

void loop() {
    // put your main code here, to run repeatedly:
    
}
