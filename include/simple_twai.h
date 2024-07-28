#ifndef SIMPLE_TWAI_H
#define SIMPLE_TWAI_H

#include <driver/twai.h>

// simple initialisation functions
esp_err_t init_normal_twai(twai_handle_t* twai_bus, uint32_t* accpetance_id);
esp_err_t init_listener_twai(twai_handle_t* twai_bus, uint32_t* accpetance_id);
esp_err_t init_normal_twai_w_ISR(twai_handle_t* twai_bus, uint32_t* accpetance_id);
esp_err_t init_listener_twai_w_ISR(twai_handle_t* twai_bus, uint32_t* accpetance_id);

// simple message creation functions
twai_message_t std_data_twai_msg(uint32_t id, uint8_t* data, uint8_t dlc);
twai_message_t std_remote_twai_msg(uint32_t id);
twai_message_t ext_data_twai_msg(uint32_t id, uint8_t* data, uint8_t dlc);
twai_message_t ext_remote_twai_msg(uint32_t id);

// simple bus control functions
esp_err_t start_twai_bus(twai_handle_t twai_bus);
esp_err_t stop_twai_bus(twai_handle_t twai_bus);
esp_err_t send_twai_msg(twai_handle_t twai_bus, twai_message_t msg);
esp_err_t receive_twai_msg(twai_handle_t twai_bus, twai_message_t* msg);

// helper functions
void id_to_acceptance_filter(uint32_t* acceptance_id, uint32_t mask, uint32_t id, bool is_single);

#endif
