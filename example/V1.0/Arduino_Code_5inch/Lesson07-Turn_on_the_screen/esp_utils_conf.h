/*
 * Project-local esp-lib-utils configuration for ESP32-P4 ECO2.
 *
 * ESP32_Display_Panel stores its devices in std::shared_ptr instances. Keep
 * the objects and their atomic reference counters in internal SRAM; large
 * LVGL and RGB frame buffers continue to use PSRAM through their own APIs.
 */
#pragma once

#define ESP_UTILS_CONF_CHECK_HANDLE_METHOD                  (ESP_UTILS_CHECK_HANDLE_WITH_ERROR_LOG)
#define ESP_UTILS_CONF_LOG_IMPL_TYPE                        (ESP_UTILS_CONF_LOG_IMPL_STDLIB)
#define ESP_UTILS_CONF_LOG_LEVEL                            (ESP_UTILS_LOG_LEVEL_INFO)

#define ESP_UTILS_CONF_MEM_GEN_ALLOC_DEFAULT_ENABLE         (1)
#define ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE                   (ESP_UTILS_MEM_ALLOC_TYPE_ESP)
#define ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_CAPS               (ESP_UTILS_MEM_ALLOC_ESP_CAPS_SRAM)
#define ESP_UTILS_CONF_MEM_GEN_ALLOC_ESP_ALIGN              (4)
#define ESP_UTILS_CONF_MEM_ENABLE_CXX_GLOB_ALLOC            (0)

#define ESP_UTILS_CONF_FILE_VERSION_MAJOR 1
#define ESP_UTILS_CONF_FILE_VERSION_MINOR 4
#define ESP_UTILS_CONF_FILE_VERSION_PATCH 0
