#pragma once

#include "pico/types.h"

#include <cstdint>
#include <optional>

#include "hardware/PIOIRSensor.hpp"

class PelletCounter {
public:
	struct Config : public PIOIRSensor<2>::Config {
		uint IRPin, SensorEnablePin;
	};

	const static uint THRESHOLD_UP   = 200;
	const static uint THRESHOLD_DOWN = 160;

	PelletCounter(const Config &config);

	std::optional<uint> Process(absolute_time_t time);

	void SetEnabled(bool enabled);

	bool PelletPresence() const;

	uint PelletCount() const;

private:
	PIOIRSensor<2> d_sensors;

	bool d_state = false;
	uint d_count = 0;
};
