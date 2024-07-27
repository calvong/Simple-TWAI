#include <string.h>
#include "simple_twai.h"

esp_err_t init_normal_twai()
{
    printf("Initializing normal TWAI\n");

    return ESP_OK;
}

esp_err_t init_listener_twai()
{
    printf("Initializing listener TWAI\n");

    return ESP_OK;
}

twai_message_t std_data_twai_msg(uint32_t id, uint8_t *data, uint8_t dlc)
{
    twai_message_t msg;
    msg.extd = 0;
    msg.rtr = 0;
    msg.ss = 0;   
    msg.self = 0;
    msg.dlc_non_comp = 0;   // this should not be 1 to stay compliant with ISO 11898-1
    msg.identifier = id;
    msg.data_length_code = dlc;

    memcpy(msg.data, data, dlc);

    return msg;
}

twai_message_t std_remote_twai_msg(uint32_t id)
{
    twai_message_t msg;
    msg.extd = 0;
    msg.rtr = 1;
    msg.ss = 0;   
    msg.self = 0;
    msg.dlc_non_comp = 0;   // this should not be 1 to stay compliant with ISO 11898-1
    msg.identifier = id;
    msg.data_length_code = 0;

    return msg;
}

twai_message_t ext_data_twai_msg(uint32_t id, uint8_t *data, uint8_t dlc)
{
    twai_message_t msg;
    msg.extd = 1;
    msg.rtr = 0;
    msg.ss = 0;   
    msg.self = 0;
    msg.dlc_non_comp = 0;   // this should not be 1 to stay compliant with ISO 11898-1
    msg.identifier = id;
    msg.data_length_code = dlc;

    memcpy(msg.data, data, dlc);

    return msg;
}

twai_message_t ext_remote_twai_msg(uint32_t id)
{
    twai_message_t msg;
    msg.extd = 1;
    msg.rtr = 1;
    msg.ss = 0;   
    msg.self = 0;
    msg.dlc_non_comp = 0;   // this should not be 1 to stay compliant with ISO 11898-1
    msg.identifier = id;
    msg.data_length_code = 0;

    return msg;
}