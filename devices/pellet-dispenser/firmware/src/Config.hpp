#pragma once

#include "WheelController.hpp"
#include "pico/types.h"

enum class OutputChannel {
	A             = 0,
	B             = 1,
	STEPPER_MOTOR = 2,
};

struct Config {
	WheelController::Config Wheel;

	uint          BackslashRewindPulseUS = 15 * 1000;
	OutputChannel Channel                = OutputChannel::B;
};
