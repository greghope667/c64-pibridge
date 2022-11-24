#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "bus.hpp"
#include "iec_protocol.hpp"
#include "stream.hpp"

bool bus::timeout_occured = false;

void core1_entry()
{
	iec::serial conn{};

	for (;;) {
		conn.tick();
	}
}

int main()
{
	stdio_init_all();

	clk_bus.init();
	data_bus.init();
	atn_bus.init();

	stream::init();

	multicore_launch_core1(core1_entry);

	for (;;) {
		stream::process_usb();
		sleep_ms(1);
	}

	return 0;
}
