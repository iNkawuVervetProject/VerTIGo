#pragma once

#include "pico/types.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>

#include "hardware/PIOIRSensor.hpp"
#include "utils/Processor.hpp"
#include "utils/Publisher.hpp"

class PelletCounter : public Processor, public Publisher<int> {
public:
	struct Config {
		uint SensorLowThreshold  = 60;
		uint SensorHighThreshold = 250;
	};

	PelletCounter(IRSensor &sensor, const Config &config);

	void Process(absolute_time_t time) override;

	void SetEnabled(bool enabled);

	bool PelletPresence() const;

	uint PelletCount() const;

private:
	IRSensor &d_sensor;

	const Config &d_config;
	bool          d_state     = false;
	uint          d_count     = 0;
	uint          d_lastValue = 0;
};
