#include <string.h>
#include "simple_twai.h"

#define DEFAULT_TWAI_TX_WAIT_MS 1000
#define DEFAULT_TWAI_RX_WAIT_MS 1000


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

esp_err_t start_twai_bus(twai_handle_t twai_bus)
{    
    return twai_start_v2(twai_bus);
}

esp_err_t stop_twai_bus(twai_handle_t twai_bus)
{
    return twai_stop_v2(twai_bus);
}

esp_err_t send_twai_msg(twai_handle_t twai_bus, twai_message_t msg)
{
    return twai_transmit_v2(twai_bus, &msg, pdMS_TO_TICKS(DEFAULT_TWAI_TX_WAIT_MS));
}

esp_err_t receive_twai_msg(twai_handle_t twai_bus, twai_message_t* msg)
{
    return twai_receive_v2(twai_bus, msg, pdMS_TO_TICKS(DEFAULT_TWAI_RX_WAIT_MS));
}