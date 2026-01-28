#ifndef _BSP_ILLUMINATE_H_
#define _BSP_ILLUMINATE_H_
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "esp_log.h"           //References for LOG Printing Function-related API Functions
#include "esp_err.h"           //References for Error Type Function-related API Functions
#include "esp_lcd_ek79007.h"   //References for lcd ek79007 Function-related API Functions
#include "esp_lcd_mipi_dsi.h"  //References for lcd mipi dsi Function-related API Functions
#include "esp_lcd_panel_ops.h" //References for lcd panel ops Function-related API Functions
#include "esp_lcd_panel_io.h"  //References for lcd panel io Function-related API Functions
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lvgl_port.h"     //References for LVGL port Function-related API Functions
#include "driver/gpio.h"       //References for GPIO Function-related API Functions
#include "driver/ledc.h"       //References for LEDC PWM Function-related API Functions
#include "lvgl.h"              //References for LVGL Function-related API Functions
#include "bsp_i2c.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define ILLUMINATE_TAG "ILLUMINATE"
#define ILLUMINATE_INFO(fmt, ...) ESP_LOGI(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)
#define ILLUMINATE_DEBUG(fmt, ...) ESP_LOGD(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)
#define ILLUMINATE_ERROR(fmt, ...) ESP_LOGE(ILLUMINATE_TAG, fmt, ##__VA_ARGS__)

#define H_size 800             // Horizontal resolution
#define V_size 480              // Vertical resolution
#define BITS_PER_PIXEL 16       // Number of image display bits of the display screen

#define LCD_GPIO_BLIGHT -1      // LCD Blight GPIO
#define BLIGHT_PWM_Hz 30000     // LCD Blight PWM GPIO

#define LV_COLOR_RED lv_color_make(0xFF, 0x00, 0x00)   // RED
#define LV_COLOR_GREEN lv_color_make(0x00, 0xFF, 0x00) // GREEN
#define LV_COLOR_BLUE lv_color_make(0x00, 0x00, 0xFF)   // BLUE
#define LV_COLOR_WHITE lv_color_make(0xFF, 0xFF, 0xFF)  // WHITE
#define LV_COLOR_BLACK lv_color_make(0x00, 0x00, 0x00)  // BLACK
#define LV_COLOR_GRAY lv_color_make(0x80, 0x80, 0x80)   // GRAY

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Refresh Rate = 18000000/(1+40+20+800)/(1+10+5+480) = 42Hz
#define RGB_LCD_PIXEL_CLOCK_HZ     (18 * 1000 * 1000)
#define RGB_LCD_H_RES              H_size
#define RGB_LCD_V_RES              V_size
#define RGB_LCD_HSYNC              4
#define RGB_LCD_HBP                8
#define RGB_LCD_HFP                8
#define RGB_LCD_VSYNC              4
#define RGB_LCD_VBP                16
#define RGB_LCD_VFP                16

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

esp_err_t display_init();                      // Display Screen Initialization Function
esp_err_t set_lcd_blight(uint32_t brightness); // Set the screen backlight

/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif
