FILE(GLOB_RECURSE component_sources "*.c")

idf_component_register(SRCS ${component_sources}
                        INCLUDE_DIRS "include"
                        REQUIRES driver esp_lcd_ek79007 lvgl esp_lvgl_port bsp_i2c bsp_stc8h1kxx esp_lcd_touch_gt911 bsp_display)
                        