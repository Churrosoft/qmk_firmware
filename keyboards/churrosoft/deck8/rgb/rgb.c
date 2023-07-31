#include "quantum.h"


// Single Indicator memory layout
typedef struct _indicator_config_t {
    uint8_t h;
    uint8_t s;
    uint8_t v;
    bool    enabled;
} indicator_config;

enum via_churro {
    id_caps_indicator_enabled    = 1,
    id_caps_indicator_brightness = 2,
    id_caps_indicator_color      = 3,
    id_get_serial_no             = 0xcc
};

indicator_config leds_cfg[DYNAMIC_KEYMAP_LAYER_COUNT][RGB_MATRIX_LED_COUNT];

void eeconfig_init_kb(void) {
    for(uint8_t k=0; k < DYNAMIC_KEYMAP_LAYER_COUNT; k++) {
        for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
            leds_cfg[k][i].h       = 0;
            leds_cfg[k][i].s       = 0;
            leds_cfg[k][i].v       = 100;
            leds_cfg[k][i].enabled = false;
        }
    }
    
    // Write default value to EEPROM now
    eeconfig_update_kb_datablock(&leds_cfg);
}

bool rgb_matrix_indicators_kb(void) {
    uint8_t current_layer = get_highest_layer(layer_state);

    for(uint8_t i=0; i < RGB_MATRIX_LED_COUNT; i++) {
        if(leds_cfg[current_layer][i].enabled) {
            HSV hsv_indicator_color = {leds_cfg[current_layer][i].h, leds_cfg[current_layer][i].s, leds_cfg[current_layer][i].v};
            RGB rgb_indicator_color = hsv_to_rgb(hsv_indicator_color);

            rgb_matrix_set_color(i, rgb_indicator_color.r, rgb_indicator_color.g, rgb_indicator_color.b);
        }
    }
    return true;
}

void churro_config_set_value(uint8_t *data) {
    // data = [ value_id, value_data ]
    uint8_t *value_id   = &(data[0]);
    uint8_t *value_data = &(data[1]);
    // value_data = [layer, keyId, value(s)]
    
    switch (*value_id) {
        case id_caps_indicator_enabled: {
            leds_cfg[value_data[0]][value_data[1]].enabled = value_data[2];
            break;
        }
        case id_caps_indicator_brightness: {
            leds_cfg[value_data[0]][value_data[1]].v = value_data[2];
            break;
        }
        case id_caps_indicator_color: {
            leds_cfg[value_data[0]][value_data[1]].h = value_data[2];
            leds_cfg[value_data[0]][value_data[1]].s = value_data[3];
            break;
        }
    }
}

void churro_config_get_value(uint8_t *data) {
    // data = [ value_id, value_data ]
    uint8_t *value_id   = &(data[0]);
    uint8_t *value_data = &(data[1]);

    switch (*value_id) {
        case id_caps_indicator_enabled: {
            value_data[2] = leds_cfg[value_data[0]][value_data[1]].enabled;
            break;
        }
        case id_caps_indicator_brightness: {
            value_data[2] = leds_cfg[value_data[0]][value_data[1]].v;
            break;
        }
        case id_caps_indicator_color: {
            value_data[2] = leds_cfg[value_data[0]][value_data[1]].h;
            value_data[3] = leds_cfg[value_data[0]][value_data[1]].s;
            break;
        }
        case id_get_serial_no: {
            uint8_t UniqueID[8];
            flash_get_unique_id(UniqueID);

            for (size_t i = 0; i < 8; i++) {
                value_data[2 + i] = UniqueID[i];
            }
            break;
        }
    }
}

void churro_config_save(void) {
    eeconfig_update_kb_datablock(&leds_cfg);
}


void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    // data = [ command_id, channel_id, value_id, value_data ]
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);
    
    if (*channel_id == id_custom_channel) {
        switch (*command_id) {
            case id_custom_set_value: {
                churro_config_set_value(value_id_and_data);
                break;
            }
            case id_custom_get_value: {
                churro_config_get_value(value_id_and_data);
                break;
            }
            case id_custom_save: {
                churro_config_save();
                break;
            }
            default: {
                // Unhandled message.
                *command_id = id_unhandled;
                break;
            }
        }
        return;
    }

    *command_id = id_unhandled;
}


