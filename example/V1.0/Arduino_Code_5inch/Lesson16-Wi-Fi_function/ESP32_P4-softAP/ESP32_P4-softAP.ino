/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"           // board pin define

/* Wireless Connectivity (WiFi) */
#include <WiFi.h>                   // Standard ESP32 WiFi library
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
const char* ap_ssid      = "ELECROW";
const char* ap_password  = "12345678";
/* —————————————————————————————————————————————————————————————————————— 
                             FUNCTION PROTOTYPES 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                                  FUNCTIONS 
————————————————————————————————————————————————————————————————————————— */

void setup() {
    Serial.begin(115200);

    // ESP-Hosted-MCU SDIO Interface Pins for WiFi
    WiFi.setPins(
        WIFI_HOSTED_SDIO_PIN_CLK, 
        WIFI_HOSTED_SDIO_PIN_CMD, 
        WIFI_HOSTED_SDIO_PIN_D0, 
        WIFI_HOSTED_SDIO_PIN_D1,
        WIFI_HOSTED_SDIO_PIN_D2,
        WIFI_HOSTED_SDIO_PIN_D3,
        WIFI_HOSTED_SDIO_PIN_RESET
    );

    // Initialize WiFi in Access Point mode
    // Parameters: SSID, Password, Channel, Hidden(bool), Max_Connections
    if (WiFi.softAP(ap_ssid, ap_password)) {
        Serial.println("Access Point Started Successfully");
    } else {
        Serial.println("Access Point Failed to Start");
    }

    // Default IP for AP mode is usually 192.168.4.1
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
}

void loop() {
    // Monitor connected clients
    int clients = WiFi.softAPgetStationNum();
    Serial.printf("Connected clients: %d\n", clients);
    
    delay(2000);
}
