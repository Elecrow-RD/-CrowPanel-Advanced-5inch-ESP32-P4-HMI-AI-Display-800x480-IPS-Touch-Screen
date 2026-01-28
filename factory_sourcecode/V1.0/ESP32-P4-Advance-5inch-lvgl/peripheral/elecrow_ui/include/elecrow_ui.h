#ifndef _ELECROW_UI_H
#define _ELECROW_UI_H
#include "lvgl.h"

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/

/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#ifdef __cplusplus
extern "C"
{
#endif
    LV_IMG_DECLARE(starting_up_background);
    LV_IMG_DECLARE(starting_up_logo);

    LV_IMG_DECLARE(loading_background_inch5);
    LV_IMG_DECLARE(loading_bar_0);
    LV_IMG_DECLARE(loading_bar_100);

    LV_IMG_DECLARE(icon_click_clicked);
    LV_IMG_DECLARE(icon_click_default);
    LV_IMG_DECLARE(icon_home_clicked);
    LV_IMG_DECLARE(icon_home_default);
    LV_IMG_DECLARE(icon_return_clicked);
    LV_IMG_DECLARE(icon_return_default);

    void elecrow_screen(void);
    extern bool elecrow_success;
#ifdef __cplusplus
} /*extern "C"*/
#endif
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/

#endif