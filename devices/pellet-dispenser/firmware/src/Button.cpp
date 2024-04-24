#include "Button.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include <optional>

Button::Button(uint pin_)
    : d_pin{pin_}
    , d_last{nil_time} {
	gpio_init(d_pin);
	gpio_set_dir(d_pin, false);
	gpio_set_pulls(d_pin, true, false);
}

std::optional<enum Button::State> Button::Process(absolute_time_t now) {
	bool pressed = !gpio_get(d_pin);

	switch (State) {
	case State::RELEASED: {
		if (is_nil_time(d_last) && pressed == false) {
			return std::nullopt;
		}
		if (is_nil_time(d_last)) {
			d_last = now;
		}

		auto diff = absolute_time_diff_us(d_last, now);
		if (pressed == false && diff >= Debounce_us) {
			return State::PRESSED;
		}

		if (diff >= LongPress_us) {
			State  = State::LONG_PRESS;
			d_last = nil_time;
			return State;
		}
		break;
	}
	case State::PRESSED:
		return std::nullopt;
	case State::LONG_PRESS: {
		if (pressed == false) {
			State  = State::RELEASED;
			d_last = nil_time;
			return State;
		}
		break;
	}
	}
	return std::nullopt;
}
