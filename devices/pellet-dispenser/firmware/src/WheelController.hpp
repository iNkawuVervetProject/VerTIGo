#pragma once

#include "hardware/IRSensor.hpp"
#include "pico/time.h"
#include "pico/types.h"

#include <optional>
#include <tuple>

#include "hardware/DRV8848.hpp"
#include "hardware/PIOIRSensor.hpp"

#include "Error.hpp"
#include "utils/Processor.hpp"
#include "utils/Publisher.hpp"

class WheelController : public Processor, public Publisher<int> {
public:
	struct Config {
		DRV8848::OutputChannel Channel = DRV8848::OutputChannel::A;

		uint Speed             = 512;
		uint RampUpDuration_us = 30 * 1000;
		uint RewindPulse_us    = 17 * 1000;
		uint SensorCooldown_us = 1200 * 1000;

		uint SensorLowerThreshold = 160;
		uint SensorUpperThreshold = 220;

		uint HighStep_us = 400 * 1000;
		uint MaxStep_us  = 1200 * 1000;
	} __attribute__((packed));

	WheelController(DRV8848 &motor, IRSensor &sensor, const Config &config);

	struct Result {
		std::optional<int>  Position;
		std::optional<uint> SensorValue;
		enum Error          Error;
	};

	void Process(absolute_time_t time) override;

	int Position();

	void Start(int direction = 1);
	void Stop();

	inline bool Ready() const {
		return d_state == State::IDLE;
	}

	inline bool WheelAligned() const {
		return d_aligned;
	}

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

	std::pair<std::optional<int>, Error> processSensor(absolute_time_t time);

	const Config  &d_config;
	DRV8848       &d_driver;
	IRSensor      &d_sensor;

	State d_state = State::IDLE;

	int  d_direction = 1;
	bool d_aligned   = false;

	int             d_position   = 0;
	absolute_time_t d_stateStart = nil_time;
	absolute_time_t d_lastStep   = nil_time;
	absolute_time_t d_sensorStop = nil_time;
};
