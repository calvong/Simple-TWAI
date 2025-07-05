#include <string.h>
#include "simple_twai.h"

#define DEFAULT_TWAI_TX_WAIT_MS 1000


SimpleTWAI::SimpleTWAI(uint8_t tx_pin, uint8_t rx_pin, std::string mode, twai_timing_config_t timing)
{
    esp_err_t err = ESP_OK;

    //Initialise configuration structures using macro initializers
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    if (mode == "normal") 
    {
        twai_general_config_t g_config = 
        TWAI_GENERAL_CONFIG_DEFAULT(
            (gpio_num_t) tx_pin, 
            (gpio_num_t) rx_pin, 
            TWAI_MODE_NORMAL);
        g_config.controller_id = 0; // assume a single TWAI bus

        err = twai_driver_install_v2(&g_config, &timing, &f_config, &twai_bus_);
    } 
    else if (mode == "listener") 
    {
        twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(
            (gpio_num_t) tx_pin, 
            (gpio_num_t) rx_pin, 
            TWAI_MODE_LISTEN_ONLY);
        g_config.controller_id = 0; // assume a single TWAI bus
        
        err = twai_driver_install_v2(&g_config, &timing, &f_config, &twai_bus_);
    } 
    else 
    {
        printf("Unknown TWAI mode: %s\n", mode.c_str());
    }

    if (err != ESP_OK) {
        // Handle error
        printf("Failed to initialise TWAI bus\n");
    }
}

void SimpleTWAI::start()
{
    esp_err_t err = twai_start_v2(twai_bus_);
    if (err != ESP_OK) 
    {
        printf("Failed to start TWAI bus\n");
    }
}

void SimpleTWAI::stop()
{
    esp_err_t err = twai_stop_v2(twai_bus_);
    if (err != ESP_OK) 
    {
        printf("Failed to stop TWAI bus\n");
    }
}

void SimpleTWAI::send(uint32_t id, uint8_t* data, uint8_t dlc, bool isExtended, bool isRemote)
{
    twai_message_t msg = {
        .extd = 0,
        .rtr = 0,
        .ss = 0,   // self-reception not supported
        .self = 0, // self-reception not supported
        .dlc_non_comp = 0,   // this should not be 1 to stay compliant with ISO 11898-1
        .identifier = id,
        .data_length_code = dlc
    };
    
    if (isExtended) msg.extd = 1;
    if (isRemote) msg.rtr = 1;

    if (dlc > 8) {
        printf("DLC must be 0-8, got %d\n", dlc);
        return;
    }

    if (!isRemote)  memcpy(msg.data, data, dlc);

    esp_err_t err = twai_transmit_v2(twai_bus_, &msg, pdMS_TO_TICKS(DEFAULT_TWAI_TX_WAIT_MS));

    // Check for errors
    if (err != ESP_OK)
    {
        printf("Failed to send TWAI message\n");
    }
}

twai_message_t SimpleTWAI::receive(uint16_t timeout_ms)
{
    twai_message_t msg;
    esp_err_t err = twai_receive_v2(twai_bus_, &msg, pdMS_TO_TICKS(timeout_ms));

    if (err == ESP_ERR_TIMEOUT) {} 
    else if (err == ESP_OK) {}
    else
    {
        printf("Failed to receive TWAI message: %s\n", esp_err_to_name(err));
        return {};
    }

    return msg;
}

void SimpleTWAI::waitForMsg(uint32_t id, std::string msgToMatch)
{
    twai_message_t msg;
    
    while (true) 
    {
        msg = receive();
        
        if (msgToMatch == "")
        {
            if (msg.identifier == id) 
            {
                break; // exit loop if message with matching ID is received
            }
        } 
        else 
        {
            // Check if the message matches the expected content
            if (msg.identifier == id && strncmp((char*)msg.data, msgToMatch.c_str(), msg.data_length_code) == 0) 
            {
                break; // exit loop if message with matching ID and content is received
            }
        }
    }
}


