#pragma once

#include "DRV8848.hpp"
#include "Error.hpp"
#include "PIOIRSensor.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include <optional>
#include <tuple>

class WheelController {
public:
	const static int  RAMP_UP_DURATION_US    = 10000;
	const static uint SENSOR_LOWER_THRESHOLD = 160;
	const static uint SENSOR_UPPER_THRESHOLD = 220;
	const static int  HIGH_STEP_TIME_US      = 200 * 1000;      //
	const static int  MAX_STEP_TIME_US       = 5 * 1000 * 1000; // 5s

	const static int STEP_THRESHOLD = 20;

	struct Config : public DRV8848::Config, public PIOIRSensor<1>::Config {
		uint SensorEnablePin;
		int  Speed              = 200;
		int  RampDownDurationUS = 20000;
		bool UseChannelA        = false;
	};

	WheelController(const Config &config);
	~WheelController();

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

	bool stalled(absolute_time_t time) const;

	void setIdle(absolute_time_t);
	void setRampingUp(absolute_time_t time);
	void setRampingDown(absolute_time_t time);
	void setMoving(absolute_time_t time);

	bool changeDirection(absolute_time_t time);

	std::optional<int> processSensor(absolute_time_t time);

	DRV8848                 d_driver;
	PIOIRSensor<1>          d_sensor;
	const DRV8848::Channel &d_channel;

	State d_state = State::IDLE;
	int   d_speed;
	int   d_rampDownDurationUS;

	int   d_direction       = 1;
	bool  d_lastState       = false;
	int   d_directionChange = 0;

	int             d_position   = -1;
	absolute_time_t d_stateStart = nil_time;
	absolute_time_t d_lastStep   = nil_time;
};
