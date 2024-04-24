#include "PelletCounter.hpp"
#include <optional>
#include <type_traits>

PelletCounter::PelletCounter(
    const StaticConfig &staticConfig, const Config &config
)
    : d_sensors{staticConfig, staticConfig.IRPin, staticConfig.SensorEnablePin}
    , d_config{config} {}

PelletCounter::Result PelletCounter::Process(absolute_time_t time) {
	auto newValue = d_sensors.Process(time);
	if (newValue < 0) {
		return {std::nullopt, std::nullopt, Error::PELLET_COUNTER_SENSOR_ISSUE};
	}

	if (newValue == 0) {
		return {
		    .Count       = std::nullopt,
		    .SensorValue = std::nullopt,
		};
	}

	if (d_state == false) {
		if (newValue >= d_config.SensorHighThreshold) {
			d_state = true;
			++d_count;
			return {.Count = d_count, .SensorValue = newValue};
		}
	} else {
		if (newValue <= d_config.SensorLowThreshold) {
			d_state = false;
		}
	}

	return {.Count = std::nullopt, .SensorValue = newValue};
}

bool PelletCounter::PelletPresence() const {
	return d_state;
}

uint PelletCounter::PelletCount() const {
	return d_count;
}

void PelletCounter::SetEnabled(bool enabled) {
	d_sensors.SetEnabled(enabled);
}
