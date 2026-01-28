/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_uart.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#ifdef CONFIG_BSP_UART_ENABLED
static uart_state uart_status = UART_SCAN;
#endif
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
#ifdef CONFIG_BSP_UART_ENABLED

int SendData(const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_0, data, len);
    return txBytes;
}

esp_err_t uart_init()
{
    esp_err_t err = ESP_OK;
    const uart_config_t uart_config = {
        .baud_rate = 115200,                   // UART baud rate
        .data_bits = UART_DATA_8_BITS,         // UART byte size
        .parity = UART_PARITY_DISABLE,         // UART parity mode
        .stop_bits = UART_STOP_BITS_1,         // UART stop bits
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // UART HW flow control mode (cts/rts)
        .source_clk = UART_SCLK_DEFAULT,       // UART source clock selection
    };
    err = uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, 0);
    if (err != ESP_OK)
    {
        UART_ERROR("uart driver install fail");
        return err;
    }
    err = uart_param_config(UART_NUM_0, &uart_config);
    if (err != ESP_OK)
        return err;
    return err;
}

uart_state get_uart_status()
{
    return uart_status;
}

void set_uart_status(uart_state status)
{
    uart_status = status;
}
#endif
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/