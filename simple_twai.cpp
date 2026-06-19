#include "simple_twai.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <esp_attr.h>

SimpleTWAI::SimpleTWAI(uint8_t tx_pin, uint8_t rx_pin, std::string mode, std::string timing)
{
    uint32_t bitrate = parseBitrate_(timing);
    if (bitrate == 0)
    {
        printf("Unrecognised TWAI timing string: %s\n", timing.c_str());
    }

    twai_onchip_node_config_t node_config = {};
    node_config.io_cfg.tx = (gpio_num_t)tx_pin;
    node_config.io_cfg.rx = (gpio_num_t)rx_pin;
    node_config.bit_timing.bitrate = bitrate;
    node_config.tx_queue_depth = 5;
    node_config.fail_retry_cnt = -1; // Retry until wait_timeout instead of single-shot, like classic CAN.

    if (mode == "normal")
    {
        // Flags default to 0 (normal operation).
    }
    else if (mode == "listener")
    {
        node_config.flags.enable_listen_only = true;
    }
    else
    {
        printf("Unknown TWAI mode: %s\n", mode.c_str());
    }

    esp_err_t err = twai_new_node_onchip(&node_config, &node_);
    if (err != ESP_OK)
    {
        printf("Failed to initialise TWAI bus: %s\n", esp_err_to_name(err));
        return;
    }

    rxQueue_ = xQueueCreate(16, sizeof(SimpleTWAIMsg_t));

    twai_event_callbacks_t cbs = {};
    cbs.on_rx_done = &SimpleTWAI::onRxDone_;
    twai_node_register_event_callbacks(node_, &cbs, this);

    setReceiveAll();
}

SimpleTWAI::~SimpleTWAI()
{
    if (node_)
    {
        twai_node_disable(node_); // Harmless if not currently enabled.
        twai_node_delete(node_);
    }
    if (rxQueue_) vQueueDelete(rxQueue_);
}

void SimpleTWAI::start()
{
    esp_err_t err = twai_node_enable(node_);
    if (err != ESP_OK)
    {
        printf("Failed to start TWAI bus: %s\n", esp_err_to_name(err));
        return;
    }
    running_ = true;
}

void SimpleTWAI::stop()
{
    esp_err_t err = twai_node_disable(node_);
    if (err != ESP_OK)
    {
        printf("Failed to stop TWAI bus: %s\n", esp_err_to_name(err));
        return;
    }
    running_ = false;
}

void SimpleTWAI::send(uint32_t id, uint8_t *data, uint8_t dlc, bool isExtended, bool isRemote,
                       bool wait, uint32_t wait_timeout_ms)
{
    if (dlc > 8)
    {
        printf("DLC must be 0-8, got %d\n", dlc);
        return;
    }

    twai_frame_t frame = {};
    frame.header.id = id;
    frame.header.ide = isExtended;
    frame.header.rtr = isRemote;
    frame.header.dlc = dlc;
    frame.buffer = isRemote ? nullptr : data;
    frame.buffer_len = isRemote ? 0 : dlc;

    esp_err_t err = twai_node_transmit(node_, &frame, wait ? (int)wait_timeout_ms : 0);
    if (err != ESP_OK)
    {
        printf("Failed to queue TWAI message: %s\n", esp_err_to_name(err));
        return;
    }

    if (wait)
    {
        err = twai_node_transmit_wait_all_done(node_, (int)wait_timeout_ms);
        if (err != ESP_OK)
        {
            printf("TWAI transmit did not complete in time: %s\n", esp_err_to_name(err));
        }
    }
}

bool SimpleTWAI::sendFromISR(uint32_t id, uint8_t *data, uint8_t dlc, bool isExtended, bool isRemote)
{
    if (dlc > 8) return false;

    twai_frame_t frame = {};
    frame.header.id = id;
    frame.header.ide = isExtended;
    frame.header.rtr = isRemote;
    frame.header.dlc = dlc;
    frame.buffer = isRemote ? nullptr : data;
    frame.buffer_len = isRemote ? 0 : dlc;

    return twai_node_transmit(node_, &frame, 0) == ESP_OK;
}

