/* —————————————————————————————————————————————————————————————————————— 
                                 INCLUDES 
————————————————————————————————————————————————————————————————————————— */
#include "board_config.h"           // board pin define

/* Wireless Connectivity (WiFi) */
#include <WiFi.h>                   // Standard ESP32 WiFi library
#include <lwip/lwip_napt.h>         // Required for NAT functionality
/* —————————————————————————————————————————————————————————————————————— 
                                DEFINITIONS 
————————————————————————————————————————————————————————————————————————— */

/* —————————————————————————————————————————————————————————————————————— 
                              GLOBAL VARIABLES 
————————————————————————————————————————————————————————————————————————— */
const char* ap_ssid      = "ELECROW";
const char* ap_password  = "12345678";

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

    // 1. Set mode to AP + STA
    WiFi.mode(WIFI_AP_STA);

    // 2. Start AP
    WiFi.softAP(ap_ssid, ap_password);
    
    // 3. Connect to Router
    WiFi.begin(sta_ssid, sta_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // 4. ENABLE NAT (The magic happens here)
    // This allows traffic to flow from AP to STA
    ip_napt_enable(WiFi.softAPIP(), 1); 
    
    Serial.println("\nNAT Enabled. Mobile device should have internet now.");
}

void loop() {
    // Check Station connection status
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[STA] Connected. IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("[STA] Connecting to router...");
    }
    
    // Display AP status
    Serial.print("[AP] Hotspot IP: ");
    Serial.println(WiFi.softAPIP());
    
    Serial.print("[AP] Stations connected: ");
    Serial.println(WiFi.softAPgetStationNum());

    delay(5000);
}
