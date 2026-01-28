# set(ELECROW_DIR ./elecrow_ui/images)
file(GLOB_RECURSE ELECROW_C_SRCS ./images/*.c)

idf_component_register(
    SRCS "elecrow_ui.c" ${ELECROW_C_SRCS} #"./elecrow_ui/images/*.c" 
    INCLUDE_DIRS "include"
    # PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES driver lvgl
    # PRIV_REQUIRES esp_lcd usb spiffs fatfs
)
