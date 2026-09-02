/**
 * @file board_config.h
 * @brief Defines the hardware and library configuration used by the lesson.
 *
 * This file belongs to the Lesson16-Wi-Fi_function course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

/*---------------------------------------------------------------
 * Board Config Module
 * Keep the related declarations and implementation details together.
 *--------------------------------------------------------------*/

#pragma once

#define FIRMWARE_VERSION_V1_0

/*********************** Pin define ***********************/
/**
 * SDIO Interface Pins for ESP-Hosted-MCU
 * Used for high-speed communication between ESP32-P4 (Host) and ESP32-C6 (Slave)
 */
#if defined(FIRMWARE_VERSION_V1_0)

#define WIFI_HOSTED_SDIO_PIN_CMD            (54) // SDIO Command/Response line
#define WIFI_HOSTED_SDIO_PIN_CLK            (53) // SDIO Serial Clock
#define WIFI_HOSTED_SDIO_PIN_D0             (52) // SDIO Data line 0
#define WIFI_HOSTED_SDIO_PIN_D1             (51) // SDIO Data line 1
#define WIFI_HOSTED_SDIO_PIN_D2             (50) // SDIO Data line 2
#define WIFI_HOSTED_SDIO_PIN_D3             (49) // SDIO Data line 3 (4-bit mode)
#define WIFI_HOSTED_SDIO_PIN_RESET          (20) // Hardware Reset for the ESP32-C6 co-processor

#endif
/*********************** Pin define ***********************/