// oled

#ifdef OLED_ENABLE

static uint32_t oled_logo_timer = 0;
static bool clear_logo = true;
static const char PROGMEM my_logo[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 
    0xff, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 
    0x0f, 0x0f, 0x0f, 0x1f, 0xff, 0xff, 0xff, 0x1f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 
    0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xfb, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xff, 
    0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#endif

#ifdef OLED_ENABLE

void init_timer(void){
    oled_logo_timer = timer_read32();
};

void user_oled_magic(void) {
    // Host Keyboard Layer Status
    oled_write_P(PSTR("Layer: "), false);

    switch (get_highest_layer(layer_state)) {
        case 0:
            oled_write_P(PSTR("One\n"), false);
            break;
        case 1:
            oled_write_P(PSTR("Two\n"), false);
            break;
        case 2:
            oled_write_P(PSTR("Three\n"), false);
            break;
        case 3:
            oled_write_P(PSTR("Four\n"), false);
            break;
        case 4:
            oled_write_P(PSTR("Five\n"), false);
            break;
        case 5:
            oled_write_P(PSTR("Six\n"), false);
            break;
        case 6:
            oled_write_P(PSTR("Seven\n"), false);
            break;
        case 7:
            oled_write_P(PSTR("Eight\n"), false);
            break;
        case 8:
            oled_write_P(PSTR("Nine\n"), false);
            break;
        case 9:
            oled_write_P(PSTR("Ten\n"), false);
            break;
        default:
            // Or use the write_ln shortcut over adding '\n' to the end of your string
            oled_write_ln_P(PSTR("Undefined"), false);
    }

    // Host Keyboard LED Status
    led_t led_state = host_keyboard_led_state();
    oled_write_P(led_state.caps_lock ? PSTR("Cap(x) ") : PSTR("Cap( ) "), false);
    oled_write_P(led_state.num_lock ? PSTR("Num(x) ") : PSTR("Num( ) "), false);
    oled_write_P(led_state.scroll_lock ? PSTR("Scrl(x)") : PSTR("Scrl( )"), false);


    switch (rgb_matrix_get_mode()) {
        case 1:
            oled_write_P(PSTR("Solid Color\n                  "), false);
            break;
        case 2:
            oled_write_P(PSTR("Alphas Mods\n                  "), false);
            break;
        case 3:
            oled_write_P(PSTR("Gradient Up Down\n                  "), false);
            break;
        case 4:
            oled_write_P(PSTR("Gradient Left Right\n                  "), false);
            break;
        case 5:
            oled_write_P(PSTR("Breathing\n                  "), false);
            break;
        case 6:
            oled_write_P(PSTR("Band Sat\n                  "), false);
            break;
        case 7:
            oled_write_P(PSTR("Band Val\n                  "), false);
            break;
        case 8:
            oled_write_P(PSTR("Band Pinwheel Sat\n                  "), false);
            break;
        case 9:
            oled_write_P(PSTR("Band Pinwheel Val\n                  "), false);
            break;
        case 10:
            oled_write_P(PSTR("Band Spiral Sat\n                  "), false);
            break;
        case 11:
            oled_write_P(PSTR("Band Spiral Val\n                  "), false);
            break;
        case 12:
            oled_write_P(PSTR("Cycle All\n                  "), false);
            break;
        case 13:
            oled_write_P(PSTR("Cycle Left Right\n                  "), false);
            break;
        case 14:
            oled_write_P(PSTR("Cycle Up Down\n                  "), false);
            break;
        case 15:
            oled_write_P(PSTR("Rainbow\nMoving Chevron    "), false);
            break;
        case 16:
            oled_write_P(PSTR("Cycle Out In\n                  "), false);
            break;
        case 17:
            oled_write_P(PSTR("Cycle Out In Dual\n                  "), false);
            break;
        case 18:
            oled_write_P(PSTR("Cycle Pinwheel\n                  "), false);
            break;
        case 19:
            oled_write_P(PSTR("Cycle Spiral\n                  "), false);
            break;
        case 20:
            oled_write_P(PSTR("Dual Beacon\n                  "), false);
            break;
        case 21:
            oled_write_P(PSTR("Rainbow Beacon\n                  "), false);
            break;
        case 22:
            oled_write_P(PSTR("Rainbow Pinwheels\n                  "), false);
            break;
        case 23:
            oled_write_P(PSTR("Raindrops\n                  "), false);
            break;
        case 24:
            oled_write_P(PSTR("Jellybean Raindrops\n                  "), false);
            break;
        case 25:
            oled_write_P(PSTR("Hue Breathing\n                  "), false);
            break;
        case 26:
            oled_write_P(PSTR("Hue Pendulum\n                  "), false);
            break;
        case 27:
            oled_write_P(PSTR("Hue Wave\n                  "), false);
            break;
        case 28:
            oled_write_P(PSTR("Pixel Rain\n                  "), false);
            break;
        case 29:
            oled_write_P(PSTR("Pixel Flow\n                  "), false);
            break;
        case 30:
            oled_write_P(PSTR("Pixel Fractal\n                  "), false);
            break;
        case 31:
            oled_write_P(PSTR("Typing Heatmap\n                  "), false);
            break;
        case 32:
            oled_write_P(PSTR("Digital Rain\n                  "), false);
            break;
        case 33:
            oled_write_P(PSTR("Solid Reactive\nSimple            "), false);
            break;
        case 34:
            oled_write_P(PSTR("Solid Reactive\n                  "), false);
            break;
        case 35:
            oled_write_P(PSTR("Solid Reactive\nWide              "), false);
            break;
        case 36:
            oled_write_P(PSTR("Solid Reactive\nMultiwide         "), false);
            break;
        case 37:
            oled_write_P(PSTR("Solid Reactive\nCross             "), false);
            break;
        case 38:
            oled_write_P(PSTR("Solid Reactive\nMulticross        "), false);
            break;
        case 39:
            oled_write_P(PSTR("Solid Reactive\nNexus             "), false);
            break;
        case 40:
            oled_write_P(PSTR("Solid Reactive\nMultinexus        "), false);
            break;
        case 41:
            oled_write_P(PSTR("Splash\n                  "), false);
            break;
        case 42:
            oled_write_P(PSTR("Multisplash\n                  "), false);
            break;
        case 43:
            oled_write_P(PSTR("Solid Splash\n                  "), false);
            break;
        case 44:
            oled_write_P(PSTR("Solid Multisplash\n                  "), false);
            break;
        default:
            // Or use the write_ln shortcut over adding '\n' to the end of your string
            oled_write_ln_P(PSTR("Undefined\n                  "), false);
    }

}

void render_logo(void) {
    oled_write_raw_P(my_logo, sizeof(my_logo));
}

void clear_screen(void) {
    if (clear_logo){
        for (uint8_t i = 0; i < OLED_DISPLAY_HEIGHT; ++i) {
            for (uint8_t j = 0; j < OLED_DISPLAY_WIDTH; ++j) {
                oled_write_raw_byte(0x0, i*OLED_DISPLAY_WIDTH + j);
            }
        }
        clear_logo = false;
    }
}

oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    return OLED_ROTATION_180;
}

//void keyboard_post_init_kb(void) {
//    init_timer();
//
//    keyboard_post_init_user();
//}

#    define SHOW_LOGO 5000
bool oled_task_kb(void) {
    if (!oled_task_user()) { return false; }
    if ((timer_elapsed32(oled_logo_timer) < SHOW_LOGO)){
        render_logo();
    }else{
        clear_screen();
        user_oled_magic();
    }
    return false;
}

#endif


void keyboard_post_init_kb(void) {
    //Read our custom menu variables from memory
    eeconfig_read_kb_datablock(&leds_cfg);
    
#ifdef OLED_ENABLE
    
    // oled
    init_timer();
    
    keyboard_post_init_user();
#endif
}
