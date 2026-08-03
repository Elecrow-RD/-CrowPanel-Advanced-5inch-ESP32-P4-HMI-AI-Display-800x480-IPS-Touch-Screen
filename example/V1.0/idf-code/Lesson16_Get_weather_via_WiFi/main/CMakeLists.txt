idf_component_register(SRCS "main.c" "ui/image_both.c"
                    REQUIRES nvs_flash esp_wifi
                             bsp_i2c bsp_illuminate bsp_wifi app_weather
                    INCLUDE_DIRS ".")
