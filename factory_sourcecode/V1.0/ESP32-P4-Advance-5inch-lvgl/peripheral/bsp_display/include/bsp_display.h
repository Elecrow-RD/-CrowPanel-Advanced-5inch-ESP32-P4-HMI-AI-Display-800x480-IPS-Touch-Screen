#ifndef _BSP_DISPLAY_H_
#define _BSP_DISPLAY_H_
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/clk_tree_defs.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lvgl_port.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "lvgl.h"
#include "bsp_i2c.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define DISPLAY_TAG "DISPLAY"
#define DISPLAY_INFO(fmt, ...) ESP_LOGI(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_DEBUG(fmt, ...) ESP_LOGD(DISPLAY_TAG, fmt, ##__VA_ARGS__)
#define DISPLAY_ERROR(fmt, ...) ESP_LOGE(DISPLAY_TAG, fmt, ##__VA_ARGS__)

#ifdef CONFIG_BSP_DISPLAY_ENABLED

#define H_size CONFIG_H_SIZE
#define V_size CONFIG_V_SIZE
#define BITS_PER_PIXEL CONFIG_BITS_PER_PIXEL

#define LCD_GPIO_BLIGHT CONFIG_LCD_GPIO_BLIGHT
#define BLIGHT_PWM_Hz CONFIG_BLIGHT_PWM_Hz

#define LV_COLOR_RED lv_color_make(0xFF, 0x00, 0x00)
#define LV_COLOR_GREEN lv_color_make(0x00, 0xFF, 0x00)
#define LV_COLOR_BLUE lv_color_make(0x00, 0x00, 0xFF)
#define LV_COLOR_WHITE lv_color_make(0xFF, 0xFF, 0xFF)
#define LV_COLOR_BLACK lv_color_make(0x00, 0x00, 0x00)
#define LV_COLOR_GRAY lv_color_make(0x80, 0x80, 0x80)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Refresh Rate = 18000000/(1+40+20+800)/(1+10+5+480) = 42Hz
#define RGB_LCD_PIXEL_CLOCK_HZ     (25 * 1000 * 1000)
#define RGB_LCD_H_RES              H_size
#define RGB_LCD_V_RES              V_size
#define RGB_LCD_HSYNC              4
#define RGB_LCD_HBP                8
#define RGB_LCD_HFP                8
#define RGB_LCD_VSYNC              4
#define RGB_LCD_VBP                16
#define RGB_LCD_VFP                16

#define RGB_LCD_BK_LIGHT_ON_LEVEL  1
#define RGB_LCD_BK_LIGHT_OFF_LEVEL !RGB_LCD_BK_LIGHT_ON_LEVEL
#define RGB_PIN_NUM_BK_LIGHT       -1
#define RGB_PIN_NUM_DISP_EN        -1

#define RGB_PIN_NUM_HSYNC          40
#define RGB_PIN_NUM_VSYNC          41
#define RGB_PIN_NUM_DE             2
#define RGB_PIN_NUM_PCLK           3

#define RGB_PIN_NUM_DATA0          8
#define RGB_PIN_NUM_DATA1          7
#define RGB_PIN_NUM_DATA2          6
#define RGB_PIN_NUM_DATA3          5
#define RGB_PIN_NUM_DATA4          4

#define RGB_PIN_NUM_DATA5          14
#define RGB_PIN_NUM_DATA6          13
#define RGB_PIN_NUM_DATA7          12
#define RGB_PIN_NUM_DATA8          11
#define RGB_PIN_NUM_DATA9          10
#define RGB_PIN_NUM_DATA10         9

#define RGB_PIN_NUM_DATA11         19
#define RGB_PIN_NUM_DATA12         18
#define RGB_PIN_NUM_DATA13         17
#define RGB_PIN_NUM_DATA14         16
#define RGB_PIN_NUM_DATA15         15
#if BITS_PER_PIXEL > 16
#define RGB_PIN_NUM_DATA16         CONFIG_RGB_LCD_DATA16_GPIO
#define RGB_PIN_NUM_DATA17         CONFIG_RGB_LCD_DATA17_GPIO
#define RGB_PIN_NUM_DATA18         CONFIG_RGB_LCD_DATA18_GPIO
#define RGB_PIN_NUM_DATA19         CONFIG_RGB_LCD_DATA19_GPIO
#define RGB_PIN_NUM_DATA20         CONFIG_RGB_LCD_DATA20_GPIO
#define RGB_PIN_NUM_DATA21         CONFIG_RGB_LCD_DATA21_GPIO
#define RGB_PIN_NUM_DATA22         CONFIG_RGB_LCD_DATA22_GPIO
#define RGB_PIN_NUM_DATA23         CONFIG_RGB_LCD_DATA23_GPIO
#endif

#if (16 == BITS_PER_PIXEL)
#define RGB_DATA_BUS_WIDTH         16
#define RGB_PIXEL_SIZE             2
#define RGB_LV_COLOR_FORMAT        LV_COLOR_FORMAT_RGB565
#elif (24 == BITS_PER_PIXEL)
#define RGB_DATA_BUS_WIDTH         24
#define RGB_PIXEL_SIZE             3
#define RGB_LV_COLOR_FORMAT        LV_COLOR_FORMAT_RGB888
#endif

#ifdef CONFIG_BSP_TOUCH_ENABLED

#define TOUCH_GPIO_RST CONFIG_TOUCH_GPIO_RST
#define TOUCH_GPIO_INT CONFIG_TOUCH_GPIO_INT

esp_err_t touch_read(void);
void set_touch_display(bool state);
void get_coor(uint16_t* x, uint16_t* y, bool* press);
#endif

esp_err_t display_init();
esp_err_t touch_init(void);
esp_err_t get_display_buff(void **buff);
esp_err_t set_lcd_blight(uint32_t brightness);
void fill_screen_with_color(lv_color_t color);
void set_canvas_display(bool state);
#endif
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif
