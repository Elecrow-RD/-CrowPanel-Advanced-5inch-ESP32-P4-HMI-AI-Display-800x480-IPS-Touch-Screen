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

#include "USB.h"
#include "USBHIDMouse.h"

#include "esp_panel_drivers_conf.h"
#include "esp_panel_board_custom_conf.h"
#include "ESP_Panel_Library.h"

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
// --- Declare the panel pointer globally ---
ESP_Panel *panel = nullptr;

// Create a mouse object
// Mouse instance to control cursor and buttons
USBHIDMouse Mouse;
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

void setup() {
    // put your setup code here, to run once:

    // Initialize the default Serial for debugging (UART0)
    Serial.begin(115200);

    // initialize mouse control:
    Mouse.begin();
    USB.begin();

    // --- Initialize Display and Touch Panel ---
    panel = new ESP_Panel();

    // Initialize the bus (MIPI-DSI) and the devices (EK79007 & GT911)
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

void loop() {
    // put your main code here, to run repeatedly:
  
    // --- Process Touch Inputs ---
    auto touch = panel->getTouch();
    
    if (touch != nullptr) {
        // --- Read data from the hardware ---
        // Use the correct struct name: ESP_PanelTouchPoint
        ESP_PanelTouchPoint point[10];  // save 10 points

        // --- Check the number of touch points ---
        int point_num = touch->readPoints(point, 10, 0);  // readPoints() ==> readRawData() + getPoints()
        for (int i=0; i<point_num; i++) {
            // Print the coordinates to Serial
            Serial.printf("Touch point[%d]: x %4d, y %3d, strength %d\r\n", i, point[i].x, point[i].y, point[i].strength);
        }

        static int prev_x = -1;
        static int prev_y = -1;
        if (0 < point_num) {
            // calculate the movement distance based on the button states:
            if (-1 != prev_x || -1 != prev_y) {
                int xDistance = (point[0].x - prev_x);
                int yDistance = (point[0].y - prev_y);
                // if X or Y is non-zero, move:
                if ((xDistance!=0) || (yDistance!=0)) {
                    Mouse.move(xDistance, yDistance, 0);
                    Serial.printf("Mouse.move: x = %2d, y = %2d\r\n", xDistance, yDistance);
                }
            }
            prev_x = point[0].x;
            prev_y = point[0].y;

            // if the mouse is not pressed, press it:
            if (!Mouse.isPressed(MOUSE_LEFT)) {
                Mouse.press(MOUSE_LEFT);
            }
        } else {
            prev_x = -1;
            prev_y = -1;

            // if the mouse is pressed, release it:
            if (Mouse.isPressed(MOUSE_LEFT)) {
                Mouse.release(MOUSE_LEFT);
            }
        }

        // a delay so the mouse doesn't move too fast:
        delay(20);

    } else {
        Serial.println("IDLE loop");
        delay(1000);
    }
}
