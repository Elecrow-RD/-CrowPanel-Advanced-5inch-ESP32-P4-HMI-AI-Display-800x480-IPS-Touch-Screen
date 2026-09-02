/**
 * @file lvgl_v9_port.h
 * @brief LVGL 9.1 display and touch port for ESP32_Display_Panel.
 */
#pragma once

#include "sdkconfig.h"
#ifdef CONFIG_ARDUINO_RUNNING_CORE
#include <Arduino.h>
#endif
#include "esp_display_panel.hpp"
#include "lvgl.h"
#include "board_config.h"

#define LVGL_PORT_TICK_PERIOD_MS                (2)

// These buffers are used only when anti-tearing is disabled.
#define LVGL_PORT_BUFFER_MALLOC_CAPS            (MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA | MALLOC_CAP_8BIT)
#define LVGL_PORT_BUFFER_SIZE_HEIGHT            (V_size)
#define LVGL_PORT_BUFFER_NUM                    (2)

#define LVGL_PORT_TASK_MAX_DELAY_MS             (500)
#define LVGL_PORT_TASK_MIN_DELAY_MS             (2)
#define LVGL_PORT_TASK_STACK_SIZE               (6 * 1024)
#define LVGL_PORT_TASK_PRIORITY                 (15)
#ifdef ARDUINO_RUNNING_CORE
#define LVGL_PORT_TASK_CORE                     (ARDUINO_RUNNING_CORE)
#else
#define LVGL_PORT_TASK_CORE                     (0)
#endif

/**
 * Anti-tearing modes:
 * 0: disabled
 * 1: two LCD frame buffers, LVGL full render mode
 * 2: three LCD frame buffers, LVGL full render mode
 * 3: two LCD frame buffers, LVGL direct render mode
 */
#ifdef CONFIG_LVGL_PORT_AVOID_TEARING_MODE
#define LVGL_PORT_AVOID_TEARING_MODE            (CONFIG_LVGL_PORT_AVOID_TEARING_MODE)
#else
#define LVGL_PORT_AVOID_TEARING_MODE            (1)
#endif

#if LVGL_PORT_AVOID_TEARING_MODE != 0
#ifdef CONFIG_LVGL_PORT_ROTATION_DEGREE
#define LVGL_PORT_ROTATION_DEGREE               (CONFIG_LVGL_PORT_ROTATION_DEGREE)
#else
#define LVGL_PORT_ROTATION_DEGREE               (0)
#endif

#define LVGL_PORT_AVOID_TEAR                    (1)
#if LVGL_PORT_AVOID_TEARING_MODE == 1
#define LVGL_PORT_DISP_BUFFER_NUM               (2)
#define LVGL_PORT_FULL_REFRESH                  (1)
#elif LVGL_PORT_AVOID_TEARING_MODE == 2
#define LVGL_PORT_DISP_BUFFER_NUM               (3)
#define LVGL_PORT_FULL_REFRESH                  (1)
#elif LVGL_PORT_AVOID_TEARING_MODE == 3
#define LVGL_PORT_DISP_BUFFER_NUM               (2)
#define LVGL_PORT_DIRECT_MODE                   (1)
#else
#error "Invalid LVGL_PORT_AVOID_TEARING_MODE"
#endif

#if (LVGL_PORT_ROTATION_DEGREE != 0) && (LVGL_PORT_ROTATION_DEGREE != 90) && \
    (LVGL_PORT_ROTATION_DEGREE != 180) && (LVGL_PORT_ROTATION_DEGREE != 270)
#error "LVGL_PORT_ROTATION_DEGREE must be 0, 90, 180 or 270"
#elif LVGL_PORT_ROTATION_DEGREE != 0
#undef LVGL_PORT_DISP_BUFFER_NUM
#define LVGL_PORT_DISP_BUFFER_NUM               (3)
#endif
#else
#define LVGL_PORT_AVOID_TEAR                    (0)
#define LVGL_PORT_ROTATION_DEGREE               (0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool lvgl_port_init(esp_panel::drivers::LCD *lcd, esp_panel::drivers::Touch *tp);
bool lvgl_port_deinit(void);
bool lvgl_port_lock(int timeout_ms);
bool lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif
