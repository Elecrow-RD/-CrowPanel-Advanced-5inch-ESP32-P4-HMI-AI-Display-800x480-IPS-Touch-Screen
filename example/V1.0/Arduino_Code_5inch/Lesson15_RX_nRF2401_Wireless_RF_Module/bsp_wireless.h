/**
 * @file bsp_wireless.h
 * @brief Provides the board-support interface used to control a lesson peripheral.
 *
 * This file belongs to the Lesson15_RX_nRF2401_Wireless_RF_Module course project. Comments explain the
 * teaching flow without changing the original program behavior.
 */

#ifndef _BSP_WIRELESS_H
#define _BSP_WIRELESS_H

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "board_config.h"

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define SX1262_TAG "SX1262"
#define SX1262_INFO(fmt, ...) ESP_LOGI(SX1262_TAG, fmt, ##__VA_ARGS__)
#define SX1262_DEBUG(fmt, ...) ESP_LOGD(SX1262_TAG, fmt, ##__VA_ARGS__)
#define SX1262_ERROR(fmt, ...) ESP_LOGE(SX1262_TAG, fmt, ##__VA_ARGS__)

#define NRF2401_TAG "NRF2401"
#define NRF2401_INFO(fmt, ...) ESP_LOGI(NRF2401_TAG, fmt, ##__VA_ARGS__)
#define NRF2401_DEBUG(fmt, ...) ESP_LOGD(NRF2401_TAG, fmt, ##__VA_ARGS__)
#define NRF2401_ERROR(fmt, ...) ESP_LOGE(NRF2401_TAG, fmt, ##__VA_ARGS__)

#define WIRELESS_UART_TAG "WIRELESS_UART"
#define WIRELESS_UART_INFO(fmt, ...) ESP_LOGI(WIRELESS_UART_TAG, fmt, ##__VA_ARGS__)
#define WIRELESS_UART_DEBUG(fmt, ...) ESP_LOGD(WIRELESS_UART_TAG, fmt, ##__VA_ARGS__)
#define WIRELESS_UART_ERROR(fmt, ...) ESP_LOGE(WIRELESS_UART_TAG, fmt, ##__VA_ARGS__)

#define RADIO_GPIO_CLK      PIN_SPI_SCK
#define RADIO_GPIO_MISO     PIN_SPI_MISO
#define RADIO_GPIO_MOSI     PIN_SPI_MOSI

#define CONFIG_BSP_SX1262_ENABLED           0
#define CONFIG_BSP_NRF2401_ENABLED          1
#define CONFIG_BSP_UART_TRANSPOND_ENABLED   0
//---------------------------------------------------------------------------
#if CONFIG_BSP_SX1262_ENABLED

#define SX1262_GPIO_BUSY    LORA_DIO2
#define SX1262_GPIO_IRQ     LORA_DIO1
#define SX1262_GPIO_NRST    LORA_RESET
#define SX1262_GPIO_NSS     LORA_CS

#ifdef __cplusplus
extern "C"
{
#endif
    esp_err_t sx1262_tx_init();
    void sx1262_tx_deinit();
    bool send_lora_pack_radio();
    
    uint32_t sx1262_get_tx_counter();

    esp_err_t sx1262_rx_init();
    void sx1262_rx_deinit();
    void received_lora_pack_radio(size_t len);
    void sx1262_set_rx_callback(void (*callback)(const char* data, size_t len, float rssi, float snr));
    size_t sx1262_get_received_len(void);
    bool sx1262_is_data_received(void);
#ifdef __cplusplus
}
#endif

#endif
//---------------------------------------------------------------------------

#if CONFIG_BSP_NRF2401_ENABLED

#define NRF24_GPIO_IRQ      NRF24_IRQ
#define NRF24_GPIO_CE       NRF24_CE
#define NRF24_GPIO_CS       NRF24_CS

#ifdef __cplusplus
extern "C"
{
#endif
    esp_err_t nrf24_tx_init();
    void nrf24_tx_deinit();
    bool send_nrf24_pack_radio();
    uint32_t nrf24_get_tx_counter();
    esp_err_t nrf24_rx_init();
    void nrf24_rx_deinit();
    void received_nrf24_pack_radio(size_t len);
    void nrf24_set_rx_callback(void (*callback)(const char* data, size_t len));
#ifdef __cplusplus
}
#endif
#endif
//---------------------------------------------------------------------------

#if CONFIG_BSP_UART_TRANSPOND_ENABLED

#define UART_GPIO_TXD 31
#define UART_GPIO_RXD 32
#ifdef __cplusplus
extern "C"
{
#endif
    esp_err_t uart_transpond_init();
    void uart_transpond_deinit();
#ifdef __cplusplus
}
#endif
#endif

//---------------------------------------------------------------------------
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif
