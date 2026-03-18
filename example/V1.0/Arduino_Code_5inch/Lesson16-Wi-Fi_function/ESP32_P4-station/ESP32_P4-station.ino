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
const char* sta_ssid     = "yanfa_software";
const char* sta_password = "yanfa-123456";
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
    
    // Set WiFi to Station mode
    WiFi.mode(WIFI_STA);
    
    // Start connection process
    WiFi.begin(sta_ssid, sta_password);
    Serial.print("Connecting to WiFi");

    // Wait until connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Success message
    Serial.println("\nConnected!");
    Serial.print("Local IP Address: ");
    Serial.println(WiFi.localIP()); 
}

void loop() {
    // Check connection status periodically
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost. Reconnecting...");
        WiFi.begin(sta_ssid, sta_password);
    }
    delay(10000);
}
