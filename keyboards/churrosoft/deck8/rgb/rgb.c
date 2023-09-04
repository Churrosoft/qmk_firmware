#include "quantum.h"

// Single Indicator memory layout
typedef struct _indicator_config_t {
    uint8_t h;
    uint8_t s;
    uint8_t v;
    bool    enabled;
} indicator_config;

indicator_config leds_cfg[DYNAMIC_KEYMAP_LAYER_COUNT][RGB_MATRIX_LED_COUNT];

enum via_churro_rgb {
    id_per_key_rgb_enable        = 1,
    id_per_key_rgb_brightness    = 2,
    id_per_key_rgb_color         = 3,
    id_get_serial_no             = 0xcc
};


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
        case id_per_key_rgb_enable: {
            leds_cfg[value_data[0]][value_data[1]].enabled = value_data[2];
            break;
        }
        case id_per_key_rgb_brightness: {
            leds_cfg[value_data[0]][value_data[1]].v = value_data[2];
            break;
        }
        case id_per_key_rgb_color: {
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
        case id_per_key_rgb_enable: {
            value_data[2] = leds_cfg[value_data[0]][value_data[1]].enabled;
            break;
        }
        case id_per_key_rgb_brightness: {
            value_data[2] = leds_cfg[value_data[0]][value_data[1]].v;
            break;
        }
        case id_per_key_rgb_color: {
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

void keyboard_post_init_kb(void) {
    //Read our custom menu variables from memory
    eeconfig_read_kb_datablock(&leds_cfg);
}
