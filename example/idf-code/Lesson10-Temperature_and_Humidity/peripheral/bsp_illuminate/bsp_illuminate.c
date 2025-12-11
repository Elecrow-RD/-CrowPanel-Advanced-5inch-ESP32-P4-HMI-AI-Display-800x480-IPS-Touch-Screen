/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_illuminate.h"  // Include BSP display header (LCD and related configurations)
#include "bsp_stc8h1kxx.h"   // Include header for STC8H1KXX microcontroller (used for PWM control)
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/

esp_lcd_panel_handle_t panel_handle = NULL;          /* Type of LCD panel handle */
static lv_display_t *my_lvgl_disp = NULL;            /* Backward compatibility with LVGL 8 */

static lv_obj_t *color_obj;                          // LVGL canvas object pointer
lv_color_t *color_buffer;                            // LVGL color buffer pointer

/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/

static esp_err_t blight_init(void)  // Initialize LCD backlight (PWM control)
{
    return ESP_OK;  // Return success (currently no specific initialization)
}

/* brightness -  (0 - 100) */
esp_err_t set_lcd_blight(uint32_t brightness)
{
    esp_err_t err = ESP_OK;                                     // Define error status variable
    stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN, brightness);          // Set the PWM duty cycle for LCD backlight brightness
    return err;                                                 // Return success
}

static esp_err_t display_port_init(void)
{
    esp_err_t err = ESP_OK;                                     // Define error status variable

    esp_lcd_rgb_panel_config_t panel_config = {                 /* RGB LCD configuration structure */
        .data_width = RGB_DATA_BUS_WIDTH,                       /* Data width: 16 bits */
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 3, 0)
        .psram_trans_align = 64,                                // Alignment for PSRAM buffer (older IDF version)
#else
        .dma_burst_size = 64,                                   /* DMA burst size alignment for PSRAM buffer */
#endif
        .num_fbs = 2,                                           /* Number of frame buffers */
#if CONFIG_RGB_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 20 * RGB_LCD_H_RES,            // Set bounce buffer size if enabled
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,                         /* Clock source for RGB LCD peripheral */
        .disp_gpio_num = RGB_PIN_NUM_DISP_EN,                   /* Display enable control pin, -1 if unused */
        .pclk_gpio_num = RGB_PIN_NUM_PCLK,                      /* PCLK signal pin */
        .vsync_gpio_num = RGB_PIN_NUM_VSYNC,                    /* VSYNC signal pin (not required in DE mode) */
        .hsync_gpio_num = RGB_PIN_NUM_HSYNC,                    /* HSYNC signal pin (not required in DE mode) */
        .de_gpio_num = RGB_PIN_NUM_DE,                          /* DE (Data Enable) signal pin */
        .data_gpio_nums = {                                     // RGB data pins
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
            .h_res = RGB_LCD_H_RES,                 /* Horizontal resolution (pixels per line) */
            .v_res = RGB_LCD_V_RES,                 /* Vertical resolution (lines per frame) */
            .hsync_back_porch = RGB_LCD_HBP,        /* Horizontal back porch (PCLK cycles between HSYNC and active data) */
            .hsync_front_porch = RGB_LCD_HFP,       /* Horizontal front porch (PCLK cycles between active data and next HSYNC) */
            .hsync_pulse_width = RGB_LCD_HSYNC,     /* HSYNC pulse width (in PCLK cycles) */
            .vsync_back_porch = RGB_LCD_VBP,        /* Vertical back porch (blank lines between VSYNC and frame start) */
            .vsync_front_porch = RGB_LCD_VFP,       /* Vertical front porch (blank lines between frame end and next VSYNC) */
            .vsync_pulse_width = RGB_LCD_VSYNC,     /* VSYNC pulse width (in lines) */
            .flags = {
                .hsync_idle_low = false,            /*!< HSYNC signal is low in IDLE state */
                .vsync_idle_low = false,            /*!< VSYNC signal is low in IDLE state */
                .de_idle_high = false,              /*!< DE signal is high in IDLE state */
                .pclk_active_neg = true,            /* RGB data is sampled on falling edge of PCLK */
                .pclk_idle_high = true,             /* PCLK remains high during idle periods */
            },
        },
        .flags.fb_in_psram = true,                  /* Allocate frame buffer in PSRAM */
    };
    err = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);  /* Create a new RGB panel instance */
    if (err != ESP_OK)
        return err;                                 // Return error if panel creation fails
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle)); /* Reset the RGB panel */
    
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));  /* Initialize the RGB panel */

    return err;                                     // Return success
}

