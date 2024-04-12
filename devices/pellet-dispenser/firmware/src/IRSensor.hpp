#pragma once

#include "pico/time.h"
#include "pico/types.h"

#include <optional>

class IRSensor {
public:
	virtual ~IRSensor() = default;

	virtual std::optional<uint32_t> Process(const absolute_time_t) = 0;

	virtual void SetEnabled(bool) = 0;
};

class BitBangIRSensor : public IRSensor {
public:
	BitBangIRSensor(uint pin, uint enablePin);

	std::optional<uint32_t> Process(const absolute_time_t time) override;

	void SetEnabled(bool) override;

private:
	uint            d_sensorPin, d_enablePin;
	absolute_time_t d_start = nil_time;
	int8_t          d_state;
};
