/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "include/elecrow_ui.h"
#include "stdio.h"
#include "esp_log.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define TAG "elecrow_ui"

static lv_obj_t *Elecrow_logo_screen;
static lv_obj_t *Elecrow_P_bar_screen;
static lv_obj_t *Elecrow_touch_screen;
static lv_obj_t *Bp_bg_img;
static lv_obj_t *Bp_logo_img;
static lv_obj_t *Load_bg_img;
static lv_obj_t *Load_frame_img;
static lv_obj_t *Load_label;
static lv_obj_t *Progress_bar_frame;
static lv_obj_t *Progress_bar_img;
static lv_obj_t *ui_menu;
static lv_obj_t *ui_enter_touch;
static lv_obj_t *ui_exit_touch;
static lv_obj_t *ui_touch_xy;
lv_timer_t *move_down_logo_timer;
lv_timer_t *loading_bar_timer;
bool elecrow_success = false;
bool enter_touch_flag = false;
static int progress = 0;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
static void loading_bar_time_cb(lv_timer_t *timer)
{
    char str[20];
    progress += 1;

    if (true == enter_touch_flag)
    {
        progress = 0;
        return;
    }
    if (progress > 100)
        progress = 100;
    lv_coord_t new_width = 0 + ((loading_bar_100.header.w * progress) / 100);
    snprintf(str, sizeof(str), "%3d%%", progress);
    lv_label_set_text(Load_label, str);
    lv_obj_set_size(Progress_bar_img, new_width, loading_bar_100.header.h);
    lv_obj_invalidate(Progress_bar_img);
    if (progress >= 100)
    {
        lv_timer_del(timer);
        elecrow_success = true;
    }
}

static void switch_to_loading_page_timer_cb(lv_timer_t *timer)
{
    lv_scr_load_anim(Elecrow_P_bar_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    loading_bar_timer = lv_timer_create(loading_bar_time_cb, 50, NULL);
}

static void move_down_logo_timer_cb(lv_timer_t *timer)
{
    static lv_coord_t new_y = -50;
    new_y += 3;

    if (new_y > 150) {
        new_y = 150;
    }
    lv_obj_set_y(Bp_logo_img, new_y);
    lv_obj_invalidate(Bp_logo_img);
    if (new_y >= 150)
    {
        lv_timer_del(timer);
        lv_timer_t *switch_timer = lv_timer_create(switch_to_loading_page_timer_cb, 1000, NULL);
        lv_timer_set_repeat_count(switch_timer, 1);
    }
}

void ui_enter_touch_event_cb(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_PRESSED) {
        lv_img_set_src(ui_enter_touch, &icon_click_clicked);
    }
    if(event_code == LV_EVENT_RELEASED) {
        lv_img_set_src(ui_enter_touch, &icon_click_default);
    }
    if(event_code == LV_EVENT_CLICKED) {
        enter_touch_flag = true;
        lv_scr_load_anim(Elecrow_touch_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }
}

void ui_menu_event_cb(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_PRESSED) {
        lv_img_set_src(ui_menu, &icon_home_clicked);
    }
    if(event_code == LV_EVENT_RELEASED) {
        lv_img_set_src(ui_menu, &icon_home_default);
    }
    if(event_code == LV_EVENT_CLICKED) {
        lv_timer_del(loading_bar_timer);
        // lv_obj_del(Elecrow_P_bar_screen);
        elecrow_success = true;
    }
}

void ui_exit_touch_event_cb(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_PRESSED) {
        lv_img_set_src(ui_exit_touch, &icon_return_clicked);
    }
    if(event_code == LV_EVENT_RELEASED) {
        lv_img_set_src(ui_exit_touch, &icon_return_default);
    }
    if(event_code == LV_EVENT_CLICKED) {
        lv_scr_load_anim(Elecrow_P_bar_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
        enter_touch_flag = false;
    }
}

static void ui_touch_xy_event_cb(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_PRESSING) {
        lv_point_t point;
        lv_indev_get_point(lv_indev_get_act(), &point);
        
        char str[64];
        snprintf(str, sizeof(str), "Touch Adjust: %4d %4d", point.x, point.y);
        ESP_LOGI(TAG, "%s", str);
        lv_label_set_text(ui_touch_xy, str);
    }
}

