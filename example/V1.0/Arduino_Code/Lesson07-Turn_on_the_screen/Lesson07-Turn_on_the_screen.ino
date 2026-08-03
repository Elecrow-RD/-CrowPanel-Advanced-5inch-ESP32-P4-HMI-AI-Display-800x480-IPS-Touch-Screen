/**
 * @file Lesson07-Turn_on_the_screen.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson07-Turn_on_the_screen course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Lesson07-Turn On The Screen Module
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

#define MAIN_INFO(fmt, ...)         PRINTF_ERROR(fmt, ##__VA_ARGS__)  // Info level log macro
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
ESP_Panel *panel = nullptr;
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

/**
 * @brief LVGL text display function (display "Hello Elecrow")
 *
 * Called from the application flow when this operation is required.
 *
 * @return Nothing.
 */
static void lvgl_show_hello_elecrow(void) {
    // 1. Lock LVGL: ensure thread-safe operations
    if (lvgl_port_lock(-1) != true) {  // 0 means non-blocking wait for the lock (timeout = 0)
        MAIN_ERROR("LVGL lock failed");  // Print error if lock fails
        return;  // Exit function
    }

    // 2. Create screen background (optional: set background color for better text visibility)
    lv_obj_t *screen = lv_scr_act();  // Get current active screen object
    lv_obj_set_style_bg_color(screen, LV_COLOR_WHITE, LV_PART_MAIN);  // Set background color to white
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);      // Set background fully opaque

    // 3. Create label object (parent object = current screen)
    lv_obj_t *hello_label = lv_label_create(screen);  // Create label
    if (hello_label == NULL) {  // Check if creation failed
        MAIN_ERROR("Create LVGL label failed");  // Log error
        lvgl_port_unlock();  // Unlock LVGL before returning
        return;  // Exit function
    }

    // 4. Set label text content
    lv_label_set_text(hello_label, "Hello Elecrow");  // Set label text

    // 5. Configure label style (font, color, background)
    static lv_style_t label_style;  // Define a style object
    lv_style_init(&label_style);    // Initialize style object
    // Set font: Montserrat size 42 (must be enabled in LVGL config)
    lv_style_set_text_font(&label_style, &lv_font_montserrat_42);
    // Set text color to black (contrast with white background)
    lv_style_set_text_color(&label_style, LV_COLOR_BLACK);
    // Set label background transparent (avoid blocking screen background)
    lv_style_set_bg_opa(&label_style, LV_OPA_TRANSP);
    // Apply style to the label
    lv_obj_add_style(hello_label, &label_style, LV_PART_MAIN);

    // 6. Adjust label position: center on screen
    lv_obj_center(hello_label);

    // 7. Unlock LVGL: release lock, allow LVGL task to render
    lvgl_port_unlock();
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

    Serial.println("Creating UI");

    lvgl_show_hello_elecrow();

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
  
    delay(1000);
}
