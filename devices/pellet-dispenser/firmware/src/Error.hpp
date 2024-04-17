#pragma once

#include <cstddef>
enum class Error {
	NO_ERROR = 0,
	WHEEL_CONTROLLER_MOTOR_FAULT,
	WHEEL_CONTROLLER_SENSOR_ISSUE,
	_NUM_ERRORS
};

constexpr static size_t NumErrors = size_t(Error::_NUM_ERRORS);

static const char *ErrorDescription[NumErrors] = {
    "No error",
    "WheelController: Motor is stalled",
    "WheelController: Sensor issue",
};

inline static const char *GetErrorDescription(Error err) {
	size_t index = size_t(err);
	if (index >= NumErrors) {
		return "Unknown Error Code";
	}
	return ErrorDescription[index];
}
