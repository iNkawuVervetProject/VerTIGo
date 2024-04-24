#pragma once

#include "pico/types.h"

#include <cstdint>
#include <optional>
#include <tuple>

#include "hardware/PIOIRSensor.hpp"

class PelletCounter {
public:
	struct StaticConfig : public PIOIRSensor<2>::Config {
		uint IRPin, SensorEnablePin;
	};

	struct Config {
		uint SensorLowThreshold  = 100;
		uint SensorHighThreshold = 200;
	};

	PelletCounter(const StaticConfig &staticConfig, const Config &config);

	std::tuple<std::optional<uint>, std::optional<uint>>
	Process(absolute_time_t time);

	void SetEnabled(bool enabled);

	bool PelletPresence() const;

	uint PelletCount() const;

private:
	PIOIRSensor<2> d_sensors;

	const Config &d_config;
	bool          d_state     = false;
	uint          d_count     = 0;
	uint          d_lastValue = 0;
};
