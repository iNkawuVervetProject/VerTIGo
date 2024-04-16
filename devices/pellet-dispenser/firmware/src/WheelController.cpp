#include "WheelController.hpp"
#include "pico/time.h"
#include "pico/types.h"

#include <optional>

#define RAMP_UP_DURATION_US    10000
#define RAMP_DOWN_DURATION_US  18000
#define SENSOR_LOWER_THRESHOLD 160
#define SENSOR_UPPER_THRESHOLD 220

#include <cstdio>

WheelController::WheelController(const Config &config)
    : d_driver{config}
    , d_sensor{config, config.SensorEnablePin}
    , d_speed{config.Speed} {
	SetIdle(get_absolute_time());
	Move(1);
}

void WheelController::Process(absolute_time_t time) {
	auto newPosition = ProcessSensor(time);
	if (newPosition.has_value()) {
		d_position = newPosition.value();
	}

	switch (d_state) {
	case State::IDLE:
		break;
	case State::RAMPING_UP: {
		if (newPosition.has_value() && d_position == d_wanted) {
			SetIdle(time);
			break;
		}
		int diff = absolute_time_diff_us(d_rampStart, time);

		if (diff >= RAMP_UP_DURATION_US) {
			SetMoving(time);
			break;
		}
		d_driver.SetChannelB(
		    (d_direction * d_speed * diff) / RAMP_UP_DURATION_US
		);
		break;
	}
	case State::RAMPING_DOWN: {
		int diff = absolute_time_diff_us(d_rampStart, time);
		if (diff >= RAMP_DOWN_DURATION_US) {
			SetIdle(time);
			break;
		}
		break;
	}
	case State::MOVING_TO_TARGET: {
		if (newPosition.has_value() && d_position == d_wanted) {
			SetRampingDown(time);
		}
		break;
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
	SetRampingUp(get_absolute_time());
}

void WheelController::Stop() {
	switch (d_state) {
	case State::IDLE:
		return;
	case State::RAMPING_UP:
		SetIdle(get_absolute_time());
		return;
	case State::MOVING_TO_TARGET:
		SetRampingDown(get_absolute_time());
		return;
	case State::RAMPING_DOWN:
		return;
	}
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

void WheelController::SetIdle(absolute_time_t time) {
	d_state = State::IDLE;
	d_driver.SetEnabled(false);
	d_sensor.SetEnabled(false);
	d_rampStart = time;
}

void WheelController::SetRampingUp(absolute_time_t time) {
	d_rampStart = time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	d_driver.SetChannelB(0);
}

void WheelController::SetRampingDown(absolute_time_t time) {
	d_rampStart = time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	d_driver.SetChannelB(-512 * d_direction);
}

void WheelController::SetMoving(absolute_time_t time) {
	d_rampStart = time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	d_driver.SetChannelB(d_speed * d_direction);
}
