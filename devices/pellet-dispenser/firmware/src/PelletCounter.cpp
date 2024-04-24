#include "PelletCounter.hpp"
#include <optional>
#include <type_traits>

PelletCounter::PelletCounter(
    const StaticConfig &staticConfig, const Config &config
)
    : d_sensors{staticConfig, staticConfig.IRPin, staticConfig.SensorEnablePin}
    , d_config{config} {}

std::tuple<std::optional<uint>, std::optional<uint>>
PelletCounter::Process(absolute_time_t time) {
	auto newValue = d_sensors.Process(time);

	if (newValue.has_value() == false) {
		return std::make_tuple(std::nullopt, std::nullopt);
	}

	if (d_state == false) {
		if (newValue.value() >= d_config.SensorHighThreshold) {
			d_state = true;
			++d_count;
			return {d_count, newValue};
		}
	} else {
		if (newValue.value() <= d_config.SensorLowThreshold) {
			d_state = false;
		}
	}

	return {std::nullopt, newValue};
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
