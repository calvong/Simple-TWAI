#include <string.h>
#include "simple_twai.h"

#define DEFAULT_TWAI_TX_WAIT_MS 1000
#define DEFAULT_TWAI_RX_WAIT_MS 1000

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t init_normal_twai(twai_handle_t* twai_bus, gpio_num_t tx_pin, gpio_num_t rx_pin, uint32_t* accpetance_id)
{
    //Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_pin, rx_pin, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();  // default to 500kbps
    twai_filter_config_t f_config;
    
    if (accpetance_id != NULL) 
    {
        // TODO: compute acceptance filter
        id_to_acceptance_filter(
            accpetance_id,
            &f_config.acceptance_code, 
            &f_config.acceptance_mask, 
            &f_config.single_filter);
    } 
    else 
    {
        // accept all messages
        f_config.acceptance_code = 0;
        f_config.acceptance_mask = 0xFFFFFFFF;
        f_config.single_filter = true;
    }

    g_config.controller_id = 0; // assume a single TWAI bus

    return (twai_driver_install_v2(&g_config, &t_config, &f_config, twai_bus) == ESP_OK);
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

void id_to_acceptance_filter(uint32_t* acceptance_id, uint32_t* code, uint32_t* mask, bool* is_single)
{

}

#ifdef __cplusplus
}
#endif