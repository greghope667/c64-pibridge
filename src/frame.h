#pragma once

#include <stdint.h>

#define FRAME_HEADER_SIZE 4
#define FRAME_DATA_SIZE 64
#define FRAME_TOTAL_SIZE (FRAME_DATA_SIZE + FRAME_HEADER_SIZE)

typedef enum frame_type {
    FRAME_NORMAL,
    FRAME_EOI,
    FRAME_OPEN,
    FRAME_CLOSE,
    FRAME_TYPE_MAX_
} frame_type;

static const char* const 
frame_type_string[] = {
    "NORMAL",
    "EOI",
    "OPEN",
    "CLOSE",
};

typedef struct frame {
    uint8_t device;
    uint8_t channel;
    uint8_t length;
    uint8_t type;

    uint8_t data[FRAME_DATA_SIZE];
} frame;
