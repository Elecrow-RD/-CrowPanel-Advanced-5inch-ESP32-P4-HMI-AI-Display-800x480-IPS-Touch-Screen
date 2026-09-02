/**
 * @file lvgl_v9_port.cpp
 * @brief LVGL 9.1 display and touch port for ESP32_Display_Panel.
 */

#include <limits.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"

#undef ESP_UTILS_LOG_TAG
#define ESP_UTILS_LOG_TAG "LvPort"
#include "esp_lib_utils.h"
#include "lvgl_v9_port.h"

using namespace esp_panel::drivers;

static constexpr uint32_t LVGL_PORT_RGB565_BYTES_PER_PIXEL = 2;
static constexpr int LVGL_PORT_BUFFER_NUM_MAX = 2;

static SemaphoreHandle_t lvgl_mux = nullptr;
static SemaphoreHandle_t touch_detected = nullptr;
static TaskHandle_t lvgl_task_handle = nullptr;
static esp_timer_handle_t lvgl_tick_timer = nullptr;
static lv_display_t *lvgl_display = nullptr;
static lv_indev_t *lvgl_indev = nullptr;
static void *lvgl_buf[LVGL_PORT_BUFFER_NUM_MAX] = {};
static LCD *lvgl_lcd = nullptr;

#if LVGL_PORT_AVOID_TEAR && (LVGL_PORT_ROTATION_DEGREE != 0)
static uint8_t rotation_frame_buffer_index = 0;

static void rotate_rgb565_full_frame(const uint16_t *src, uint16_t *dst, int physical_width, int physical_height)
{
    if (src == nullptr || dst == nullptr) {
        return;
    }

#if LVGL_PORT_ROTATION_DEGREE == 90
    const int logical_width = physical_height;
    const int logical_height = physical_width;
    for (int y = 0; y < logical_height; ++y) {
        for (int x = 0; x < logical_width; ++x) {
            dst[x * physical_width + (physical_width - 1 - y)] = src[y * logical_width + x];
        }
    }
#elif LVGL_PORT_ROTATION_DEGREE == 180
    for (int y = 0; y < physical_height; ++y) {
        for (int x = 0; x < physical_width; ++x) {
            dst[(physical_height - 1 - y) * physical_width + (physical_width - 1 - x)] =
                src[y * physical_width + x];
        }
    }
#elif LVGL_PORT_ROTATION_DEGREE == 270
    const int logical_width = physical_height;
    const int logical_height = physical_width;
    for (int y = 0; y < logical_height; ++y) {
        for (int x = 0; x < logical_width; ++x) {
            dst[(physical_height - 1 - x) * physical_width + y] = src[y * logical_width + x];
        }
    }
#endif
}
#endif

