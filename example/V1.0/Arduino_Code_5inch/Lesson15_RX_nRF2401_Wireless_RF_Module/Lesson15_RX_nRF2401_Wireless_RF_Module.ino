/**
 * @file Lesson15_RX_nRF2401_Wireless_RF_Module.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson15_RX_nRF2401_Wireless_RF_Module course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Lesson15 Receive N Rf2401 Wireless Rf Module Module
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

/* panel driver */
#include "esp_panel_board_custom_conf.h"
#include <ESP_Panel_Library.h>

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
static lv_obj_t *s_rx_label = NULL;             // LVGL label object to display received data
static uint32_t rx_packet_count = 0;            // Counter for the number of received nRF24L01 packets
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

/**
 * @brief Callback function triggered when nRF24L01 data is received
 *
 * Called by the owning driver or framework when the event occurs.
 *
 * @param data Value supplied by the caller for this operation.
 *
 * @param len Value supplied by the caller for this operation.
 *
 * @return Nothing.
 */
static void rx_data_callback(const char* data, size_t len)
{
    rx_packet_count++;  // Increment the received packet count each time data is received
    
    // (Update LVGL screen display)
    if (lvgl_port_lock(-1) == true) {  // Acquire LVGL lock before updating the UI to ensure thread safety
        // (Format received data as NRF24_RX_Hello World:i)
        if (s_rx_label != NULL) {
            char rx_text[64];  // Buffer to store formatted text
            snprintf(rx_text, sizeof(rx_text), "NRF24_RX_Hello World:%lu", (unsigned long)rx_packet_count);
            lv_label_set_text(s_rx_label, rx_text);  // Update the text of the RX label
        }
        
        lvgl_port_unlock();  // Release LVGL lock after updating the UI
    }
    
    char rx_display_text[64];  // Local buffer for logging display
    snprintf(rx_display_text, sizeof(rx_display_text), "NRF24_RX_Hello World:%lu", (unsigned long)rx_packet_count);
    MAIN_INFO("NRF24 RX: %s", rx_display_text);  // Log received data info to console
}

/**
 * @brief Initialize the LVGL display interface for nRF24L01 RX
 *
 * Called during application startup before the feature is used.
 *
 * @return Nothing.
 */
static void lvgl_show_rx_interface_init(void)
{
    if (lvgl_port_lock(-1) != true) {  // Try to lock LVGL before creating objects
        MAIN_ERROR("LVGL lock failed");  // Log error if lock acquisition fails
        return;  // Exit the function
    }

    lv_obj_t *screen = lv_scr_act();  // Get the current active LVGL screen object
    lv_obj_set_style_bg_color(screen, LV_COLOR_WHITE, LV_PART_MAIN);  // Set the screen background color to white
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);      // Set background opacity to fully opaque

    // (Create the title label)
    lv_obj_t *title_label = lv_label_create(screen);  // Create a new LVGL label for the title
    if (title_label != NULL) {
        lv_label_set_text(title_label, "nRF24L01 RX Receiver");  // Set the title text
        static lv_style_t title_style;  // Define a separate style for the title
        lv_style_init(&title_style);  
        lv_style_set_text_font(&title_style, &lv_font_montserrat_42);  // Use large font for title
        lv_style_set_text_color(&title_style, lv_color_black());       // Set text color
        lv_style_set_bg_opa(&title_style, LV_OPA_TRANSP);              // Make background transparent
        lv_obj_add_style(title_label, &title_style, LV_PART_MAIN);     // Apply the style to title label
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);            // Align title to top center with Y offset
    }

    //  (Create label to show received message)
    s_rx_label = lv_label_create(screen);  // Create label object for RX message display
    if (s_rx_label != NULL) {
        lv_label_set_text(s_rx_label, "NRF24_RX_Hello World:0");  // Set initial text content
        static lv_style_t rx_style;  // Create style for RX label
        lv_style_init(&rx_style);
        lv_style_set_text_font(&rx_style, &lv_font_montserrat_42);  // Font for RX text
        lv_style_set_text_color(&rx_style, lv_color_black());       // Black text color
        lv_style_set_bg_opa(&rx_style, LV_OPA_TRANSP);              // Transparent background
        lv_obj_add_style(s_rx_label, &rx_style, LV_PART_MAIN);      // Apply RX style
        lv_obj_align(s_rx_label, LV_ALIGN_CENTER, 0, -40);          // Align label near center
    }

    lvgl_port_unlock();  // Unlock LVGL after all UI components are created
}

/**
 * @brief nRF24L01 receive task that checks incoming packets continuously
 *
 * Called by the FreeRTOS scheduler after the task is created.
 *
 * @param param Value supplied by the caller for this operation.
 *
 * @return Nothing.
 */
static void nrf24_rx_task(void *param)
{
    while (1) {  // Infinite loop for continuous checking
        //  (Check if data has been received)
        // Note: nRF24L01 doesn't have the same data received flag as SX1262
        // We'll use a reasonable maximum packet size for nRF24L01 (32 bytes)
        received_nrf24_pack_radio(32);  // Handle received packet data with maximum size
        vTaskDelay(10 / portTICK_PERIOD_MS); // Check every 10ms to reduce CPU usage
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

    // nRF24L01 Wireless RX init (nRF24L01 receiver initialization)
    while (1) {
        uint8_t sw_level;
        stc8_gpio_get_level(STC8_GPIO_IN_SW_SPI_UART, &sw_level);
        if (SW_LEVEL_SELECT_TO_WIRELESS != sw_level)
        {
            MAIN_ERROR("Please switch key to wireless. The initialization of wireless module failed!");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        } else {
            // nRF24L01 rx init
            esp_err_t err = nrf24_rx_init();  // Initialize nRF24L01 receiver
            if (err != ESP_OK) {  // Check error
                MAIN_ERROR("nRF24L01 Wireless Module RX init failed");  // Halt if failed
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            } else {
                MAIN_INFO("The nRF24L01 wireless module RX initialization was successful.");  // Log success
                break;
            }
        }
    }

    lvgl_show_rx_interface_init();  // Initialize LVGL user interface
    MAIN_INFO("-------- LVGL RX Interface OK ----------");  // Log successful UI init

    //  (Set callback function for received data)
    nrf24_set_rx_callback(rx_data_callback);  // Register nRF24L01 RX callback function
    MAIN_INFO("RX callback registered");  // Log callback registration success

    // (Create nRF24L01 receiving task)
    xTaskCreatePinnedToCore(nrf24_rx_task, "nrf24_rx", 4096, NULL,
                            configMAX_PRIORITIES - 5, NULL, 1);  // Create FreeRTOS task pinned to core 1

    MAIN_INFO("nRF24L01 RX receiver started, waiting for data...");  // Log start message

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
    
}
