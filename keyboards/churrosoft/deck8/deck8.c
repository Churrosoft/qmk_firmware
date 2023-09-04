#include "quantum.h"

enum via_churro {
    id_get_serial_no             = 0xcc
};

__attribute__((weak)) void churro_config_get_value(uint8_t *data) {
    // data = [ value_id, value_data ]
    uint8_t *value_id   = &(data[0]);
    uint8_t *value_data = &(data[1]);

    switch (*value_id) {
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

__attribute__((weak)) void churro_config_set_value(uint8_t *data) {
    return;
}

__attribute__((weak)) void churro_config_save(void) {
    return;
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