static void wait_for_lcd_vsync(void)
{
    ulTaskNotifyValueClear(nullptr, ULONG_MAX);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

static void flush_callback(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    LCD *lcd = static_cast<LCD *>(lv_display_get_user_data(display));
    if (lcd == nullptr) {
        lv_display_flush_ready(display);
        return;
    }

#if LVGL_PORT_AVOID_TEAR
    if (lv_display_flush_is_last(display)) {
#if LVGL_PORT_ROTATION_DEGREE == 0
        lcd->switchFrameBufferTo(px_map);
#else
        void *next_fb = lcd->getFrameBufferByIndex(rotation_frame_buffer_index);
        rotation_frame_buffer_index ^= 1U;
        rotate_rgb565_full_frame(
            reinterpret_cast<const uint16_t *>(px_map), reinterpret_cast<uint16_t *>(next_fb),
            lcd->getFrameWidth(), lcd->getFrameHeight()
        );
        lcd->switchFrameBufferTo(next_fb);
#endif
        wait_for_lcd_vsync();
    }
    lv_display_flush_ready(display);
#else
    lcd->drawBitmap(
        area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1,
        reinterpret_cast<const uint8_t *>(px_map)
    );
    if (lcd->getBus()->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        lv_display_flush_ready(display);
    }
#endif
}

static void rounder_event_callback(lv_event_t *event)
{
    LCD *lcd = static_cast<LCD *>(lv_event_get_user_data(event));
    lv_area_t *area = static_cast<lv_area_t *>(lv_event_get_param(event));
    if (lcd == nullptr || area == nullptr) {
        return;
    }

    const uint8_t x_align = lcd->getBasicAttributes().basic_bus_spec.x_coord_align;
    const uint8_t y_align = lcd->getBasicAttributes().basic_bus_spec.y_coord_align;
    if (x_align > 1) {
        area->x1 &= ~(x_align - 1);
        area->x2 = (area->x2 & ~(x_align - 1)) + x_align - 1;
    }
    if (y_align > 1) {
        area->y1 &= ~(y_align - 1);
        area->y2 = (area->y2 & ~(y_align - 1)) + y_align - 1;
    }
}

static lv_display_t *display_init(LCD *lcd)
{
    ESP_UTILS_CHECK_FALSE_RETURN(lcd != nullptr, nullptr, "Invalid LCD device");
    ESP_UTILS_CHECK_FALSE_RETURN(lcd->getRefreshPanelHandle() != nullptr, nullptr, "LCD device is not initialized");

    const int physical_width = lcd->getFrameWidth();
    const int physical_height = lcd->getFrameHeight();
#if LVGL_PORT_ROTATION_DEGREE == 90 || LVGL_PORT_ROTATION_DEGREE == 270
    const int logical_width = physical_height;
    const int logical_height = physical_width;
#else
    const int logical_width = physical_width;
    const int logical_height = physical_height;
#endif

    lv_display_t *display = lv_display_create(logical_width, logical_height);
    ESP_UTILS_CHECK_NULL_RETURN(display, nullptr, "Create LVGL display failed");
    lv_display_set_user_data(display, lcd);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, flush_callback);

    const uint32_t full_buffer_bytes =
        static_cast<uint32_t>(physical_width) * physical_height * LVGL_PORT_RGB565_BYTES_PER_PIXEL;

#if LVGL_PORT_AVOID_TEAR
#if LVGL_PORT_ROTATION_DEGREE != 0
    lvgl_buf[0] = lcd->getFrameBufferByIndex(2);
    lvgl_buf[1] = nullptr;
    lv_display_set_buffers(display, lvgl_buf[0], nullptr, full_buffer_bytes, LV_DISPLAY_RENDER_MODE_FULL);
#else
#if LVGL_PORT_AVOID_TEARING_MODE == 2
    lvgl_buf[0] = lcd->getFrameBufferByIndex(1);
    lvgl_buf[1] = lcd->getFrameBufferByIndex(2);
#else
    lvgl_buf[0] = lcd->getFrameBufferByIndex(0);
    lvgl_buf[1] = lcd->getFrameBufferByIndex(1);
#endif
    ESP_UTILS_CHECK_FALSE_RETURN(lvgl_buf[0] != nullptr && lvgl_buf[1] != nullptr, nullptr,
                                 "Get LCD frame buffers failed");
#if LVGL_PORT_AVOID_TEARING_MODE == 3
    lv_display_set_buffers(display, lvgl_buf[0], lvgl_buf[1], full_buffer_bytes, LV_DISPLAY_RENDER_MODE_DIRECT);
#else
    lv_display_set_buffers(display, lvgl_buf[0], lvgl_buf[1], full_buffer_bytes, LV_DISPLAY_RENDER_MODE_FULL);
#endif
#endif
#else
    const uint32_t buffer_bytes = static_cast<uint32_t>(physical_width) * LVGL_PORT_BUFFER_SIZE_HEIGHT *
                                  LVGL_PORT_RGB565_BYTES_PER_PIXEL;
    for (int i = 0; i < LVGL_PORT_BUFFER_NUM && i < LVGL_PORT_BUFFER_NUM_MAX; ++i) {
        lvgl_buf[i] = heap_caps_malloc(buffer_bytes, LVGL_PORT_BUFFER_MALLOC_CAPS);
        ESP_UTILS_CHECK_NULL_RETURN(lvgl_buf[i], nullptr, "Allocate LVGL draw buffer failed");
    }
    lv_display_set_buffers(display, lvgl_buf[0], lvgl_buf[1], buffer_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

    if (lcd->getBasicAttributes().basic_bus_spec.x_coord_align > 1 ||
        lcd->getBasicAttributes().basic_bus_spec.y_coord_align > 1) {
        lv_display_add_event_cb(display, rounder_event_callback, LV_EVENT_INVALIDATE_AREA, lcd);
    }

    return display;
}

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    Touch *tp = static_cast<Touch *>(lv_indev_get_user_data(indev));
    data->state = LV_INDEV_STATE_RELEASED;
    if (tp == nullptr) {
        return;
    }
    if (tp->isInterruptEnabled() && xSemaphoreTake(touch_detected, 0) == pdFALSE) {
        return;
    }

    TouchPoint point;
    if (tp->readPoints(&point, 1, 0) > 0) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
}