SimpleTWAIMsg_t SimpleTWAI::receive(uint16_t timeout_ms, bool *received)
{
    SimpleTWAIMsg_t msg{};
    bool ok = xQueueReceive(rxQueue_, &msg, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
    if (received != nullptr) *received = ok;
    return msg;
}

void SimpleTWAI::waitForMsg(uint32_t id, std::string msgToMatch)
{
    SimpleTWAIMsg_t msg;
    bool received;

    while (true)
    {
        msg = receive(10000, &received);
        if (!received) continue;

        if (msgToMatch.empty())
        {
            if (msg.identifier == id) break;
        }
        else if (msg.identifier == id &&
                 strncmp((char *)msg.data, msgToMatch.c_str(), msg.data_length_code) == 0)
        {
            break;
        }
    }
}

void SimpleTWAI::setReceiveOnly(uint32_t id, bool isExtended)
{
    uint32_t idWidthMask = isExtended ? 0x1FFFFFFFu : 0x7FFu;

    twai_mask_filter_config_t cfg = {};
    cfg.id = id & idWidthMask;
    cfg.mask = idWidthMask; // Every ID bit must match exactly.
    cfg.is_ext = isExtended;

    applyMaskFilter_(cfg, "receive-only");
}

void SimpleTWAI::setReceiveRange(uint32_t idLow, uint32_t idHigh, bool isExtended)
{
    if (idLow > idHigh)
    {
        uint32_t tmp = idLow;
        idLow = idHigh;
        idHigh = tmp;
    }

    uint32_t idWidthMask = isExtended ? 0x1FFFFFFFu : 0x7FFu;

    // A single hardware mask can only express a power-of-two-aligned ID
    // block, so this finds the smallest such block containing [idLow,
    // idHigh] - exact if already aligned, otherwise a superset.
    uint32_t diff = (idLow ^ idHigh) & idWidthMask;
    uint32_t wildcard = 0;
    while (diff)
    {
        wildcard = (wildcard << 1) | 1;
        diff >>= 1;
    }

    twai_mask_filter_config_t cfg = {};
    cfg.mask = idWidthMask & ~wildcard;
    cfg.id = idLow & cfg.mask;
    cfg.is_ext = isExtended;

    applyMaskFilter_(cfg, "receive-range");
}

void SimpleTWAI::setReceiveAll()
{
    twai_mask_filter_config_t cfg = {};
    cfg.id = 0;
    cfg.mask = 0; // Every ID bit is don't-care.
    cfg.is_ext = false;

    applyMaskFilter_(cfg, "accept-all");
}

void SimpleTWAI::applyMaskFilter_(const twai_mask_filter_config_t &cfg, const char *errCtx)
{
    // twai_node_config_mask_filter() only works while the node is disabled.
    bool wasRunning = running_;
    if (wasRunning) twai_node_disable(node_);

    esp_err_t err = twai_node_config_mask_filter(node_, 0, &cfg);
    if (err != ESP_OK)
    {
        printf("Failed to set TWAI %s filter: %s\n", errCtx, esp_err_to_name(err));
    }

    if (wasRunning) twai_node_enable(node_);
}

void SimpleTWAI::onReceive(RxCallback cb, void *user_ctx)
{
    userRxCb_ = cb;
    userRxCtx_ = user_ctx;
}

IRAM_ATTR bool SimpleTWAI::onRxDone_(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    (void)edata;
    SimpleTWAI *self = static_cast<SimpleTWAI *>(user_ctx);

    uint8_t buf[8];
    twai_frame_t frame = {};
    frame.buffer = buf;
    frame.buffer_len = sizeof(buf);

    if (twai_node_receive_from_isr(handle, &frame) != ESP_OK) return false;

    SimpleTWAIMsg_t msg{};
    msg.identifier = frame.header.id;
    msg.data_length_code = frame.header.dlc > 8 ? 8 : frame.header.dlc;
    msg.extd = frame.header.ide;
    msg.rtr = frame.header.rtr;
    if (!msg.rtr) memcpy(msg.data, buf, msg.data_length_code);

    BaseType_t higherPrioWoken = pdFALSE;
    if (self->rxQueue_) xQueueSendFromISR(self->rxQueue_, &msg, &higherPrioWoken);

    if (self->userRxCb_) self->userRxCb_(msg, self->userRxCtx_);

    return higherPrioWoken == pdTRUE;
}

uint32_t SimpleTWAI::parseBitrate_(const std::string &timing)
{
    std::string s;
    s.reserve(timing.size());
    for (char c : timing)
    {
        if (!isspace((unsigned char)c)) s += (char)tolower((unsigned char)c);
    }

    size_t i = 0;
    while (i < s.size() && (isdigit((unsigned char)s[i]) || s[i] == '.')) ++i;
    if (i == 0) return 0;

    double value = atof(s.substr(0, i).c_str());
    char suffix = (i < s.size()) ? s[i] : '\0';

    uint32_t multiplier = 1;
    if (suffix == 'm') multiplier = 1000000;
    else if (suffix == 'k') multiplier = 1000;

    return (uint32_t)(value * multiplier);
}
