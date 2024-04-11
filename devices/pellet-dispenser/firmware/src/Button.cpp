#include "Button.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"

Button::Button(uint pin_)
    : pin{pin_}
    , last{nil_time} {
	gpio_init(pin);
	gpio_set_dir(pin, false);
	gpio_set_pulls(pin, true, false);
}

bool Button::Process(absolute_time_t curTime) {
	bool pressed = !gpio_get(pin);
	if (pressed == Pressed) {
		return false;
	}

	if (is_nil_time(last)) {
		last = curTime;
		return false;
	}

	if (absolute_time_diff_us(last, curTime) > 8000) {
		Pressed = pressed;
		last    = nil_time;
		return true;
	}

	return false;
}
