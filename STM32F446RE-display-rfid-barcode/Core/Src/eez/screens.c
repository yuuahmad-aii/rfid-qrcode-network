#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            lv_obj_set_pos(obj, 367, 273);
            lv_obj_set_size(obj, 100, 38);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // tombol_admin
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.tombol_admin = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_btn_admin_on_pressed, LV_EVENT_PRESSED, (void *)0);
                    add_style_header_text(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Admin");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 54, 148);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "Silahkan Scan Barcode atau RFID");
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
}

void create_screen_admin_password() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.admin_password = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            // button_matrix_password
            lv_obj_t *obj = lv_buttonmatrix_create(parent_obj);
            objects.button_matrix_password = obj;
            lv_obj_set_pos(obj, 132, 110);
            lv_obj_set_size(obj, 217, 195);
            static const char *map[16] = {
                "1",
                "2",
                "3",
                "\n",
                "4",
                "5",
                "6",
                "\n",
                "7",
                "8",
                "9",
                "\n",
                "ok",
                "0",
                "del",
                NULL,
            };
            lv_buttonmatrix_set_map(obj, map);
            lv_obj_add_event_cb(obj, action_button_matrix_password_pressed, LV_EVENT_PRESSED, (void *)0);
        }
        {
            // textarea_input_password
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.textarea_input_password = obj;
            lv_obj_set_pos(obj, 105, 35);
            lv_obj_set_size(obj, 271, 42);
            lv_textarea_set_max_length(obj, 128);
            lv_textarea_set_placeholder_text(obj, "input password");
            lv_textarea_set_one_line(obj, false);
            lv_textarea_set_password_mode(obj, false);
        }
    }
    
    tick_screen_admin_password();
}

void tick_screen_admin_password() {
}

void create_screen_admin_panel() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.admin_panel = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    
    tick_screen_admin_panel();
}

void tick_screen_admin_panel() {
}

void create_screen_qr_read() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.qr_read = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 154, 80);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "QR Code Terdeteksi");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 128, 127);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "ID :");
        }
        {
            // spinner_loading_1
            lv_obj_t *obj = lv_spinner_create(parent_obj);
            objects.spinner_loading_1 = obj;
            lv_obj_set_pos(obj, 223, 174);
            lv_obj_set_size(obj, 80, 80);
        }
        {
            // id_qrcode
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.id_qrcode = obj;
            lv_obj_set_pos(obj, 177, 127);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "Text");
        }
    }
    
    tick_screen_qr_read();
}

void tick_screen_qr_read() {
}

void create_screen_rfid_read() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.rfid_read = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            // rfid_terdeteksi
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.rfid_terdeteksi = obj;
            lv_obj_set_pos(obj, 164, 81);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "Rfid Terdeteksi");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 128, 127);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "ID :");
        }
        {
            // spinner_loading
            lv_obj_t *obj = lv_spinner_create(parent_obj);
            objects.spinner_loading = obj;
            lv_obj_set_pos(obj, 223, 174);
            lv_obj_set_size(obj, 80, 80);
        }
        {
            // id_rfid
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.id_rfid = obj;
            lv_obj_set_pos(obj, 177, 127);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "Text");
        }
    }
    
    tick_screen_rfid_read();
}

void tick_screen_rfid_read() {
}

void create_screen_access_rejected() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.access_rejected = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 157, 148);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "Akses Diterima");
        }
    }
    
    tick_screen_access_rejected();
}

void tick_screen_access_rejected() {
}

void create_screen_access_accepted() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.access_accepted = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 165, 148);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "Akses Ditolak");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 95, 183);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "Alasan :");
        }
        {
            // error_reason
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.error_reason = obj;
            lv_obj_set_pos(obj, 194, 183);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_header_text(obj);
            lv_label_set_text_static(obj, "Text");
        }
    }
    
    tick_screen_access_accepted();
}

void tick_screen_access_accepted() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_admin_password,
    tick_screen_admin_panel,
    tick_screen_qr_read,
    tick_screen_rfid_read,
    tick_screen_access_rejected,
    tick_screen_access_accepted,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 7) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
    create_screen_admin_password();
    create_screen_admin_panel();
    create_screen_qr_read();
    create_screen_rfid_read();
    create_screen_access_rejected();
    create_screen_access_accepted();
}