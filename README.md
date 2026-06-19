# Simple-TWAI

An ESP-IDF component library to simplify the use of the ESP TWAI/CAN driver. Built on top
of the IDF v5.5+ "Node" TWAI driver (`esp_driver_twai` / `esp_twai.h`), not the legacy
`driver/twai.h` queue-based API.

## Basic usage

```cpp
SimpleTWAI twai(tx_pin, rx_pin, "normal", "500kbps"); // mode: "normal" or "listener"
twai.start();

uint8_t data[4] = {1, 2, 3, 4};
twai.send(0x123, data, sizeof(data));              // blocks until sent

bool received = false;
SimpleTWAIMsg_t msg = twai.receive(100, &received); // 100 ms timeout
if (received) { /* msg.identifier, msg.data, msg.data_length_code, msg.extd, msg.rtr */ }
```

`timing` accepts strings like `"1Mbps"`, `"500kbps"`, `"250kbps"`, or a bare bitrate in
bits/sec (`"250000"`) - case-insensitive, only the leading number and a `k`/`m` prefix are
checked.

## Sending without blocking

```cpp
twai.send(id, data, dlc, false, false, /*wait=*/false);
bool ok = twai.sendFromISR(id, data, dlc); // call only from interrupt context
```

The new TWAI driver holds a pointer to `data` rather than copying it, so when `wait=false`
(or via `sendFromISR`), `data` **must remain valid until the frame actually goes out on the
wire** - use a static/global buffer, not a stack temporary that goes out of scope when the
calling function returns.

## Runtime ID filtering

ESP32-S3 has a single hardware mask filter, so only one filter is active at a time:

```cpp
twai.setReceiveOnly(0x123);             // accept only ID 0x123
twai.setReceiveRange(0x100, 0x1FF);     // accept an ID block (see caveat below)
twai.setReceiveAll();                   // clear filtering, accept everything
```

`setReceiveRange()` is implemented with a single bitmask, which can only express a
power-of-two-aligned ID block. If `[idLow, idHigh]` isn't already aligned to such a block,
the effective filter is the smallest aligned block that contains the requested range (a
superset - it may also pass some IDs outside what you asked for).

The driver only allows filter changes while the node is disabled, so each `setReceiveXXX()`
call briefly stops and restarts the node if it was running. Avoid calling these frequently
on a busy bus - each switch is a short window where this node won't ACK other nodes' frames,
which is harmless occasionally (CAN's error-retry handles it) but adds up if done constantly.

## Receiving: options

Messages arrive via an `on_rx_done` ISR callback in the new driver, so receiving them
"synchronously" needs a small bridge. Two are available, and can be used together:

1. **Internal queue (default).** Every received frame is copied into a small FreeRTOS
   queue; `receive(timeout_ms)` / `waitForMsg()` block on that queue. This is the natural
   fit for a state machine or polling loop - just call `receive()` wherever you'd normally
   check "is there a new message".
2. **Direct callback (`onReceive`).** Register a `RxCallback` to be invoked straight from
   the driver's ISR, in addition to the queue above. Lowest latency, but it runs in
   interrupt context: keep it short, non-blocking, and IRAM-safe if
   `CONFIG_TWAI_ISR_IN_IRAM` is enabled (avoid printf/most libc calls; this library's own
   internal callback is marked `IRAM_ATTR` accordingly).

If you outgrow both of these (e.g. need to inspect raw, possibly-partial byte streams
instead of fixed 0-8 byte frames), consider swapping the internal queue for an
`esp_ringbuf` ring buffer - not provided here since CAN frames are small and fixed-size.
