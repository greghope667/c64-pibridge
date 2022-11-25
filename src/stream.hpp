#pragma once

/* Streaming interface for PC-pico data transfer
 * Uses 4+64 byte frames for transfer
 */

#include "frame.h"

namespace stream
{
    /* USB layer */
    void process_usb();
    void init();

    /* Functions for pi pico-c64 transfer layer */
    /* pico -> c64 */
    char read();
    [[nodiscard]] bool available();
    [[nodiscard]] bool eoi();

    /* c64 -> pico */
    void write(char);
    void flush();
    void set(frame_type type);
    void setDevice(uint8_t device);
    void setChannel(uint8_t channel);
    void logf(const char* fmt, ...) __attribute__((format (printf, 1, 2)));
}