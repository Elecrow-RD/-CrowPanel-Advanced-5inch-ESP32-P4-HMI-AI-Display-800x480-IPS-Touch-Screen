/**
 * @file main.h
 * @brief Teaching source for 5inch_P4_IDF_09_LVGL_Lighting_Control.
 *
 * This file is part of the CrowPanel Advanced 5-inch ESP32-P4 course.
 * The comments explain module responsibilities and observable behavior
 * without changing the original program logic.
 */

#ifndef _MAIN_H_
#define _MAIN_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "bsp_extra.h"
#include "bsp_display.h"
#include "bsp_illuminate.h"
#include "bsp_i2c.h"
#include "bsp_stc8h1kxx.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

/* Log macro definition */
#define MAIN_TAG "MAIN"
#define MAIN_INFO(fmt, ...) ESP_LOGI(MAIN_TAG, fmt, ##__VA_ARGS__)
#define MAIN_DEBUG(fmt, ...) ESP_LOGD(MAIN_TAG, fmt, ##__VA_ARGS__)
#define MAIN_ERROR(fmt, ...) ESP_LOGE(MAIN_TAG, fmt, ##__VA_ARGS__)

/*—————————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif