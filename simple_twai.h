#ifndef SIMPLE_TWAI_H
#define SIMPLE_TWAI_H

#include <esp_twai.h>
#include <esp_twai_onchip.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstdint>
#include <string>

// Self-contained snapshot of one received frame. The driver's own twai_frame_t
// is only valid for the duration of the on_rx_done ISR callback, so receive()
// returns this POD copy instead.
struct SimpleTWAIMsg_t
{
    uint32_t identifier = 0;
    uint8_t data[8] = {};
    uint8_t data_length_code = 0;
    bool extd = false;
    bool rtr = false;
};

// Minimal wrapper around the ESP-IDF v5.4+ TWAI "Node" driver for sending and
// receiving CAN/TWAI frames.
class SimpleTWAI
{
public:
    // Optional receive callback, invoked from ISR context alongside the
    // internal queue used by receive(). Must be fast, non-blocking, and
    // IRAM-safe if CONFIG_TWAI_ISR_IN_IRAM is enabled.
    using RxCallback = bool (*)(const SimpleTWAIMsg_t &msg, void *user_ctx);

    // timing: e.g. "1Mbps", "500kbps", or a bare bitrate ("250000").
    // Case-insensitive; only the leading number and a "k"/"m" prefix matter.
    SimpleTWAI(uint8_t tx_pin, uint8_t rx_pin, std::string mode = "normal", std::string timing = "1Mbps");
    ~SimpleTWAI();

    SimpleTWAI(const SimpleTWAI &) = delete;
    SimpleTWAI &operator=(const SimpleTWAI &) = delete;

    void start();
    void stop();

    // wait=true (default) blocks until the frame has been transmitted;
    // wait=false queues it and returns immediately.
    //
    // The driver holds a pointer to `data` instead of copying it, so when
    // wait=false (or with sendFromISR below), `data` must stay valid until
    // the frame is actually sent - use a static/global buffer, not a stack
    // temporary.
    void send(uint32_t id, uint8_t *data, uint8_t dlc, bool isExtended = false,
              bool isRemote = false, bool wait = true, uint32_t wait_timeout_ms = 1000);

    // ISR-safe, non-blocking send; call only from interrupt context. Same
    // buffer-lifetime caveat as send(wait=false) above.
    bool sendFromISR(uint32_t id, uint8_t *data, uint8_t dlc, bool isExtended = false, bool isRemote = false);

    // Waits up to timeout_ms for a frame. On timeout, returns a
    // zero-initialized SimpleTWAIMsg_t and sets *received = false; callers
    // must check *received before trusting the result.
    SimpleTWAIMsg_t receive(uint16_t timeout_ms = 10000, bool *received = nullptr);
    void waitForMsg(uint32_t id, std::string msgToMatch = "");

    // Runtime ID filtering (single hardware mask filter on ESP32-S3).
    void setReceiveOnly(uint32_t id, bool isExtended = false);
    void setReceiveRange(uint32_t idLow, uint32_t idHigh, bool isExtended = false);
    void setReceiveAll();

    // Installs or replaces the optional ISR-context receive callback (see
    // RxCallback above); pass nullptr to remove it. Runs alongside, not
    // instead of, the queue that backs receive()/waitForMsg().
    void onReceive(RxCallback cb, void *user_ctx = nullptr);

private:
    static bool onRxDone_(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx);
    static uint32_t parseBitrate_(const std::string &timing);

    // Filter changes require the node to be disabled, so this pauses it if
    // needed and restores the previous state afterward.
    void applyMaskFilter_(const twai_mask_filter_config_t &cfg, const char *errCtx);

    twai_node_handle_t node_ = nullptr;
    QueueHandle_t rxQueue_ = nullptr;
    RxCallback userRxCb_ = nullptr;
    void *userRxCtx_ = nullptr;
    bool running_ = false;
};

#endif