static bool on_touch_interrupt_callback(void *user_data)
{
    (void)user_data;
    BaseType_t higher_priority_task_woken = pdFALSE;
    xSemaphoreGiveFromISR(touch_detected, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
    return false;
}

static lv_indev_t *indev_init(Touch *tp)
{
    ESP_UTILS_CHECK_FALSE_RETURN(tp != nullptr, nullptr, "Invalid touch device");
    ESP_UTILS_CHECK_FALSE_RETURN(tp->getPanelHandle() != nullptr, nullptr, "Touch device is not initialized");

    if (tp->isInterruptEnabled()) {
        touch_detected = xSemaphoreCreateBinary();
        ESP_UTILS_CHECK_NULL_RETURN(touch_detected, nullptr, "Create touch semaphore failed");
        tp->attachInterruptCallback(on_touch_interrupt_callback, tp);
    }

    lv_indev_t *indev = lv_indev_create();
    ESP_UTILS_CHECK_NULL_RETURN(indev, nullptr, "Create LVGL input device failed");
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read);
    lv_indev_set_user_data(indev, tp);
    lv_indev_set_display(indev, lvgl_display);
    return indev;
}

static void tick_increment(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_PORT_TICK_PERIOD_MS);
}

static bool tick_init(void)
{
    const esp_timer_create_args_t timer_args = {
        .callback = &tick_increment,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "LVGL tick",
        .skip_unhandled_events = true,
    };
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_timer_create(&timer_args, &lvgl_tick_timer), false, "Create LVGL tick timer failed"
    );
    ESP_UTILS_CHECK_ERROR_RETURN(
        esp_timer_start_periodic(lvgl_tick_timer, LVGL_PORT_TICK_PERIOD_MS * 1000ULL), false,
        "Start LVGL tick timer failed"
    );
    return true;
}

static bool tick_deinit(void)
{
    if (lvgl_tick_timer == nullptr) {
        return true;
    }
    ESP_UTILS_CHECK_ERROR_RETURN(esp_timer_stop(lvgl_tick_timer), false, "Stop LVGL tick timer failed");
    ESP_UTILS_CHECK_ERROR_RETURN(esp_timer_delete(lvgl_tick_timer), false, "Delete LVGL tick timer failed");
    lvgl_tick_timer = nullptr;
    return true;
}

