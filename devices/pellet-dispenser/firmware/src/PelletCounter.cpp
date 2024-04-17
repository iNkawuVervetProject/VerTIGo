#include "PelletCounter.hpp"
#include <optional>

PelletCounter::PelletCounter(const Config &config)
    : d_sensors{config, config.IRPin, config.SensorEnablePin} {}

std::optional<uint> PelletCounter::Process(absolute_time_t time) {
	auto newValue = d_sensors.Process(time);

	if (newValue.has_value() == false) {
		return std::nullopt;
	}

	if (d_state == false) {
		if (newValue.value() >= THRESHOLD_UP) {
			d_state = true;
			++d_count;
			return d_count;
		}
	} else {
		if (newValue.value() <= THRESHOLD_DOWN) {
			d_state = false;
		}
	}
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
