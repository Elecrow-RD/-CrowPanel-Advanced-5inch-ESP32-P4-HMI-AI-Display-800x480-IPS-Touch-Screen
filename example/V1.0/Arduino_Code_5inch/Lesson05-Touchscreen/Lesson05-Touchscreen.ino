/**
 * @file Lesson05-Touchscreen.ino
 * @brief Implements the runnable Arduino example and demonstrates the lesson workflow.
 *
 * This file belongs to the Lesson05-Touchscreen course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Lesson05-Touchscreen Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

/**
 * IMPORTANT:
 * This code requires the "ESP32_Display_Panel" library.
 * Before uploading, you MUST configure the following file in your libraries folder:
 * ./esp_panel_board_custom_conf.h
 * 
 * 1. Enable RGB:      #define ESP_PANEL_BOARD_LCD_BUS_TYPE        (ESP_PANEL_BUS_TYPE_RGB)
 * 2. Set LCD Driver:  #define ESP_PANEL_BOARD_LCD_CONTROLLER      ST7262
 * 3. Set Touch:       #define ESP_PANEL_BOARD_TOUCH_CONTROLLER    GT911
 */
/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"   // board pin define
#include <Arduino.h>        // Arduino core library. Must be placed at the very top to ensure recognition of Arduino APIs

#include <string.h>         // C string lib
#include <esp_log.h>        // ESP-IDF logging library
#include <esp_err.h>        // ESP-IDF error codes

#include "esp_panel_drivers_conf.h"
#include "esp_panel_board_custom_conf.h"
#include "esp_display_panel.hpp"

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

/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
static const char *TAG = "TOUCH_APP";  // Tag for logging messages

// --- Declare the panel pointer globally ---
esp_panel::board::Board *panel = nullptr;
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */


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

    // --- Initialize Display and Touch Panel ---
    panel = new esp_panel::board::Board();

    // Initialize the bus and the devices (ST7265 & GT911)
    // The library uses settings from ESP_Panel_Conf.h internally
    Serial.println("Initializing Panel (ST7265 + GT911)...");
    if (!panel->init()) {
        Serial.println("Panel Initialization Failed!");
        while (1) delay(100);
    }

    // Begin the panel (Startup sequences and backlight)
    if (!panel->begin()) {
        Serial.println("Panel Start Failed!");
        while (1) delay(100);
    }

    Serial.println("Display and Touch system online.");
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
  
    // --- Process Touch Inputs ---
    auto touch = panel->getTouch();
    
    if (touch != nullptr) {
        // --- Read data from the hardware ---
        // Use the current ESP32_Display_Panel touch point type.
        esp_panel::drivers::TouchPoint point[10];  // save 10 points

        // --- Check the number of touch points ---
        int point_num = touch->readPoints(point, 10, 0);  // readPoints() ==> readRawData() + getPoints()

        for (int i=0; i<point_num; i++) {
            // Print the coordinates to Serial
            Serial.printf("Touch point[%d]: x %4d, y %3d, strength %d\n", i, point[i].x, point[i].y, point[i].strength);
        }

        if (!touch->isInterruptEnabled()) {
            delay(20);  // Small delay to maintain stability
        }
    } else {
        Serial.println("IDLE loop");
        delay(1000);
    }
}
