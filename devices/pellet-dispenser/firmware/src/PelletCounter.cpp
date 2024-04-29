#include "PelletCounter.hpp"
#include <optional>
#include <type_traits>

PelletCounter::PelletCounter(IRSensor &sensor, const Config &config)
    : d_sensor{sensor}
    , d_config{config} {}

void PelletCounter::Process(absolute_time_t time) {
	Clear();
	auto [newValue, err] = d_sensor.Value();
	if (err != Error::NO_ERROR) {
		return;
	}

	if (newValue.has_value() == false) {
		return;
	}

	if (d_state == false) {
		if (newValue.value() >= d_config.SensorHighThreshold) {
			d_state = true;
			++d_count;
			Publish(d_count, Error::NO_ERROR);
		}
	} else {
		if (newValue <= d_config.SensorLowThreshold) {
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
	d_sensor.SetEnabled(enabled);
}