static void lvgl_port_task(void *arg)
{
    (void)arg;
    uint32_t task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
    while (true) {
        if (lvgl_port_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            lvgl_port_unlock();
        }
        if (task_delay_ms > LVGL_PORT_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_PORT_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_PORT_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

static bool IRAM_ATTR on_lcd_vsync_callback(void *user_data)
{
    TaskHandle_t task_handle = static_cast<TaskHandle_t>(user_data);
    if (task_handle == nullptr) {
        return false;
    }
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(task_handle, &higher_priority_task_woken);
    return higher_priority_task_woken == pdTRUE;
}

static bool on_draw_bitmap_finish_callback(void *user_data)
{
    lv_display_t *display = static_cast<lv_display_t *>(user_data);
    if (display != nullptr) {
        lv_display_flush_ready(display);
    }
    return false;
}

bool lvgl_port_init(LCD *lcd, Touch *tp)
{
    ESP_UTILS_CHECK_FALSE_RETURN(lcd != nullptr, false, "Invalid LCD device");
    const auto bus_type = lcd->getBus()->getBasicAttributes().type;
#if LVGL_PORT_AVOID_TEAR
    ESP_UTILS_CHECK_FALSE_RETURN(
        bus_type == ESP_PANEL_BUS_TYPE_RGB || bus_type == ESP_PANEL_BUS_TYPE_MIPI_DSI, false,
        "Anti-tearing requires an RGB or MIPI-DSI LCD"
    );
#endif

    lv_init();
    ESP_UTILS_CHECK_FALSE_RETURN(tick_init(), false, "Initialize LVGL tick failed");

    lvgl_lcd = lcd;
    lvgl_display = display_init(lcd);
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_display, false, "Initialize LVGL display failed");

    if (bus_type != ESP_PANEL_BUS_TYPE_RGB) {
        lcd->attachDrawBitmapFinishCallback(on_draw_bitmap_finish_callback, lvgl_display);
    }

    if (tp != nullptr) {
#if LVGL_PORT_ROTATION_DEGREE != 0
        auto &transformation = tp->getTransformation();
#if LVGL_PORT_ROTATION_DEGREE == 90
        tp->swapXY(!transformation.swap_xy);
        tp->mirrorY(!transformation.mirror_y);
#elif LVGL_PORT_ROTATION_DEGREE == 180
        tp->mirrorX(!transformation.mirror_x);
        tp->mirrorY(!transformation.mirror_y);
#elif LVGL_PORT_ROTATION_DEGREE == 270
        tp->swapXY(!transformation.swap_xy);
        tp->mirrorX(!transformation.mirror_x);
#endif
#endif
        lvgl_indev = indev_init(tp);
        ESP_UTILS_CHECK_NULL_RETURN(lvgl_indev, false, "Initialize LVGL input device failed");
    }

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_mux, false, "Create LVGL mutex failed");

    const BaseType_t core_id = LVGL_PORT_TASK_CORE < 0 ? tskNO_AFFINITY : LVGL_PORT_TASK_CORE;
    const BaseType_t ret = xTaskCreatePinnedToCore(
        lvgl_port_task, "lvgl", LVGL_PORT_TASK_STACK_SIZE, nullptr, LVGL_PORT_TASK_PRIORITY,
        &lvgl_task_handle, core_id
    );
    ESP_UTILS_CHECK_FALSE_RETURN(ret == pdPASS, false, "Create LVGL task failed");

#if LVGL_PORT_AVOID_TEAR
    ESP_UTILS_CHECK_FALSE_RETURN(
        lcd->attachRefreshFinishCallback(on_lcd_vsync_callback, lvgl_task_handle), false,
        "Attach LCD VSYNC callback failed"
    );
#endif
    return true;
}

bool lvgl_port_lock(int timeout_ms)
{
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_mux, false, "LVGL mutex is not initialized");
    const TickType_t timeout_ticks = timeout_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

bool lvgl_port_unlock(void)
{
    ESP_UTILS_CHECK_NULL_RETURN(lvgl_mux, false, "LVGL mutex is not initialized");
    return xSemaphoreGiveRecursive(lvgl_mux) == pdTRUE;
}

bool lvgl_port_deinit(void)
{
    ESP_UTILS_CHECK_FALSE_RETURN(tick_deinit(), false, "Deinitialize LVGL tick failed");
    if (lvgl_task_handle != nullptr) {
        vTaskDelete(lvgl_task_handle);
        lvgl_task_handle = nullptr;
    }
    if (lvgl_indev != nullptr) {
        lv_indev_delete(lvgl_indev);
        lvgl_indev = nullptr;
    }
    if (lvgl_display != nullptr) {
        lv_display_delete(lvgl_display);
        lvgl_display = nullptr;
    }
#if !LVGL_PORT_AVOID_TEAR
    for (void *&buffer : lvgl_buf) {
        if (buffer != nullptr) {
            heap_caps_free(buffer);
            buffer = nullptr;
        }
    }
#endif
    if (touch_detected != nullptr) {
        vSemaphoreDelete(touch_detected);
        touch_detected = nullptr;
    }
    if (lvgl_mux != nullptr) {
        vSemaphoreDelete(lvgl_mux);
        lvgl_mux = nullptr;
    }
    lvgl_lcd = nullptr;
    lv_deinit();
    return true;
}
