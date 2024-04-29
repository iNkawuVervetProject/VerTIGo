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

void Button::Process(absolute_time_t now) {
	Clear();
	bool pressed = !gpio_get(d_pin);

	Tracef("Button[%d]: pressed: %s", d_pin, pressed ? "true" : "false");

	switch (State) {
	case ButtonState::RELEASED: {
		if (is_nil_time(d_last) && pressed == false) {
			return;
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
				return;
			}

			Debugf("Button[%d]: short press", d_pin);
			State = ButtonState::PRESSED;
			Publish(State, Error::NO_ERROR);
			return;
		}

		if (diff >= LongPress_us) {
			Debugf("Button[%d]: long press", d_pin);
			State  = ButtonState::LONG_PRESS;
			d_last = nil_time;
			Publish(State, Error::NO_ERROR);
			return;
		}
		break;
	}
	case ButtonState::PRESSED:
		if (pressed == false) {
			State = ButtonState::RELEASED;
			Publish(State, Error::NO_ERROR);
			return;
		}
		break;
	case ButtonState::LONG_PRESS: {
		if (pressed == false) {
			Debugf("Button[%d]: released", d_pin);
			State  = ButtonState::RELEASED;
			d_last = nil_time;
			Publish(State, Error::NO_ERROR);
			return;
		}
		break;
	}
	}
}
