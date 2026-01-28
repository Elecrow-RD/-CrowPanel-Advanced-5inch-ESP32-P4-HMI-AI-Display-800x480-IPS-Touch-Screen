#ifndef _BSP_UART_H_
#define _BSP_UART_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define UART_TAG "UART"
#define UART_INFO(fmt, ...) ESP_LOGI(UART_TAG, fmt, ##__VA_ARGS__)
#define UART_DEBUG(fmt, ...) ESP_LOGD(UART_TAG, fmt, ##__VA_ARGS__)
#define UART_ERROR(fmt, ...) ESP_LOGE(UART_TAG, fmt, ##__VA_ARGS__)

#ifdef CONFIG_BSP_UART_ENABLED

typedef enum
{
    UART_SCAN = 1,
    UART_DECODE,
    UART_ERR,
} uart_state;

int SendData(const char *data);
esp_err_t uart_init();
uart_state get_uart_status();
void set_uart_status(uart_state status);

#endif
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif