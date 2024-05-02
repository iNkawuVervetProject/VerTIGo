#pragma once

#include <cstddef>
enum class Error {
	NO_ERROR = 0,
	IR_SENSOR_READOUT_ERROR,
	WHEEL_CONTROLLER_MOTOR_FAULT,
	WHEEL_CONTROLLER_SENSOR_ISSUE,
	WHEEL_CONTROLLER_SENSOR_IRRESPONSIVE,
	PELLET_COUNTER_SENSOR_ISSUE,
	DISPENSER_EMPTY,
	DISPENSER_JAMMED,
	DISPENSER_QUEUE_FULL,
	DISPENSER_SELF_CHECK_FAIL,
	DISPENSER_CALIBRATION_ERROR,
	_NUM_ERRORS
};

constexpr static size_t NumErrors = size_t(Error::_NUM_ERRORS);

static const char *ErrorDescription[NumErrors] = {
    "No error",
    "IRSensor: readout error",
    "WheelController: motor is stalled",
    "WheelController: sensor issue",
    "WheelController: sensor seems irresponsive",
    "PelletCounter: sensor issue",
    "Dispenser: empty",
    "Dispenser: jammed",
    "Dispenser: too many commands",
    "Dispenser: self-test failed",
    "Dispenser: calibration error",
};

inline static const char *GetErrorDescription(Error err) {
	size_t index = size_t(err);
	if (index >= NumErrors) {
		return "Unknown Error Code";
	}
	return ErrorDescription[index];
}
