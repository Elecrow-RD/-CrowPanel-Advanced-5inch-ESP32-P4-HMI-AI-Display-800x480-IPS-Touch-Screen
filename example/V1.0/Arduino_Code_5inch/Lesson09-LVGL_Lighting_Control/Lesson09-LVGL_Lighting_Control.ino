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

using namespace esp_panel::drivers;
using namespace esp_panel::board;
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */
#define PRINTF_ORIGINAL(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__);
#define PRINTF_PRINT(fmt, ...)    Serial.print(fmt);
#define PRINTF_LN(fmt, ...)       Serial.println(fmt);

#define PRINTF_ERROR(fmt, ...)      do { \
                                        Serial.print("ERROR"); \
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
static const char *TAG = "TOUCH_APP";  // Tag for logging messages

// --- Declare the panel pointer globally ---
ESP_Panel *panel = nullptr;
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

/* Button callback function - turn on LED */
static void btn_on_click_event(lv_event_t *e)
{
    (void)e;
    digitalWrite(PIN_LED, LED_ON);  // Turn on LED on GPIO48
    MAIN_INFO("LED turned ON");
}

/* Button callback function - turn off LED */
static void btn_off_click_event(lv_event_t *e)
{
    (void)e;
    digitalWrite(PIN_LED, LED_OFF); // Turn off LED on GPIO48
    MAIN_INFO("LED turned OFF");
}

/* Create LED control UI */
static void create_led_control_ui(void)
{
    // Lock LVGL: ensure thread-safe operations
    if (lvgl_port_lock(-1) != true) {  // -1 means blocking wait for the lock (timeout = portMAX_DELAY)
        MAIN_ERROR("LVGL lock failed");  // Print error if lock fails
        return;  // Exit function
    }
    // Create main screen
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), LV_PART_MAIN);  // Set white background

    // Create title label
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "LED Controller");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 50);
    // Font size
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);

    // Create LED ON button
    lv_obj_t *btn_on = lv_btn_create(scr);
    lv_obj_set_size(btn_on, 120, 50);
    lv_obj_align(btn_on, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_event_cb(btn_on, btn_on_click_event, LV_EVENT_CLICKED, NULL);

    // ON button label
    lv_obj_t *label_on = lv_label_create(btn_on);
    lv_label_set_text(label_on, "LED ON");

    // Create LED OFF button
    lv_obj_t *btn_off = lv_btn_create(scr);
    lv_obj_set_size(btn_off, 120, 50);
    lv_obj_align(btn_off, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(btn_off, btn_off_click_event, LV_EVENT_CLICKED, NULL);

    // OFF button label
    lv_obj_t *label_off = lv_label_create(btn_off);
    lv_label_set_text(label_off, "LED OFF");

    lvgl_port_unlock();
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

    /* print LCD config */
    // Bus *lcd_bus = lcd->getBus();
    // static_cast<BusRGB *>(lcd_bus)->getConfig().printRefreshPanelConfig();
}

void setup() {
    // put your setup code here, to run once:

    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);

#if (1 == ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST)
    i2c_init(I2C_GPIO_SCL, I2C_GPIO_SDA);
#endif

    //Since the display library automatically initializes the I2C bus for touch, there is no need to initialize the i2c bus again
    //If you want to cancel the i2c bus initialization of the display library, modify the macro definition in the file esp_panel_board_custom_conf.h
    /**
     * If set to 1, the bus will skip to initialize the corresponding host. Users need to initialize the host in advance.
     *
     * For drivers which created by this library, even if they use the same host, the host will be initialized only once.
     * So it is not necessary to set the macro to `1`. For other drivers (like `Wire`), please set the macro to `1`
     * ensure that the host is initialized only once.
     */
    // #define ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST        (0)     // 0/1. Typically set to 0
    display_touch_lvgl_init();

    // Initialize LED control GPIO (GPIO48)
    MAIN_INFO("Initializing GPIO48 for LED...");
    pinMode(PIN_LED, OUTPUT); 
    digitalWrite(PIN_LED, LED_OFF);
    MAIN_INFO("LED initialized to OFF state");

    create_led_control_ui();

    delay(100);  // Wait lvgl run, Prevent the screen from flickering
    stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN, 100);     // set backlight (0~100)
}

void loop() {
    // put your main code here, to run repeatedly:
  
    delay(1000);
}
