#include "Button.hpp"
#include "Log.hpp"
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

	Tracef("Button[%d]: pressed: %s", d_pin, pressed ? "true" : "false");

	switch (State) {
	case State::RELEASED: {
		if (is_nil_time(d_last) && pressed == false) {
			return std::nullopt;
		}
		if (is_nil_time(d_last)) {
			Debugf("Button[%d]: first pressed", d_pin);
			d_last = now;
		}

		auto diff = absolute_time_diff_us(d_last, now);
		if (pressed == false) {
			d_last = nil_time;
			if (diff < Debounce_us) {
				Debugf("Button[%d]: debounced", d_pin);
				return std::nullopt;
			}

			Debugf("Button[%d]: short press", d_pin);
			State = State::PRESSED;
			return State;
		}

		if (diff >= LongPress_us) {
			Debugf("Button[%d]: long press", d_pin);
			State  = State::LONG_PRESS;
			d_last = nil_time;
			return State;
		}
		break;
	}
	case State::PRESSED:
		if (pressed == false) {
			State = State::RELEASED;
			return State;
		}
		return std::nullopt;
	case State::LONG_PRESS: {
		if (pressed == false) {
			Debugf("Button[%d]: released", d_pin);
			State  = State::RELEASED;
			d_last = nil_time;
			return State;
		}
		break;
	}
	}
	return std::nullopt;
}
