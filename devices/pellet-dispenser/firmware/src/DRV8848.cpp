#include "DRV8848.hpp"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <algorithm>

#include <cstdio>

#define PWM_RESOLUTION (1 << 10)

DRV8848::DRV8848(const Config &config)
    : d_nSleep{config.nSleep}
    , d_nFault{config.nFault}
    , d_A{config.AIn1, config.AIn2}
    , d_B{config.BIn1, config.BIn2} {

	gpio_init(d_nSleep);
	gpio_set_dir(d_nSleep, 1);
	gpio_put(d_nSleep, 0);

	gpio_init(d_nFault);

	gpio_set_dir(d_nFault, 0);
	gpio_set_pulls(d_nFault, true, false);
}

void DRV8848::SetEnabled(bool on) {
	gpio_put(d_nSleep, on);
}

void DRV8848::SetChannelA(int value) {
	d_A.Set(value);
}

void DRV8848::SetChannelB(int value) {
	d_B.Set(value);
}

bool DRV8848::HasFault() const {
	return !gpio_get(d_nFault);
}

DRV8848::Channel::Channel(uint in1, uint in2) {
	d_slices[0]   = pwm_gpio_to_slice_num(in1);
	d_slices[1]   = pwm_gpio_to_slice_num(in2);
	d_channels[0] = pwm_gpio_to_channel(in1);
	d_channels[1] = pwm_gpio_to_channel(in2);

	gpio_set_function(in1, GPIO_FUNC_PWM);
	gpio_set_function(in2, GPIO_FUNC_PWM);

	for (auto slice : d_slices) {
		pwm_set_wrap(slice, PWM_RESOLUTION - 1);
		// 125MHz / 1024 -> 122kHz PWM, above human hearing, and also vervet
		// (45kHz).
		// ref:https://pubs.aip.org/asa/jasa/article/71/S1/S51/648857/Absolute-auditory-thresholds-in-monkeys-and-humans
		pwm_set_clkdiv(slice, 1.0);
		pwm_set_enabled(slice, true);
	}

	Set(0);
}

void DRV8848::Channel::Set(int value) {
	size_t activeIndex = 1;
	if (value < 0) {
		activeIndex = 0;
		value       = -value;
	}
	value                = std::min(PWM_RESOLUTION - 1, value);
	size_t inactiveIndex = 1 - activeIndex;

	// set the active index to resolution - value (inverse polarity)
	pwm_set_chan_level(
	    d_slices[activeIndex],
	    d_channels[activeIndex],
	    PWM_RESOLUTION - value + 1
	);

	// set the inactive index  high
	pwm_set_chan_level(
	    d_slices[inactiveIndex],
	    d_channels[inactiveIndex],
	    PWM_RESOLUTION - 1
	);
}