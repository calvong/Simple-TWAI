#ifndef SIMPLE_TWAI_H
#define SIMPLE_TWAI_H

#include <driver/twai.h>

esp_err_t init_normal_twai();
esp_err_t init_listener_twai();
// twai_start() <- idf function      
// twai_stop()  <- idf function
twai_message_t std_data_twai_msg(uint32_t id, uint8_t *data, uint8_t dlc);
twai_message_t std_remote_twai_msg(uint32_t id);
twai_message_t ext_data_twai_msg(uint32_t id, uint8_t *data, uint8_t dlc);
twai_message_t ext_remote_twai_msg(uint32_t id);

#endif
