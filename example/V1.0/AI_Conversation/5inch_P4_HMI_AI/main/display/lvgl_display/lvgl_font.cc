/*---------------------------------------------------------------
 * Teaching module overview: User interface display
 * This file groups the lvgl_font responsibilities so learners can
 * follow the subsystem boundary before reading individual routines.
 *--------------------------------------------------------------*/

#include "lvgl_font.h"
#include <cbin_font.h>


LvglCBinFont::LvglCBinFont(void* data) {
    font_ = cbin_font_create(static_cast<uint8_t*>(data));
}

LvglCBinFont::~LvglCBinFont() {
    if (font_ != nullptr) {
        cbin_font_delete(font_);
    }
}
