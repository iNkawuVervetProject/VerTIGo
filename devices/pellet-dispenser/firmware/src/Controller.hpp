#pragma once

#include "Button.hpp"
#include "Config.hpp"
#include "Error.hpp"
#include "PelletCounter.hpp"
#include "WheelController.hpp"
#include "pico/types.h"

class Controller {
public:
	struct StaticConfig : public PelletCounter::StaticConfig,
	                      public WheelController::StaticConfig {
		uint TestButtonPin;
	};

	Controller(const StaticConfig &staticConfig, const Config &config);

	void Dispense(
	    uint count, const std::function<void(Error)> &callback = [](Error) {}
	);

	struct CalibrationResult {};

	void Calibrate(
	    const std::function<void(const CalibrationResult &)> &callback =
	        [](const CalibrationResult &) {}
	);

	void Process(absolute_time_t time);

private:
	class Mode {
	public:
		virtual ~Mode() = default;

		virtual std::unique_ptr<Mode> operator()(absolute_time_t) = 0;
	};

	Button d_testButton;

	PelletCounter         d_pelletCounter;
	WheelController       d_wheelController;
	std::unique_ptr<Mode> d_mode;
};
