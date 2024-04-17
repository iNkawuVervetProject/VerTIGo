#include "WheelController.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include <cmath>
#include <memory>
#include <optional>

WheelController::WheelController(const Config &config)
    : d_driver{config}
    , d_sensor{config, config.SensorEnablePin}
    , d_channel{config.UseChannelA ? d_driver.A() : d_driver.B()}
    , d_speed{config.Speed} {
	setIdle(get_absolute_time());
}

std::tuple<std::optional<int>, WheelController::Error>
WheelController::Process(absolute_time_t time) {
	auto newPosition = processSensor(time);
	if (newPosition.has_value()) {
		d_position = newPosition.value();
		d_lastStep = time;
	}

	Error err = Error::NO_ERROR;

	switch (d_state) {
	case State::IDLE:
		break;
	case State::RAMPING_UP: {
		if (stalled(time)) {
			err = Error::BLOCKED;
			if (!changeDirection(time)) {
				setIdle(time);
			}
			break;
		}

		int diff = absolute_time_diff_us(d_stateStart, time);

		if (diff >= RAMP_UP_DURATION_US) {
			setMoving(time);
			break;
		}
		d_channel.Set((d_direction * d_speed * diff) / RAMP_UP_DURATION_US);
		break;
	}
	case State::RAMPING_DOWN: {
		int diff = absolute_time_diff_us(d_stateStart, time);
		if (diff >= RAMP_DOWN_DURATION_US) {
			setIdle(time);
			break;
		}
		break;
	}
	case State::MOVING_TO_TARGET: {
		if (stalled(time)) {
			err = Error::BLOCKED;
			if (!changeDirection(time)) {
				setRampingDown(time);
			}
			break;
		}

		if (absolute_time_diff_us(d_lastStep, time) >= HIGH_STEP_TIME_US) {
			err = Error::SENSOR_ISSUE;
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

void WheelController::setIdle(absolute_time_t time) {
	d_state = State::IDLE;
	d_driver.SetEnabled(false);
	d_sensor.SetEnabled(false);
	d_stateStart      = time;
	d_directionChange = 0;
	d_lastStep        = nil_time;

	// invert the direction if too much step in one.
	if (std::abs(d_position) > STEP_THRESHOLD) {
		d_direction *= -1;
	}
}

void WheelController::setRampingUp(absolute_time_t time) {
	d_stateStart = time;
	d_lastStep   = time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	d_channel.Set(0);
}

void WheelController::setRampingDown(absolute_time_t time) {
	d_stateStart = time;
	d_lastStep   = nil_time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	d_channel.Set(-512 * d_direction);
}

void WheelController::setMoving(absolute_time_t time) {
	d_stateStart = time;
	d_driver.SetEnabled(true);
	d_sensor.SetEnabled(true);
	d_channel.Set(d_speed * d_direction);
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
	        absolute_time_diff_us(d_lastStep, time) >= MAX_STEP_TIME_US);
}
