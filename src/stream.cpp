#include "pico/stdlib.h"

#include "stream.hpp"
#include "queue.hpp"
#include "frame.h"

#include <cstdarg>
#include <cstring>
#include <string>

static queue_t device_to_c64_handle;
static queue_t c64_to_device_handle;
static queue<frame> device_to_c64{&device_to_c64_handle};
static queue<frame> c64_to_device{&c64_to_device_handle};

void stream::init()
{
    device_to_c64.init(20);
    c64_to_device.init(50);
}

/* PC -- pico */

static bool valid(int raw)
{
    return (0 <= raw) && (raw < 256);
}

void recv_usb()
{
    if (device_to_c64.full())
        return;
  
    auto r = getchar_timeout_us(0);

    if (not valid(r))
        return;

    frame recv{};
    char* buf = reinterpret_cast<char*>(&recv);

    buf[0] = r;

    for (int i=1; i<FRAME_TOTAL_SIZE; i++) {
        r = getchar_timeout_us(100'000);
        if (not valid(r)) {
            stream::logf("Incomplete usb recv frame (%i/%zu)\n", i, sizeof(frame));
            return;
        }
        buf[i] = r;
    }
    device_to_c64.push(recv);
}

void send_usb()
{
    if (c64_to_device.empty())
        return;

    frame f;
    c64_to_device.pop(f);
    char* buf = reinterpret_cast<char*>(&f);

    for (int i=0; i<FRAME_TOTAL_SIZE; i++) {
        putchar_raw(buf[i]);
    }
}

void stream::process_usb()
{
    for (int i=0; i<3; i++) {
        send_usb();
    }
    recv_usb();
}

/* pico -> c64 */

static frame sending{};
static int sent{};

char stream::read()
{
    if (sent >= sending.length) {
        // Next frame
        memset(&sending, 0, sizeof(frame));
        device_to_c64.try_pop(sending);
        sent = 0;
    }
    char ret = static_cast<char>(sending.data[sent]);
    sent++;
    return ret;
}

bool stream::eoi()
{
    return (sent == sending.length) && (sending.type == FRAME_EOI);
}

bool stream::available()
{
    return (sent < sending.length) || (not device_to_c64.empty());
}

/* c64 -> pico */

static frame receiving{};
static uint8_t primary{};
static uint8_t secondary{};

void stream::flush()
{
    if (receiving.length > 0 || receiving.type != FRAME_NORMAL) {
        c64_to_device.push(receiving);
        memset(&receiving, 0, sizeof(frame));
    }
    receiving.device = primary;
    receiving.channel = secondary;
}

void stream::set(frame_type type)
{
    receiving.type = type;
}

void stream::setDevice(uint8_t device)
{
    primary = device;
    receiving.device = device;
}

void stream::setChannel(uint8_t channel)
{
    secondary = channel;
    receiving.channel = channel;
}

void stream::write(char c)
{
    if (receiving.length >= FRAME_DATA_SIZE) {
        stream::flush();
    }
    receiving.data[receiving.length++] = static_cast<uint8_t>(c);
}

void stream::logf(const char* fmt, ...)
{
    if (c64_to_device.full()) {
        printf("Send queue full\n");
        return;
    }

    frame f{};
    auto buffer = reinterpret_cast<char*>(&f.data[0]);

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, FRAME_DATA_SIZE, fmt, args);
    va_end(args);

    if (n < 0 || n >= FRAME_DATA_SIZE) {
        const char error[] = "[Unable to fit message]";
        memcpy(buffer, error, sizeof(error));
        f.length = sizeof(error);
    } else {
        f.length = n;
    }

    c64_to_device.try_push(f);
}