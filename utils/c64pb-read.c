#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/frame.h"

#include <libserialport.h>

static void print_frame(frame f)
{
    if (f.device == 0 && f.channel == 0) {
        printf("DEBUG:");
    } else {
        printf("%u:%u:", f.device, f.channel);
    }

    if (f.type < FRAME_TYPE_MAX_) {
        printf("%s:", frame_type_string[f.type]);
    } else {
        printf("%u:", f.type);
    }

    for (int i=0; i<f.length; i++) {
        char c = f.data[i];
        if (isprint(c)) {
            putchar(c);
        } else {
            printf("\\%03o",c);
        }
    }

    putchar('\n');
}

static void serial_err() {
    char* msg = sp_last_error_message();
    fprintf(stderr, "libserialport error: %s\n", msg);
    sp_free_error_message(msg);
}

typedef struct sp_port* serial;

static int readloop(serial pico)
{
    int status = 0;

    struct sp_event_set* event_set;
    if (sp_new_event_set(&event_set) != SP_OK) {
        serial_err();
        return 1;
    }

    if (sp_add_port_events(event_set, pico, SP_EVENT_RX_READY) != SP_OK) {
        serial_err();
        status = 1;
        goto free_event_set;
    }

    for (;;) {
        frame f;
        memset(&f, 0, sizeof(f));

        sp_wait(event_set, 0);
        int n = sp_blocking_read(pico, &f, sizeof(f), 100);

        if (n <= 0) {
            fprintf(stderr, "Port read error, exiting\n");
            serial_err();
            status = 1;
            goto free_event_set;
        } else if (n < sizeof(f)) {
            fprintf(stderr, "Error - partial frame (%i/%zu)\n", n, sizeof(f));
        } else {
            print_frame(f);
        }
    }

free_event_set:
    sp_free_event_set(event_set);
    return status;
}

int main(int argc, char** argv)
{
    frame buf;
    char* path;
    int status = 0;

    if (argc >= 2) {
        path = argv[1];
    } else {
        path = "/dev/ttyACM0";
    }

    fprintf(stderr, "Reading from %s\n", path);

    serial pico;
    if (sp_get_port_by_name(path, &pico) != SP_OK) {
        fprintf(stderr, "Unable to access: %s\n", path);
        serial_err();
        return 1;
    }

    if (sp_open(pico, SP_MODE_READ) != SP_OK) {
        fprintf(stderr, "Couldn't open: %s\n", path);
        serial_err();
        status = 1;
        goto free_port;
    }

    status = readloop(pico);

close_port:
    sp_close(pico);
free_port:
    sp_free_port(pico);
}