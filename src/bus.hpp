#pragma once

#include "pico/stdlib.h"
#include "hardware/gpio.h"

static constexpr int CLK_READ = 7;
static constexpr int CLK_PULL = 6;
static constexpr int DATA_READ = 11;
static constexpr int DATA_PULL = 10;
static constexpr int ATN_READ = 15;
static constexpr int ATN_PULL = 14;

struct bus
{
	int read_pin;
	int pull_pin;

	static bool timeout_occured;

	constexpr bus(int read, int pull)
		: read_pin(read)
		, pull_pin(pull)
	{}

	void init () const
	{
		gpio_init(read_pin);
		gpio_set_dir(read_pin, GPIO_IN);

		gpio_init(pull_pin);
		gpio_set_dir(pull_pin, GPIO_OUT);

		release();
	}

	/*
		Read-write single bit.
		This uses active-low logic, so
		low == true
	*/

	bool read () const {
		return not gpio_get(read_pin);
	}

	void write (bool on) const {
		gpio_put(pull_pin, on);
	}

	void hold () const {
		write(1);
	}

	void release () const {
		write(0);
	}

	/*
		High/low form of logic
	*/

	bool isHigh () const {
		return not read();
	}

	bool isLow () const {
		return read();
	}

	void high () const {
		write(0);
	}

	void low () const {
		write(1);
	}
};

constexpr bus clk_bus = {CLK_READ, CLK_PULL};
constexpr bus data_bus = {DATA_READ, DATA_PULL};
constexpr bus atn_bus = {ATN_READ, ATN_PULL};

inline void pulse(const bus& b)
{
	b.hold();
	sleep_ms(20);
	b.release();
	sleep_ms(10);
}

inline uint64_t micros()
{
	return to_us_since_boot(get_absolute_time());
}

constexpr uint64_t MAX_TIMEOUT = 1'000'000;
inline bool until(const bus& b, bool state, uint64_t timeout=MAX_TIMEOUT)
{
	b.release();
	auto start = micros();

	for (;;)
	{
		if (b.read() == state) {
			return true;
		}
		if (micros() - start > timeout) {
			bus::timeout_occured = true;
			return false;
		}
	}
}

inline bool untilHigh(const bus& b, uint64_t timeout=MAX_TIMEOUT)
{
	return until(b, false, timeout);
}

inline bool untilLow(const bus& b, uint64_t timeout=MAX_TIMEOUT)
{
	return until(b, true, timeout);
}

inline bool untilRisingEdge(const bus& b, uint64_t timeout=MAX_TIMEOUT)
{
	return untilLow(b, timeout) && untilHigh(b, timeout);
}

inline bool untilFallingEdge(const bus& b, uint64_t timeout=MAX_TIMEOUT)
{
	return untilHigh(b, timeout) && untilLow(b, timeout);
}

#define TRY(x) if (not (x)) { return false; }