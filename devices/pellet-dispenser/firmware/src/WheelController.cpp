#include "WheelController.hpp"

#include "Log.hpp"
#include "hardware/DRV8848.hpp"
#include "hardware/IRSensor.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <optional>

WheelController::WheelController(
    DRV8848 &motor, IRSensor &sensor, const Config &config
)
    : d_driver{motor}
    , d_sensor{sensor}
    , d_config{config} {
	setIdle(get_absolute_time());
}

void WheelController::Process(absolute_time_t time) {
	Clear();
	auto [newPosition, err] = processSensor(time);

	if (newPosition.has_value()) {
		d_position = newPosition.value();
		d_lastStep = time;
	}

	switch (d_state) {
	case State::IDLE:
		if (absolute_time_diff_us(d_stateStart, time) >=
		    d_config.SensorCooldown_us) {
			d_sensor.SetEnabled(false);
		}
		break;
	case State::RAMPING_UP: {
		if (stalled(time)) {
			err = Error::WHEEL_CONTROLLER_MOTOR_FAULT;
			setIdle(time);
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
			setIdle(time);
			break;
		}

		if (absolute_time_diff_us(d_lastStep, time) >= d_config.HighStep_us) {
			err = Error::WHEEL_CONTROLLER_SENSOR_IRRESPONSIVE;
		}

		break;
	}
	}

	if (err != Error::NO_ERROR) {
		PublishError(err);
	} else if (newPosition.has_value()) {
		PublishValue(newPosition.value());
	}
}

int WheelController::Position() {
	return d_position;
}

void WheelController::Start(int direction) {
	if (d_state != State::IDLE) {
		return;
	}
	d_direction = direction > 0 ? 1 : -1;
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

std::pair<std::optional<int>, Error>
WheelController::processSensor(absolute_time_t time) {

	if (d_sensor.HasError() || d_sensor.HasValue() == false) {
		return {std::nullopt, Error::NO_ERROR};
	}

	auto value = d_sensor.Value();
	if (d_lastState == true) {
		if (value < d_config.SensorLowerThreshold) {
			d_lastState = false;
		}
	} else {
		if (value > d_config.SensorUpperThreshold) {
			d_lastState = true;
			return {d_position + d_direction, Error::NO_ERROR};
		}
	}
	return {std::nullopt, Error::NO_ERROR};
}

void WheelController::setIdle(absolute_time_t time) {
	d_state = State::IDLE;
	d_driver.SetEnabled(false);
	d_stateStart = time;
	d_lastStep   = nil_time;
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

bool WheelController::stalled(absolute_time_t time) const {
	return d_driver.HasFault() ||
	       (d_lastStep != nil_time &&
	        absolute_time_diff_us(d_lastStep, time) >= d_config.MaxStep_us);
}
