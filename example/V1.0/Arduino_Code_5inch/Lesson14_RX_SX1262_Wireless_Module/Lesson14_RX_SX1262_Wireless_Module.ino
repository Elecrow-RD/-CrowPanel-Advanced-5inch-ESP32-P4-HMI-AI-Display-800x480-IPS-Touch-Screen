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
static lv_obj_t *s_rssi_label = NULL;           // LVGL label object to display RSSI value
static lv_obj_t *s_snr_label = NULL;            // LVGL label object to display SNR value
static uint32_t rx_packet_count = 0;            // Counter for the number of received LoRa packets
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

/**
 * @brief Callback function triggered when LoRa data is received
 */
static void rx_data_callback(const char* data, size_t len, float rssi, float snr)
{
    rx_packet_count++;  // Increment the received packet count each time data is received
    
    // (Update LVGL screen display)
    if (lvgl_port_lock(-1) == true) {  // Acquire LVGL lock before updating the UI to ensure thread safety
        // (Format received data as RX_Hello World:i)
        if (s_rx_label != NULL) {
            char rx_text[64];  // Buffer to store formatted text
            snprintf(rx_text, sizeof(rx_text), "RX_Hello World:%lu", (unsigned long)rx_packet_count);
            lv_label_set_text(s_rx_label, rx_text);  // Update the text of the RX label
        }
        
        //  (Update RSSI display)
        if (s_rssi_label != NULL) {
            char rssi_text[32];  // Buffer to store formatted RSSI text
            snprintf(rssi_text, sizeof(rssi_text), "RSSI: %.1f dBm", rssi);
            lv_label_set_text(s_rssi_label, rssi_text);  // Update the RSSI label text
        }
        
        //  (Update SNR display)
        if (s_snr_label != NULL) {
            char snr_text[32];  // Buffer to store formatted SNR text
            snprintf(snr_text, sizeof(snr_text), "SNR: %.1f dB", snr);
            lv_label_set_text(s_snr_label, snr_text);  // Update the SNR label text
        }
        
        lvgl_port_unlock();  // Release LVGL lock after updating the UI
    }
    
    char rx_display_text[64];  // Local buffer for logging display
    snprintf(rx_display_text, sizeof(rx_display_text), "RX_Hello World:%lu", (unsigned long)rx_packet_count);
    MAIN_INFO("RX: %s (RSSI: %.1f dBm, SNR: %.1f dB)", rx_display_text, rssi, snr);  // Log received data info to console
}

/**
 * @brief Initialize the LVGL display interface for LoRa RX
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

    // Create a common text style for RSSI and SNR labels
    static lv_style_t info_style;  
    lv_style_init(&info_style);  // Initialize LVGL style object
    lv_style_set_text_font(&info_style, &lv_font_montserrat_42);  // Set font size
    lv_style_set_text_color(&info_style, lv_color_black());       // Set text color to black
    lv_style_set_bg_opa(&info_style, LV_OPA_TRANSP);              // Set transparent background

    // (Create the title label)
    lv_obj_t *title_label = lv_label_create(screen);  // Create a new LVGL label for the title
    if (title_label != NULL) {
        lv_label_set_text(title_label, "LoRa RX Receiver");  // Set the title text
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
        lv_label_set_text(s_rx_label, "RX_Hello World:0");  // Set initial text content
        static lv_style_t rx_style;  // Create style for RX label
        lv_style_init(&rx_style);
        lv_style_set_text_font(&rx_style, &lv_font_montserrat_42);  // Font for RX text
        lv_style_set_text_color(&rx_style, lv_color_black());       // Black text color
        lv_style_set_bg_opa(&rx_style, LV_OPA_TRANSP);              // Transparent background
        lv_obj_add_style(s_rx_label, &rx_style, LV_PART_MAIN);      // Apply RX style
        lv_obj_align(s_rx_label, LV_ALIGN_CENTER, 0, -40);          // Align label near center
    }

    //  (Create RSSI display label)
    s_rssi_label = lv_label_create(screen);
    if (s_rssi_label != NULL) {
        lv_label_set_text(s_rssi_label, "RSSI: -- dBm");            // Initial RSSI text
        lv_obj_add_style(s_rssi_label, &info_style, LV_PART_MAIN);  // Apply common style
        lv_obj_align(s_rssi_label, LV_ALIGN_CENTER, -180, 150);     // Align to bottom-left area
    }

    //  (Create SNR display label with same style)
    s_snr_label = lv_label_create(screen);
    if (s_snr_label != NULL) {
        lv_label_set_text(s_snr_label, "SNR: -- dB");               // Initial SNR text
        lv_obj_add_style(s_snr_label, &info_style, LV_PART_MAIN);   // Apply shared style
        lv_obj_align(s_snr_label, LV_ALIGN_CENTER, 180, 150);       // Align to bottom-right area
    }

    lvgl_port_unlock();  // Unlock LVGL after all UI components are created
}

/**
 * @brief LoRa receive task that checks incoming packets continuously
 */
static void lora_rx_task(void *param)
{
    while (1) {  // Infinite loop for continuous checking
        //  (Check if data has been received)
        if (sx1262_is_data_received()) {
            // (Get actual received data length)
            size_t len = sx1262_get_received_len();
            received_lora_pack_radio(len);  // Handle received packet data
        }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Check every 10ms to reduce CPU usage
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
    
    lvgl_show_rx_interface_init();  // Initialize LVGL user interface
    MAIN_INFO("-------- LVGL RX Interface OK ----------");  // Log successful UI init

    // Wireless RX init (LoRa receiver initialization)
    while (1) {
        uint8_t sw_level;
        stc8_gpio_get_level(STC8_GPIO_IN_SW_SPI_UART, &sw_level);
        if (SW_LEVEL_SELECT_TO_WIRELESS != sw_level)
        {
            MAIN_ERROR("Please switch key to wireless. The initialization of wireless module failed!");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        } else {
            // lora rx init
            esp_err_t err = sx1262_rx_init();   // Initialize SX1262 LoRa receiver
            if (err != ESP_OK) {  // Check error
                MAIN_ERROR("The initialization of wireless module failed!");  // Handle failure
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            } else {
                MAIN_INFO("The wireless module RX initialization was successful.");  // Print success log
                break;
            }
        }
    }

    //  (Set callback function for received data)
    sx1262_set_rx_callback(rx_data_callback);  // Register LoRa RX callback function
    MAIN_INFO("RX callback registered");  // Log callback registration success

    // (Create LoRa receiving task)
    xTaskCreatePinnedToCore(lora_rx_task, "sx1262_rx", 4096, NULL,
                            configMAX_PRIORITIES - 5, NULL, 1);  // Create FreeRTOS task pinned to core 1

    MAIN_INFO("LoRa RX receiver started, waiting for data...");  // Log start message

    delay(100);  // Wait lvgl run, Prevent the screen from flickering
    stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN, 100);     // set backlight (0~100)
}

void loop() {
    // put your main code here, to run repeatedly:
    
}
