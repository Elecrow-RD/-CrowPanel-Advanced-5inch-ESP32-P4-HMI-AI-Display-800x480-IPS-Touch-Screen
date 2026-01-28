FILE(GLOB_RECURSE main ${CMAKE_SOURCE_DIR}/main/*.c)

idf_component_register(SRCS ${main}
                        INCLUDE_DIRS "include" 
                        REQUIRES bsp_display lvgl esp_lvgl_port bsp_i2c bsp_stc8h1kxx bsp_uart elecrow_ui)
                                 