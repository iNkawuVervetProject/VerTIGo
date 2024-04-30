#include "PelletCounter.hpp"
#include <optional>
#include <type_traits>

PelletCounter::PelletCounter(IRSensor &sensor, const Config &config)
    : d_sensor{sensor}
    , d_config{config} {}

void PelletCounter::Process(absolute_time_t time) {
	Clear();
	if (d_sensor.HasError() || d_sensor.HasValue() == false) {
		return;
	}

	auto value = d_sensor.Value();

	if (d_state == false) {
		if (value >= d_config.SensorHighThreshold) {
			d_state = true;
			++d_count;
			PublishValue(d_count);
		}
	} else {
		if (value <= d_config.SensorLowThreshold) {
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
