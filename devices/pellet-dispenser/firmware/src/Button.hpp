#pragma once

#include "pico/types.h"
#include <optional>

struct Button {
	enum class State {
		RELEASED,
		PRESSED,
		LONG_PRESS,
	};
	enum State State;

	Button(uint pin);
	std::optional<enum State> Process(absolute_time_t time);

private:
	static constexpr uint LongPress_us = 300 * 1000; // 300ms
	static constexpr uint Debounce_us  = 5 * 1000;   // 5ms;

	uint            d_pin;
	absolute_time_t d_last;
};
