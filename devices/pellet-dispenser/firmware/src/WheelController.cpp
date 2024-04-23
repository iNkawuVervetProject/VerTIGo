#include "WheelController.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include <cmath>
#include <memory>
#include <optional>

#include <cstdio>

WheelController::WheelController(
    const StaticConfig &staticConfig, const Config &config
)
    : d_driver{staticConfig}
    , d_sensor{staticConfig, staticConfig.SensorEnablePin}
    , d_config{config} {
	setIdle(get_absolute_time());
}

std::tuple<std::optional<int>, Error>
WheelController::Process(absolute_time_t time) {
	auto newPosition = processSensor(time);
	if (newPosition.has_value()) {
		d_position = newPosition.value();
		d_lastStep = time;
	}

	Error err = Error::NO_ERROR;

	switch (d_state) {
	case State::IDLE:
		if (absolute_time_diff_us(d_stateStart, time) >= 500000) {
			d_sensor.SetEnabled(false);
		}
		break;
	case State::RAMPING_UP: {
		if (stalled(time)) {
			err = Error::WHEEL_CONTROLLER_MOTOR_FAULT;
			if (!changeDirection(time)) {
				setIdle(time);
			}
			break;
		}

		int diff = absolute_time_diff_us(d_stateStart, time);

		if (diff >= d_config.RampUpDuration_us) {
			setMoving(time);
			break;
		}
		Channel().Set(
		    (d_direction * d_config.Speed * diff) / d_config.RampUpDuration_us
		);
		break;
	}
	case State::RAMPING_DOWN: {
		int diff = absolute_time_diff_us(d_stateStart, time);
		if (diff >= d_config.RewindPulse_us) {
			setIdle(time);
			break;
		}
		break;
	}
	case State::MOVING_TO_TARGET: {
		if (stalled(time)) {
			err = Error::WHEEL_CONTROLLER_MOTOR_FAULT;
			if (!changeDirection(time)) {
				setRampingDown(time);
			}
			break;
		}

		if (absolute_time_diff_us(d_lastStep, time) >= d_config.HighStep_us) {
			err = Error::WHEEL_CONTROLLER_SENSOR_ISSUE;
		}

		break;
	}
	}

	return {newPosition, err};
}

int WheelController::Position() {
	return d_position;
}

void WheelController::Start() {
	if (d_state != State::IDLE) {
		return;
	}
	setRampingUp(get_absolute_time());
}

void WheelController::Stop() {
	switch (d_state) {
	case State::IDLE:
		return;
	case State::RAMPING_UP:
		setIdle(get_absolute_time());
		return;
	case State::MOVING_TO_TARGET:
		setRampingDown(get_absolute_time());
		return;
	case State::RAMPING_DOWN:
		return;
	}
}

std::optional<int> WheelController::processSensor(absolute_time_t time) {
	auto newValue = d_sensor.Process(time);
	if (newValue.has_value() == false) {
		return std::nullopt;
	}

	if (d_lastState) {
		if (newValue.value() < d_config.SensorLowerThreshold) {
			d_lastState = !d_lastState;
		}
	} else {
		if (newValue.value() > d_config.SensorUpperThreshold) {
			d_lastState = !d_lastState;
			return d_position + d_direction;
		}
	}
	return std::nullopt;
}

void WheelController::setIdle(absolute_time_t time) {
	d_state = State::IDLE;
	d_driver.SetEnabled(false);
	d_stateStart      = time;
	d_directionChange = 0;
	d_lastStep        = nil_time;

	// invert the direction if too much step in one.
	if (std::abs(d_position) > d_config.StepReverseThreshold) {
		d_direction *= -1;
	}
}

void WheelController::setRampingUp(absolute_time_t time) {
	d_state      = State::RAMPING_UP;
	d_stateStart = time;
	d_lastStep   = time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	Channel().Set(0);
}

void WheelController::setRampingDown(absolute_time_t time) {
	d_state      = State::RAMPING_DOWN;
	d_stateStart = time;
	d_lastStep   = nil_time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	Channel().Set(-512 * d_direction);
}

void WheelController::setMoving(absolute_time_t time) {
	d_stateStart = time;
	d_state      = State::MOVING_TO_TARGET;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	Channel().Set(d_config.Speed * d_direction);
}

bool WheelController::changeDirection(absolute_time_t time) {
	if (d_directionChange >= 1) {
		return false;
	}
	d_direction = -1 * d_direction;
	setRampingUp(time);
	return true;
}

bool WheelController::stalled(absolute_time_t time) const {
	return d_driver.HasFault() ||
	       (d_lastStep != nil_time &&
	        absolute_time_diff_us(d_lastStep, time) >= d_config.MaxStep_us);
}
