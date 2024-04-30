#include "IRSensor.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include <cstdint>
#include <optional>

BitBangIRSensor::BitBangIRSensor(uint pin, uint enablePin)
    : d_sensorPin{pin}
    , d_enablePin{enablePin} {

	gpio_init(d_sensorPin);
	gpio_init(d_enablePin);
	gpio_init(16);

	gpio_set_dir(d_enablePin, true);
	gpio_set_dir(d_sensorPin, false);
	gpio_set_dir(16, true);
}

enum State {
	WAIT_FOR_START,
	PULSE,
	READ

};

void BitBangIRSensor::SetEnabled(bool value) {
	if (value == false) {
		gpio_put(d_enablePin, false);
		gpio_set_dir(d_sensorPin, false);
		gpio_put(16, 0);
	} else {
		gpio_put(d_enablePin, true);
		gpio_put(16, false);
		d_start = get_absolute_time();
		d_state = WAIT_FOR_START;
	}
}

void BitBangIRSensor::Process(const absolute_time_t time) {
	Clear();
	if (gpio_get(d_enablePin) == false) {
		return;
	}
	auto rel = absolute_time_diff_us(d_start, time);
	gpio_put(16, gpio_get(d_sensorPin));

	switch (d_state) {

	case WAIT_FOR_START:
		if (rel < 0) {
			return;
		}
		gpio_set_dir(d_sensorPin, true);
		gpio_put(d_sensorPin, true);
		d_state = PULSE;
		break;
	case PULSE:
		if (rel < 10) {
			return;
		}
		gpio_set_dir(d_sensorPin, false);
		d_state = READ;
		break;
	case READ:
		auto isLow = !gpio_get(d_sensorPin);
		if (isLow || rel >= 1000) { // either low or timeouted
			d_start = isLow ? delayed_by_ms(d_start, 1) : time;
			d_state = WAIT_FOR_START;
			if (isLow) {
				PublishValue(rel);
			} else {
				PublishError(Error::IR_SENSOR_READOUT_ERROR);
			}
		}
		break;
	}
	return;
}
