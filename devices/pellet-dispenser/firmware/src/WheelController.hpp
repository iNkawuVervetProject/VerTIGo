#pragma once

#include "pico/time.h"
#include "pico/types.h"

#include <optional>
#include <tuple>

#include "hardware/DRV8848.hpp"
#include "hardware/PIOIRSensor.hpp"

#include "Error.hpp"

class WheelController {
public:
	struct StaticConfig : public DRV8848::Config,
	                      public ::PIOIRSensor<1>::Config {
		uint SensorEnablePin = -1;
	};

	struct Config {
		DRV8848::OutputChannel Channel = DRV8848::OutputChannel::A;

		uint Speed             = 200;
		uint RampUpDuration_us = 10000;
		uint RewindPulse_us    = 20000;

		uint SensorLowerThreshold = 160;
		uint SensorUpperThreshold = 220;

		uint HighStep_us = 200 * 1000;
		uint MaxStep_us  = 2 * 1000 * 1000;

		uint StepReverseThreshold = 20;
	};

	WheelController(const StaticConfig &staticConfig, const Config &config);

	std::tuple<std::optional<int>, Error> Process(absolute_time_t time);

	int Position();

	void Start();
	void Stop();

private:
	enum class State {
		IDLE = 0,
		RAMPING_UP,
		MOVING_TO_TARGET,
		RAMPING_DOWN,
	};

	inline const class DRV8848::Channel &Channel() const {
		return d_driver.Channel(d_config.Channel);
	}

	bool stalled(absolute_time_t time) const;

	void setIdle(absolute_time_t);
	void setRampingUp(absolute_time_t time);
	void setRampingDown(absolute_time_t time);
	void setMoving(absolute_time_t time);

	bool changeDirection(absolute_time_t time);

	std::tuple<std::optional<int>, Error> processSensor(absolute_time_t time);

	const Config  &d_config;
	DRV8848        d_driver;
	PIOIRSensor<1> d_sensor;

	State d_state = State::IDLE;

	int  d_direction       = 1;
	bool d_lastState       = false;
	int  d_directionChange = 0;

	int             d_position   = -1;
	absolute_time_t d_stateStart = nil_time;
	absolute_time_t d_lastStep   = nil_time;
};
