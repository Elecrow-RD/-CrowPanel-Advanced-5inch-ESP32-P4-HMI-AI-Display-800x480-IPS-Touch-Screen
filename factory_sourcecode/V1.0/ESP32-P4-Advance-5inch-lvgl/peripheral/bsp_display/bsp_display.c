/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_display.h"
#include "bsp_stc8h1kxx.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#ifdef CONFIG_BSP_DISPLAY_ENABLED
esp_lcd_panel_handle_t panel_handle = NULL;
static lv_display_t *my_lvgl_disp = NULL;
static lv_obj_t *color_obj;
lv_color_t *color_buffer;
void *display_buffer = NULL;
#ifdef CONFIG_BSP_TOUCH_ENABLED
static esp_lcd_touch_handle_t tp = NULL;
static esp_lcd_panel_io_handle_t tp_io_handle = NULL;
static lv_indev_t *my_touch_indev;
#endif
#endif

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
#ifdef CONFIG_BSP_DISPLAY_ENABLED

#ifdef CONFIG_BSP_TOUCH_ENABLED

esp_err_t touch_init(void)
{
    esp_err_t err = ESP_OK;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 16,
        .flags =
            {
                .disable_control_phase = 1,
            },
        .scl_speed_hz = 400000,
    };
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = H_size,
        .y_max = V_size,
        .rst_gpio_num = TOUCH_GPIO_RST,
        .int_gpio_num = TOUCH_GPIO_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
    };
    err = esp_lcd_new_panel_io_i2c((i2c_master_bus_handle_t)i2c_bus_handle, &io_config, &tp_io_handle);
    if (err != ESP_OK)
        return err;
    tp_cfg.driver_data = (void*)&io_config.dev_addr;
    err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);
    return err;
}

static uint16_t touch_x = 0xffff;
static uint16_t touch_y = 0xffff;
static bool is_pressed = false; 

static void set_coor(uint16_t x, uint16_t y, bool press)
{
    touch_x = x;
    touch_y = y;
    is_pressed = press;
}

void get_coor(uint16_t* x, uint16_t* y, bool* press)
{
    *x = touch_x;
    *y = touch_y;
    *press = is_pressed;
}

esp_err_t touch_read(void)
{
    esp_err_t err = ESP_OK;
    uint16_t touch_x[1];
    uint16_t touch_y[1];
    uint16_t touch_strength[1];
    uint8_t touch_cnt = 0;
    err = esp_lcd_touch_read_data(tp);
    if (err != ESP_OK)
    {
        DISPLAY_INFO("GT911 read error");
        return err;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
    if (esp_lcd_touch_get_coordinates(tp, touch_x, touch_y, touch_strength, &touch_cnt, 1)) {
        DISPLAY_INFO("X=%hu Y=%hu strenth=%hu cnt=%d", touch_x[0], touch_y[0], touch_strength[0], touch_cnt);
        set_coor(touch_x[0], touch_y[0], true);
    }
    else {
        set_coor(0xffff, 0xffff, false);
    }
    return err;
}

#endif

static esp_err_t blight_init(void)
{
    return ESP_OK;
}

esp_err_t set_lcd_blight(uint32_t brightness)
{
    esp_err_t err = ESP_OK;
    stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN, brightness);
    return err;
}

