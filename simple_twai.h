#ifndef SIMPLE_TWAI_H
#define SIMPLE_TWAI_H

#include <driver/twai.h>
#include <string>


class SimpleTWAI 
{
public:
    SimpleTWAI(uint8_t tx_pin, uint8_t rx_pin, std::string mode="normal", twai_timing_config_t timing=TWAI_TIMING_CONFIG_1MBITS());
    void start();
    void stop();

    void send(uint32_t id, uint8_t* data, uint8_t dlc, bool isExtended=false, bool isRemote=false);
    twai_message_t receive(uint16_t timeout_ms=10000);
    void waitForMsg(uint32_t id, std::string msgToMatch="");

private:
    twai_handle_t twai_bus_;
};

#endif
