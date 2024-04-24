#pragma once

#include "Button.hpp"
#include "Config.hpp"
#include "PelletCounter.hpp"
#include "WheelController.hpp"
#include "pico/types.h"

class PelletDispenser {
public:
	struct StaticConfig : public PelletCounter::StaticConfig,
	                      public WheelController::StaticConfig {
		uint TestButtonPin;
	};

	PelletDispenser(const StaticConfig &staticConfig, const Config &config);

	void Dispense(uint count);

	void Process(absolute_time_t time);

private:
	Button          d_testButton;
	PelletCounter   d_pelletCounter;
	WheelController d_wheelController;
};