void elecrow_start_screen(void)
{
    Elecrow_logo_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(Elecrow_logo_screen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(Elecrow_logo_screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    Bp_bg_img = lv_img_create(Elecrow_logo_screen);
    lv_img_set_src(Bp_bg_img, &starting_up_background);
    lv_obj_align(Bp_bg_img, LV_ALIGN_CENTER, 0, 0);

    Bp_logo_img = lv_img_create(Bp_bg_img);
    lv_img_set_src(Bp_logo_img, &starting_up_logo);
    lv_obj_align(Bp_logo_img, LV_ALIGN_TOP_MID, 0, -50);

    move_down_logo_timer = lv_timer_create(move_down_logo_timer_cb, 30, NULL);
}

void elecrow_loading_screen(void)
{
    Elecrow_P_bar_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(Elecrow_P_bar_screen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(Elecrow_P_bar_screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    Load_bg_img = lv_img_create(Elecrow_P_bar_screen);
    lv_img_set_src(Load_bg_img, &loading_background_inch5);
    lv_obj_align(Load_bg_img, LV_ALIGN_CENTER, 0, 0);
    
    Load_frame_img = lv_img_create(Load_bg_img);
    lv_img_set_src(Load_frame_img, &loading_bar_0);
    lv_obj_align(Load_frame_img, LV_ALIGN_BOTTOM_MID, 0, -30);

    Progress_bar_img = lv_img_create(Load_frame_img);
    lv_img_set_src(Progress_bar_img, &loading_bar_100);
    lv_obj_align(Progress_bar_img, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_size(Progress_bar_img, 0, loading_bar_100.header.h);

    ui_enter_touch = lv_img_create(Load_bg_img);
    lv_img_set_src(ui_enter_touch, &icon_click_default);
    lv_obj_set_pos(ui_enter_touch, 40, -40);
    lv_obj_set_align(ui_enter_touch, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_add_flag(ui_enter_touch, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_enter_touch, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_add_event_cb(ui_enter_touch, ui_enter_touch_event_cb, LV_EVENT_ALL, NULL);

    ui_menu = lv_img_create(Load_bg_img);
    lv_img_set_src(ui_menu, &icon_home_default);
    lv_obj_set_pos(ui_menu, -40, -40);
    lv_obj_set_align(ui_menu, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_add_flag(ui_menu, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_menu, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_add_event_cb(ui_menu, ui_menu_event_cb, LV_EVENT_ALL, NULL);

    Load_label = lv_label_create(Load_bg_img);
    lv_label_set_text(Load_label, "  0%%");
    lv_obj_set_style_text_color(Load_label, lv_color_make(0x00, 0xA0, 0xF0), LV_PART_MAIN);
    lv_obj_set_style_text_font(Load_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Load_label, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_align_to(Load_label, Load_frame_img, LV_ALIGN_OUT_TOP_MID, 0, 0);
}

void elecrow_screen_init(void)
{
    Elecrow_touch_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(Elecrow_touch_screen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(Elecrow_touch_screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(Elecrow_touch_screen, ui_touch_xy_event_cb, LV_EVENT_PRESSING, NULL);

    ui_exit_touch = lv_img_create(Elecrow_touch_screen);
    lv_img_set_src(ui_exit_touch, &icon_return_default);
    lv_obj_set_pos(ui_exit_touch, 100, -150);
    lv_obj_set_align(ui_exit_touch, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_exit_touch, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_exit_touch, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_add_event_cb(ui_exit_touch, ui_exit_touch_event_cb, LV_EVENT_ALL, NULL);

    ui_touch_xy = lv_label_create(Elecrow_touch_screen);
    lv_obj_set_width(ui_touch_xy, 400);   /// 1
    lv_obj_set_height(ui_touch_xy, 40);    /// 1
    lv_obj_set_x(ui_touch_xy, 0);
    lv_obj_set_y(ui_touch_xy, -50);
    lv_obj_set_align(ui_touch_xy, LV_ALIGN_CENTER);
    lv_obj_set_style_text_align(ui_touch_xy, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_text(ui_touch_xy, "Touch Adjust:    0    0");
    lv_obj_set_style_text_color(ui_touch_xy, lv_color_hex(0x09BEFB), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_touch_xy, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_touch_xy, &lv_font_montserrat_30, LV_PART_MAIN | LV_STATE_DEFAULT);
}


void elecrow_screen(void)
{
    elecrow_start_screen();
    lv_scr_load(Elecrow_logo_screen);

    elecrow_loading_screen();
    elecrow_screen_init();
}

/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/