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
#include "utils/Processor.hpp"
#include "utils/Publisher.hpp"

class IRSensor : public Processor, public Publisher<uint> {
public:
	virtual ~IRSensor() = default;

	virtual void SetEnabled(bool enabled) = 0;

	virtual bool Enabled() const = 0;
};

class BitBangIRSensor : public IRSensor {
public:
	BitBangIRSensor(uint pin, uint enablePin);

	void Process(absolute_time_t time) override;

	void SetEnabled(bool) override;

	bool Enabled() const override;

private:
	uint            d_sensorPin, d_enablePin;
	absolute_time_t d_start = nil_time;
	int8_t          d_state;
};
