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

	std::optional<uint32_t> Process(const absolute_time_t time) override;

	void SetEnabled(bool) override;

private:
	uint            d_sensorPin, d_enablePin;
	absolute_time_t d_start = nil_time;
	int8_t          d_state;
};

template <size_t N, std::enable_if_t<N >= 1, void> * = nullptr>
class PIOIRSensor : public IRSensor {
public:
	template <
	    typename... T,
	    std::enable_if_t<sizeof...(T) == N, void> * = nullptr>
	PIOIRSensor(PIO pio, uint sensorPin, T &&...enablePins)
	    : d_pio{pio}
	    , d_enablePins{std::forward<uint>(enablePins...)} {

		d_sm = pio_claim_unused_sm(d_pio, true);

		auto offset = pio_add_program(pio, &ir_sensor_program);
		ir_sensor_program_configure(d_pio, d_sm, 10, 1000);

		for (auto p : d_enablePins) {
			gpio_init(p);
			gpio_put(p, 0);
		}
	}

private:
	PIO  d_pio;
	uint d_sm;

	std::array<uint, N> d_enablePins;
};
