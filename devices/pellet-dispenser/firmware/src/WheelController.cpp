#include "WheelController.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include <optional>

#define RAMP_DURATION_US       1000000
#define SENSOR_LOWER_THRESHOLD 160
#define SENSOR_UPPER_THRESHOLD 220

#include <cstdio>

WheelController::WheelController(const Config &config)
    : d_driver{config}
    , d_sensor{config, config.SensorEnablePin}
    , d_speed{config.Speed} {
	Move(1);
}

void WheelController::Process(absolute_time_t time) {
	auto newPosition = ProcessSensor(time);
	if (newPosition.has_value()) {
		d_position = newPosition.value();
	}

	switch (d_state) {
	case State::IDLE:
		d_driver.SetEnabled(false);
		d_sensor.SetEnabled(false);
		return;
	case State::RAMPING_UP: {
		if (newPosition.has_value() && d_position == d_wanted) {
			d_state     = State::RAMPING_DOWN;
			d_rampStart = time;
			return;
		}
		int diff = absolute_time_diff_us(d_rampStart, time);

		if (diff >= RAMP_DURATION_US) {
			d_driver.SetChannelB(d_direction * d_speed);
			d_state     = State::MOVING_TO_TARGET;
			d_rampStart = time;
			return;
		}
		d_driver.SetChannelB((d_direction * d_speed * diff) / RAMP_DURATION_US);
		return;
	}
	case State::RAMPING_DOWN: {
		int diff = absolute_time_diff_us(d_rampStart, time);
		if (diff >= RAMP_DURATION_US) {
			d_driver.SetChannelB(0);
			d_state     = State::IDLE;
			d_rampStart = time;
			return;
		}
		d_driver.SetChannelB(
		    (d_direction * d_speed * (RAMP_DURATION_US - diff)) /
		    RAMP_DURATION_US
		);
		return;
	}
	case State::MOVING_TO_TARGET: {
		if (newPosition.has_value() && d_position == d_wanted) {
			d_state     = State::RAMPING_DOWN;
			d_rampStart = time;
		}
		return;
	}
	}
}

int WheelController::Position() {
	return d_position;
}

void WheelController::Move(int i) {
	if (d_state != State::IDLE) {
		return;
	}

	d_wanted = d_position + d_direction * i;
	d_sensor.SetEnabled(true);
	d_driver.SetEnabled(true);
	d_state     = State::RAMPING_UP;
	d_rampStart = get_absolute_time();
}

std::optional<int> WheelController::ProcessSensor(absolute_time_t time) {
	auto newValue = d_sensor.Process(time);
	if (newValue.has_value() == false) {
		return std::nullopt;
	}

	if (d_lastState) {
		if (newValue.value() < SENSOR_LOWER_THRESHOLD) {
			d_lastState = !d_lastState;
		}
	} else {
		if (newValue.value() > SENSOR_UPPER_THRESHOLD) {
			d_lastState = !d_lastState;
			return d_position + d_direction;
		}
	}
	return std::nullopt;
}

bool WheelController::Stalled() const {
	return d_driver.HasFault();
}
