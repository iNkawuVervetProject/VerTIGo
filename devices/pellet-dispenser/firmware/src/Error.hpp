#pragma once

#include "pico/time.h"
#include "pico/types.h"
#include <array>
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
	_NUM_ERRORS
};

constexpr static size_t NumErrors = size_t(Error::_NUM_ERRORS);

static const char *ErrorDescription[NumErrors] = {
    "No error",
    "IRSensor: Readout Error",
    "WheelController: Motor is stalled",
    "WheelController: Sensor issue",
    "WheelController: Sensor seems irresponsive",
    "PelletSensor: Sensor issue",
    "Dispenser: Empty",
    "Dispenser: Jammed",
    "Dispenser: too many commands",
    "Dispenser: self-test failed",
};

inline static const char *GetErrorDescription(Error err) {
	size_t index = size_t(err);
	if (index >= NumErrors) {
		return "Unknown Error Code";
	}
	return ErrorDescription[index];
}

class ErrorReporter {
public:
	inline static ErrorReporter &Get() {
		static ErrorReporter reporter;
		return reporter;
	}

	static inline void Report(Error err, uint timeout_us) {
		Get().report(err, timeout_us);
	};

	void Process(absolute_time_t time);

private:
	void report(Error e, uint timeout_us);

	std::array<absolute_time_t, NumErrors> d_firedErrors;

	absolute_time_t d_next = nil_time;
};
