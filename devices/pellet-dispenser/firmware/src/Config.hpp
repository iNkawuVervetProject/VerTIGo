#pragma once

#include "PelletCounter.hpp"
#include "WheelController.hpp"
#include "pico/types.h"

enum class OutputChannel {
	A             = 0,
	B             = 1,
	STEPPER_MOTOR = 2,
};

struct Config {
	WheelController::Config Wheel;
	PelletCounter::Config   Pellet;

	uint          BackslashRewindPulseUS = 15 * 1000;
	OutputChannel Channel                = OutputChannel::B;
};
