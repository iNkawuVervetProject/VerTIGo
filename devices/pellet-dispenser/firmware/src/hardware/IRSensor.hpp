#pragma once

#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pico/time.h"
#include "pico/types.h"

#include <array>
#include <initializer_list>
#include <optional>
#include <type_traits>

#include "../Error.hpp"
#include "ir_sensor.pio.h"

class IRSensor {
public:
	virtual ~IRSensor() = default;

	virtual int Process(const absolute_time_t) = 0;

	virtual void SetEnabled(bool) = 0;
};

class BitBangIRSensor : public IRSensor {
public:
	BitBangIRSensor(uint pin, uint enablePin);

	int Process(absolute_time_t time) override;

	void SetEnabled(bool) override;

private:
	uint            d_sensorPin, d_enablePin;
	absolute_time_t d_start = nil_time;
	int8_t          d_state;
};
