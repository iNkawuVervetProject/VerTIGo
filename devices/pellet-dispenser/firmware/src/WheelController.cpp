#include "WheelController.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include <cmath>
#include <optional>

#define RAMP_UP_DURATION_US    10000
#define RAMP_DOWN_DURATION_US  18000
#define SENSOR_LOWER_THRESHOLD 160
#define SENSOR_UPPER_THRESHOLD 220
#define MAX_STEP_TIME_US       400000

WheelController::WheelController(const Config &config)
    : d_driver{config}
    , d_sensor{config, config.SensorEnablePin}
    , d_speed{config.Speed} {
	SetIdle(get_absolute_time());
	Move(1);
}

std::tuple<std::optional<int>, WheelController::Error>
WheelController::Process(absolute_time_t time) {
	auto newPosition = ProcessSensor(time);
	if (newPosition.has_value()) {
		d_position = newPosition.value();
		d_lastStep = time;
	}

	switch (d_state) {
	case State::IDLE:
		break;
	case State::RAMPING_UP: {
		if (Stalled(time)) {
			if (d_directionChange == 0) {
				if (!ChangeDirection(time)) {
					SetIdle(time);
					return {std::nullopt, Error::BLOCKED};
				}
			}
			break;
		}

		if (newPosition.has_value() && d_position == d_wanted) {
			SetIdle(time);
			break;
		}

		int diff = absolute_time_diff_us(d_stateStart, time);

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
		int diff = absolute_time_diff_us(d_stateStart, time);
		if (diff >= RAMP_DOWN_DURATION_US) {
			SetIdle(time);
			break;
		}
		break;
	}
	case State::MOVING_TO_TARGET: {
		if (Stalled(time)) {
			if (!ChangeDirection(time)) {
				SetRampingDown(time);
				return {std::nullopt, Error::BLOCKED};
			}
			break;
		}

		if (newPosition.has_value()) {
			if (d_position == d_wanted) {
				SetRampingDown(time);
			}
		}
		break;
	}
	}

	return {newPosition, Error::NO_ERROR};
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

bool WheelController::Stalled(absolute_time_t time) const {
	return d_driver.HasFault() ||
	       absolute_time_diff_us(d_stateStart, time) > MAX_STEP_TIME_US;
}

void WheelController::SetIdle(absolute_time_t time) {
	d_state = State::IDLE;
	d_driver.SetEnabled(false);
	d_sensor.SetEnabled(false);
	d_stateStart = time;

	// invert the direction if too much step in one.
	if (std::abs(d_position) > 20) {
		d_direction *= -1;
	}
}

void WheelController::SetRampingUp(absolute_time_t time) {
	d_stateStart      = time;
	d_lastStep        = time;
	d_directionChange = 0;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	d_driver.SetChannelB(0);
}

void WheelController::SetRampingDown(absolute_time_t time) {
	d_stateStart = time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	d_driver.SetChannelB(-512 * d_direction);
}

void WheelController::SetMoving(absolute_time_t time) {
	d_stateStart = time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	d_driver.SetChannelB(d_speed * d_direction);
}

bool WheelController::ChangeDirection(absolute_time_t time) {
	if (d_directionChange >= 1) {
		return false;
	}
	d_direction = -1 * d_direction;
	auto needed = std::abs(d_wanted - d_position);
	d_wanted    = d_position + d_direction * needed;
	SetRampingUp(time);
	return true;
}