static esp_err_t display_port_init(void)
{
    esp_err_t err = ESP_OK;

    esp_lcd_rgb_panel_config_t panel_config = { /* RGB LCD configuration structure */
        .data_width = RGB_DATA_BUS_WIDTH,       /* Data width is 16 bits */
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
        .psram_trans_align = 64,
#else
        .dma_burst_size = 64,                   /* Alignment of buffers allocated in PSRAM */
#endif
        .num_fbs = 2,                           /* Number of frame buffers */
#if CONFIG_RGB_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 20 * RGB_LCD_H_RES,
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,         /* RGB LCD peripheral clock source */
        .disp_gpio_num = RGB_PIN_NUM_DISP_EN,   /* Used for display control signal, set to -1 if not used */
        .pclk_gpio_num = RGB_PIN_NUM_PCLK,      /* PCLK signal pin */
        .vsync_gpio_num = RGB_PIN_NUM_VSYNC,    /* VSYNC signal pin, optional in DE mode */
        .hsync_gpio_num = RGB_PIN_NUM_HSYNC,    /* HSYNC signal pin, optional in DE mode */
        .de_gpio_num = RGB_PIN_NUM_DE,          /* DE signal pin */
        .data_gpio_nums = {
            RGB_PIN_NUM_DATA0,
            RGB_PIN_NUM_DATA1,
            RGB_PIN_NUM_DATA2,
            RGB_PIN_NUM_DATA3,
            RGB_PIN_NUM_DATA4,
            RGB_PIN_NUM_DATA5,
            RGB_PIN_NUM_DATA6,
            RGB_PIN_NUM_DATA7,
            RGB_PIN_NUM_DATA8,
            RGB_PIN_NUM_DATA9,
            RGB_PIN_NUM_DATA10,
            RGB_PIN_NUM_DATA11,
            RGB_PIN_NUM_DATA12,
            RGB_PIN_NUM_DATA13,
            RGB_PIN_NUM_DATA14,
            RGB_PIN_NUM_DATA15,
#if CONFIG_RGB_LCD_DATA_LINES > 16
            RGB_PIN_NUM_DATA16,
            RGB_PIN_NUM_DATA17,
            RGB_PIN_NUM_DATA18,
            RGB_PIN_NUM_DATA19,
            RGB_PIN_NUM_DATA20,
            RGB_PIN_NUM_DATA21,
            RGB_PIN_NUM_DATA22,
            RGB_PIN_NUM_DATA23
#endif  
        },                                          
        .timings = {                                /* RGB LCD timing parameters */
            .pclk_hz = RGB_LCD_PIXEL_CLOCK_HZ,      /* Pixel clock frequency */
            .h_res = RGB_LCD_H_RES,                 /* Horizontal resolution, number of pixels per line */
            .v_res = RGB_LCD_V_RES,                 /* Vertical resolution, number of lines per frame */
            .hsync_back_porch = RGB_LCD_HBP,        /* Horizontal back porch, PCLK cycles between HSYNC and start of active data */
            .hsync_front_porch = RGB_LCD_HFP,       /* Horizontal front porch, PCLK cycles between end of active data and next HSYNC */
            .hsync_pulse_width = RGB_LCD_HSYNC,     /* HSYNC pulse width, unit: PCLK cycles */
            .vsync_back_porch = RGB_LCD_VBP,        /* Vertical back porch, number of invalid lines between VSYNC and start of frame */
            .vsync_front_porch = RGB_LCD_VFP,       /* Vertical front porch, number of invalid lines between end of frame and next VSYNC */
            .vsync_pulse_width = RGB_LCD_VSYNC,     /* VSYNC pulse width, unit: number of lines */
            .flags = {
                .hsync_idle_low = false,            /*!< The hsync signal is low in IDLE state */
                .vsync_idle_low = false,            /*!< The vsync signal is low in IDLE state */
                .de_idle_high = false,              /*!< The de signal is high in IDLE state */
                .pclk_active_neg = true,            /* RGB data is clocked on falling edge */
                .pclk_idle_high = true,             /* PCLK stays high during idle phase */
            },
        },
        .flags.fb_in_psram = true,  /* Allocate frame buffer in PSRAM */
    };
    err = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);  /* Create RGB object */
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle)); /* Reset RGB panel */
    
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));  /* Initialize RGB panel */

    return err;
}

static void display_port_deinit(void)
{
    if (esp_lcd_panel_del(panel_handle) != ESP_OK)
        DISPLAY_ERROR("deinit panel_handle error");

    panel_handle = NULL;

    set_lcd_blight(0);
}

static esp_err_t lvgl_init()
{
    esp_err_t err = ESP_OK;
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = configMAX_PRIORITIES - 4, /* LVGL task priority */
        .task_stack = 8192*2,                        /* LVGL task stack size */
        .task_affinity = -1,                       /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 10,                   /* Maximum sleep in LVGL task */
        .timer_period_ms = 5,                      /* LVGL timer tick period in ms */
    };
    err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK)
    {
        DISPLAY_ERROR("LVGL port initialization failed");
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        // .io_handle = mipi_dbi_io,
        .panel_handle = panel_handle,
        .control_handle = panel_handle,
        .buffer_size = (H_size * V_size),
        .double_buffer = false,
        .hres = H_size,
        .vres = V_size,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .buff_spiram = true,
            .sw_rotate = false,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
#if CONFIG_DISPLAY_LVGL_FULL_REFRESH
            .full_refresh = true,
#else
            .full_refresh = false,
#endif
#if CONFIG_DISPLAY_LVGL_DIRECT_MODE
            .direct_mode = true,
#else
            .direct_mode = false,
#endif
        },
    };
    const lvgl_port_display_rgb_cfg_t lvgl_rgb_cfg = {
        .flags = {
#if CONFIG_DISPLAY_LVGL_AVOID_TEAR
            .avoid_tearing = true,
#else
            .avoid_tearing = false,
#endif
        },
    };
    my_lvgl_disp = lvgl_port_add_disp_rgb(&disp_cfg, &lvgl_rgb_cfg);
    if (my_lvgl_disp == NULL)
    {
        err = ESP_FAIL;
        DISPLAY_ERROR("LVGL rgb port add fail");
    }
#ifdef CONFIG_BSP_TOUCH_ENABLED
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = my_lvgl_disp,
        .handle = tp,
    };
    my_touch_indev = lvgl_port_add_touch(&touch_cfg);
    if (my_touch_indev == NULL)
    {
        err = ESP_FAIL;
        DISPLAY_ERROR("LVGL touch port add fail");
    }
#endif
    return err;
}

void fill_screen_with_color(lv_color_t color)
{
    lv_canvas_fill_bg(color_obj, color, LV_OPA_COVER);
}

void set_canvas_display(bool state)
{
    if (state)
    {
        if (color_obj != NULL)
            lv_obj_clear_flag(color_obj, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        if (color_obj != NULL)
            lv_obj_add_flag(color_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

esp_err_t display_init()
{
    esp_err_t err = ESP_OK;
    err = blight_init();
    if (err != ESP_OK)
        return err;
    err = display_port_init();
    if (err != ESP_OK)
        return err;
    err = lvgl_init();
    if (err != ESP_OK)
    {
        DISPLAY_ERROR("Display init fail");
        return err;
    }
    return err;
}

esp_err_t get_display_buff(void **buff)
{
    esp_err_t err = ESP_OK;
    err = esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 1, buff);
    return err;
}

#endif
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/