/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "main.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
TaskHandle_t pressure;
TaskHandle_t pressure_extra_gpio;
TaskHandle_t pressure_extra_uart;
static esp_ldo_channel_handle_t ldo4 = NULL;
static esp_ldo_channel_handle_t ldo3 = NULL;
// adc_oneshot_unit_handle_t adc_handle = NULL;
static work_message work_status;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
void init_fail(const char *name, esp_err_t err)
{
    static bool state = false;
    while (1)
    {
        if (!state)
        {
            MAIN_ERROR("%s init  [ %s ]", name, esp_err_to_name(err));
            state = true;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void Init(void)
{
    static esp_err_t err = ESP_OK;
    esp_ldo_channel_config_t ldo3_cof = {
        .chan_id = 3,
        .voltage_mv = 2500,
    };
    err = esp_ldo_acquire_channel(&ldo3_cof, &ldo3);
    if (err != ESP_OK)
        init_fail("ldo3", err);
    esp_ldo_channel_config_t ldo4_cof = {
        .chan_id = 4,
        .voltage_mv = 3300,
    };
    err = esp_ldo_acquire_channel(&ldo4_cof, &ldo4);
    if (err != ESP_OK)
        init_fail("ldo4", err);
    err = gpio_install_isr_service(0);
    if (err != ESP_OK)
        init_fail("gpio isr service", err);

    work_status.work_flag = false;
    work_status.audio_flag = false;
    work_status.camera_flag = false;
    work_status.display_flag = false;
    work_status.blight_flag = false;
    work_status.lora_flag = false;
    work_status.nrf24_flag = false;
    work_status.wifi_flag = false;
    work_status.bluetooth_flag = false;
    work_status.usb_flag = false;

#ifdef CONFIG_BSP_I2C_ENABLED
    err = i2c_init();
    if (err != ESP_OK)
        init_fail("i2c", err);
    vTaskDelay(200 / portTICK_PERIOD_MS);
#endif

#ifdef CONFIG_BSP_STC8H1KXX_ENABLED
    err = stc8_i2c_init();
    if (err != ESP_OK)
        init_fail("stc8h1kxx", err);
#endif

#ifdef CONFIG_BSP_DISPLAY_ENABLED
#ifdef CONFIG_BSP_TOUCH_ENABLED
    err = touch_init();
    if (err != ESP_OK)
        init_fail("display touch", err);
#endif
    err = display_init();
    if (err != ESP_OK)
        init_fail("display", err);
#endif
}

// 画布尺寸（与屏幕一致，避免滚动）
#define CANVAS_W    H_size
#define CANVAS_H    V_size

// 点大小
#define DOT_SIZE    3

static lv_obj_t *canvas;
static lv_color_t *canvas_buf;

static void draw_dot(int32_t x, int32_t y) {
    if(x < 0 || x >= CANVAS_W || y < 0 || y >= CANVAS_H) {
        return;
    }

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = LV_RADIUS_CIRCLE;
    rect_dsc.bg_color = lv_color_hex(0xFF0000); // 红色点（醒目）
    rect_dsc.bg_opa = LV_OPA_COVER;

    lv_coord_t x1 = x - DOT_SIZE/2;
    lv_coord_t y1 = y - DOT_SIZE/2;
    lv_coord_t w = DOT_SIZE;
    lv_coord_t h = DOT_SIZE;

    lv_canvas_draw_rect(canvas, x1, y1, w, h, &rect_dsc);
}

static void canvas_event_handler(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    ESP_LOGI("EVENT", "code = %d", code);
    if(code == LV_EVENT_PRESSED) ESP_LOGI("EVENT", "触发按下事件");
    if(code == LV_EVENT_PRESSING) ESP_LOGI("EVENT", "触发触摸移动事件");

    lv_indev_t *indev = lv_indev_get_act();
    if(!indev) {
        ESP_LOGW("EVENT", "无输入设备");

        return;
    }

    lv_point_t touch_pos;
    lv_indev_get_point(indev, &touch_pos);

    // 转换为画布坐标（画布左上角对齐屏幕，x=0,y=0）
    lv_coord_t canvas_x = touch_pos.x - lv_obj_get_x(obj);
    lv_coord_t canvas_y = touch_pos.y - lv_obj_get_y(obj);

    if(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        draw_dot(canvas_x, canvas_y);
    }
}

void create_simple_drawing(void) {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), 0);

    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE); // 禁止屏幕滚动
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // 分配画布缓冲区
    canvas_buf = heap_caps_malloc(CANVAS_W * CANVAS_H * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(canvas_buf == NULL) {
        ESP_LOGE("BUFFER", "SPIRAM分配失败，尝试内部RAM");
        canvas_buf = heap_caps_malloc(CANVAS_W * CANVAS_H * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    if(canvas_buf == NULL) {
        ESP_LOGE("BUFFER", "画布缓冲区分配失败！");
        return;
    }

    // 创建画布
    canvas = lv_canvas_create(scr);
    lv_canvas_set_buffer(canvas, canvas_buf, CANVAS_W, CANVAS_H, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(canvas, lv_color_hex(0xFFFFFF), LV_OPA_TRANSP);

    // 画布位置（左上角对齐，无偏移）
    lv_obj_set_pos(canvas, 0, 0);

    // 修正2：禁用画布滚动
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE); // 禁止屏幕滚动
    lv_obj_set_scrollbar_mode(canvas, LV_SCROLLBAR_MODE_OFF);

    // 画布样式
    lv_obj_set_style_border_width(canvas, 2, 0);
    lv_obj_set_style_border_color(canvas, lv_color_hex(0x0000FF), 0); // 蓝色边框

    // 绑定事件
    lv_obj_add_event_cb(canvas, canvas_event_handler, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(canvas, canvas_event_handler, LV_EVENT_PRESSING, NULL);
    
    // 修正3：允许点击（LVGL 8用lv_obj_set_clickable）
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE); // 显式允许点击

    ESP_LOGI("INIT", "画点界面初始化完成");
}


void app_main(void)
{
    MAIN_INFO("----------Demo version----------\r\n");
    // // 运行时屏蔽所有日志输出
    // esp_log_level_set("*", ESP_LOG_WARN);  // "*" 表示匹配所有标签

    Init();

    lvgl_port_lock(0);
    elecrow_screen();
    lvgl_port_unlock();

    set_lcd_blight(100);

    while (!elecrow_success)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    lvgl_port_lock(0);
    
    // create_simple_drawing();
    lv_demo_widgets();

    lvgl_port_unlock();

    // while (1)
    // {
    //     // 堆内存信息
    //     size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    //     size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    //     size_t used_heap = total_heap - free_heap;
        
    //     // DMA内存信息
    //     size_t dma_total = heap_caps_get_total_size(MALLOC_CAP_DMA);
    //     size_t dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA);
    //     size_t dma_used = dma_total - dma_free;
        
    //     // 最大空闲块
    //     size_t max_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        
    //     printf("==================== 内存信息 ====================\n");
    //     printf("堆内存: 总大小=%d KB, 已使用=%d KB, 空闲=%d KB\n",
    //             total_heap / 1024, used_heap / 1024, free_heap / 1024);
    //     printf("DMA内存: 总大小=%d KB, 已使用=%d KB, 空闲=%d KB\n",
    //             dma_total / 1024, dma_used / 1024, dma_free / 1024);
    //     printf("最大连续空闲块: %d KB\n", max_free_block / 1024);
    //     printf("==================================================\n");
    //     vTaskDelay(2000 / portTICK_PERIOD_MS);
    // }
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
