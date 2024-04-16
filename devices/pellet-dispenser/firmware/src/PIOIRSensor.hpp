#pragma once

#include "IRSensor.hpp"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/pio_instructions.h"
#include "ir_sensor.pio.h"
#include "pico/types.h"
#include <optional>

template <size_t N, std::enable_if_t<N >= 1, void> * = nullptr>
class PIOIRSensor : public IRSensor {
public:
	struct Config {
		PIO      Pio = pio0;
		uint     SensorPin;
		uint16_t PulseUS  = 10;
		uint16_t PeriodUS = 1000;
	};

	template <
	    typename... T,
	    std::enable_if_t<sizeof...(T) == N, void> * = nullptr>
	PIOIRSensor(const Config &config, T &&...enablePins)
	    : d_pio{config.Pio}
	    , d_pin{config.SensorPin}
	    , d_enablePins{std::forward<uint>(enablePins...)}
	    , d_pulse{config.PulseUS}
	    , d_period{config.PeriodUS} {

		d_sm = pio_claim_unused_sm(d_pio, true);

		d_offset = pio_add_program(d_pio, &ir_sensor_program);
		ir_sensor_program_init(d_pio, d_sm, d_offset, d_pin);
		ir_sensor_program_configure(d_pio, d_sm, d_pulse, d_period);

		for (auto p : d_enablePins) {
			gpio_init(p);
			gpio_set_dir(p, true);
			gpio_put(p, 0);
		}
	}

	std::optional<uint32_t> Process(absolute_time_t time) override {
		uint32_t res = 0xffffffff;
		// drain all readings
		while (pio_sm_is_rx_fifo_empty(d_pio, d_sm) == false) {
			res = pio_sm_get_blocking(d_pio, d_sm);
		}
		if (res == 0xffffffff) {
			return std::nullopt;
		}
		return d_period - d_pulse - res;
	}

	void SetEnabled(bool enabled) override {
		if (gpio_get(d_enablePins[0]) == enabled) {
			return;
		}
		if (enabled) {
			pio_sm_set_pindirs_with_mask(d_pio, d_sm, 0, (1u << d_pin));
			pio_sm_exec_wait_blocking(d_pio, d_sm, pio_encode_jmp(d_offset));
		}
		for (auto p : d_enablePins) {
			gpio_put(p, enabled);
		}
		pio_sm_set_enabled(d_pio, d_sm, enabled);
	}

private:
	PIO  d_pio;
	uint d_sm;
	uint d_pin;
	uint d_offset;

	std::array<uint, N> d_enablePins;

	uint16_t d_pulse, d_period;
};
