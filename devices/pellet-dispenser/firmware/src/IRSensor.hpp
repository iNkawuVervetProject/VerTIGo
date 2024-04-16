#pragma once

#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "ir_sensor.pio.h"
#include "pico/time.h"
#include "pico/types.h"

#include <array>
#include <initializer_list>
#include <optional>
#include <type_traits>

class IRSensor {
public:
	virtual ~IRSensor() = default;

	virtual std::optional<uint32_t> Process(const absolute_time_t) = 0;

	virtual void SetEnabled(bool) = 0;
};

class BitBangIRSensor : public IRSensor {
public:
	BitBangIRSensor(uint pin, uint enablePin);

	std::optional<uint32_t> Process(absolute_time_t time) override;

	void SetEnabled(bool) override;

private:
	uint            d_sensorPin, d_enablePin;
	absolute_time_t d_start = nil_time;
	int8_t          d_state;
};