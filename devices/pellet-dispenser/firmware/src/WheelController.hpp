#pragma once

#include "DRV8848.hpp"
#include "PIOIRSensor.hpp"
#include "pico/types.h"

class WheelController {
public:
	struct Config : public DRV8848::Config, public PIOIRSensor<1>::Config {
		uint SensorEnablePin;
		int  Speed = 200;
	};

	WheelController(const Config &config);

	void Process(absolute_time_t time);

	int  Position();
	void Move(int wanted);
	void Stop();

	bool Stalled() const;

private:
	enum class State {
		IDLE = 0,
		RAMPING_UP,
		MOVING_TO_TARGET,
		RAMPING_DOWN,
	};

	void SetIdle(absolute_time_t);
	void SetRampingUp(absolute_time_t time);
	void SetRampingDown(absolute_time_t time);
	void SetMoving(absolute_time_t time);

	std::optional<int> ProcessSensor(absolute_time_t time);

	DRV8848        d_driver;
	PIOIRSensor<1> d_sensor;
	State          d_state = State::IDLE;
	int            d_speed;
	int            d_direction = 1;
	bool           d_lastState = false;

	int             d_position = -1, d_wanted = 0;
	absolute_time_t d_rampStart;
};
