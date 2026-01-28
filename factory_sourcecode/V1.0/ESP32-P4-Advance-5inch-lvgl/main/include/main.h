#ifndef _MAIN_H_
#define _MAIN_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_private/esp_clk.h"
#include "esp_ldo_regulator.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp_timer.h"
#include "Demos/widgets/lv_demo_widgets.h"
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_uart.h"
#include "bsp_stc8h1kxx.h"
#include "elecrow_ui.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/
/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define MAIN_TAG "MAIN"
#define MAIN_INFO(fmt, ...) ESP_LOGI(MAIN_TAG, fmt, ##__VA_ARGS__)
#define MAIN_DEBUG(fmt, ...) ESP_LOGD(MAIN_TAG, fmt, ##__VA_ARGS__)
#define MAIN_ERROR(fmt, ...) ESP_LOGE(MAIN_TAG, fmt, ##__VA_ARGS__)

#define SW_LEVEL_SELECT_TO_UART         (0)
#define SW_LEVEL_SELECT_TO_WIRELESS     (1)

typedef struct
{
    bool work_flag;
    bool audio_flag;
    bool camera_flag;
    bool display_flag;
    bool lora_flag;
    bool nrf24_flag;
    bool wifi_flag;
    bool bluetooth_flag;
    bool blight_flag;
    bool usb_flag;
} work_message;

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif