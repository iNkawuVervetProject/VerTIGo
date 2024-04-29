#pragma once

#include "Button.hpp"
#include "Config.hpp"
#include "Error.hpp"
#include "PelletCounter.hpp"
#include "WheelController.hpp"
#include "hardware/DRV8848.hpp"
#include "pico/types.h"

class Mode;

class Controller : public Processor {
public:
	~Controller();

	struct StaticConfig {
		Button          &TestButton;
		IRSensor        &PelletSensor, &WheelSensor;
		PelletCounter   &Counter;
		WheelController &Wheel;
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

	void Process(absolute_time_t time) override;

private:
	friend class Mode;

	Button          &d_button;
	IRSensor        &d_wheelSensor, &d_pelletSensor;
	WheelController &d_wheel;
	PelletCounter   &d_counter;

	std::unique_ptr<Mode> d_mode;
};