static esp_err_t lvgl_init()
{
    esp_err_t err = ESP_OK;                                        // Define error status variable
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = configMAX_PRIORITIES - 4,                 /* LVGL task priority */
        .task_stack = 8192*2,                                      /* LVGL task stack size */
        .task_affinity = -1,                                       /* No CPU core affinity */
        .task_max_sleep_ms = 10,                                   /* Max sleep time for LVGL task */
        .timer_period_ms = 5,                                      /* LVGL timer tick period (ms) */
    };
    err = lvgl_port_init(&lvgl_cfg);                               // Initialize LVGL port
    if (err != ESP_OK)
    {
        ILLUMINATE_ERROR("LVGL port initialization failed");        // Log error if LVGL init fails
    }

    const lvgl_port_display_cfg_t disp_cfg = {
        // .io_handle = mipi_dbi_io,
        .panel_handle = panel_handle,                              // Bind to the created LCD panel
        .control_handle = panel_handle,                            // Control handle (same as panel)
        .buffer_size = (H_size * V_size),                          // LVGL buffer size
        .double_buffer = false,                                    // Disable double buffering
        .hres = H_size,                                            // Horizontal resolution
        .vres = V_size,                                            // Vertical resolution
        .monochrome = false,                                       // Color display
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,                    // Color format for LVGL 9
#endif
        .rotation = {
            .swap_xy = false,                                      // No XY swap
            .mirror_x = false,                                     // No X mirroring
            .mirror_y = false,                                     // No Y mirroring
        },
        .flags = {
            .buff_dma = false,                                     // Disable DMA buffer
            .buff_spiram = true,                                   // Allocate buffer in PSRAM
            .sw_rotate = false,                                    // Disable software rotation
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,                                    // Swap byte order for RGB565
#endif
#if CONFIG_DISPLAY_LVGL_FULL_REFRESH
            .full_refresh = true,                                  // Enable full screen refresh
#else
            .full_refresh = false,                                 // Disable full screen refresh
#endif
#if CONFIG_DISPLAY_LVGL_DIRECT_MODE
            .direct_mode = true,                                   // Enable direct rendering mode
#else
            .direct_mode = true,                                   // Direct rendering mode (default)
#endif
        },
    };
    const lvgl_port_display_rgb_cfg_t lvgl_rgb_cfg = {
        .flags = {
#if CONFIG_DISPLAY_LVGL_AVOID_TEAR
            .avoid_tearing = true,                                 // Enable tearing avoidance
#else
            .avoid_tearing = true,                                 // Enable tearing avoidance (default)
#endif
        },
    };
    my_lvgl_disp = lvgl_port_add_disp_rgb(&disp_cfg, &lvgl_rgb_cfg); // Add LVGL RGB display
    if (my_lvgl_disp == NULL)
    {
        err = ESP_FAIL;                                            // Set error if display creation fails
        ILLUMINATE_ERROR("LVGL rgb port add fail");                // Log error
    }

    return err;                                                    // Return success or failure
}

void fill_screen_with_color(lv_color_t color)
{
    lv_canvas_fill_bg(color_obj, color, LV_OPA_COVER);             // Fill the LVGL canvas background with a solid color
}

esp_err_t display_init()
{
    esp_err_t err = ESP_OK;                                        // Define error status variable
    err = blight_init();                                           // Initialize backlight
    if (err != ESP_OK)
        return err;
    err = display_port_init();                                     // Initialize display port (RGB LCD)
    if (err != ESP_OK)
        return err;
    err = lvgl_init();                                             // Initialize LVGL graphics library
    if (err != ESP_OK)
    {
        ILLUMINATE_ERROR("Display init fail");                     // Log error if display initialization fails
        return err;
    }

    return err;                                                    // Return success
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
