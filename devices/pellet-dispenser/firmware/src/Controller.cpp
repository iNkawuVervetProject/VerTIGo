#include "Controller.hpp"
#include "Display.hpp"
#include "Error.hpp"
#include "Log.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include "utils/Publisher.hpp"
#include <memory>

class Mode {
public:
	inline Mode(Controller &controller)
	    : d_controller{controller} {}

	virtual ~Mode() = default;

	virtual std::unique_ptr<Mode> operator()(absolute_time_t) = 0;

protected:
	inline Controller &Self() const {
		return d_controller;
	}

	Controller &d_controller;
};

class IdleMode : public Mode {
private:
	absolute_time_t d_start;

public:
	inline IdleMode(Controller &controller, absolute_time_t start)
	    : Mode{controller}
	    , d_start{start} {}

	std::unique_ptr<Mode> operator()(absolute_time_t now) override;
};

class SelfCheckMode : public Mode {
private:
	bool d_pelletTested = false;
	bool d_wheelTested  = false;
	bool d_pelletGood   = false;
	bool d_wheelGood    = false;

public:
	inline SelfCheckMode(Controller &controller)
	    : Mode{controller} {
		Self().d_pelletSensor.SetEnabled(true);
		Self().d_wheelSensor.SetEnabled(true);
	}

	std::unique_ptr<Mode> operator()(absolute_time_t now) override {
		if (Self().d_pelletSensor.HasError() ||
		    Self().d_pelletSensor.HasValue()) {
			d_pelletTested = true;
			d_pelletGood   = !Self().d_pelletSensor.HasError();
			Self().d_pelletSensor.SetEnabled(false);
		}

		if (Self().d_wheelSensor.HasError() ||
		    Self().d_wheelSensor.HasValue()) {

			d_wheelTested = true;
			d_wheelGood   = !Self().d_wheelSensor.HasError();
			Self().d_wheelSensor.SetEnabled(false);
		}

		if (d_pelletTested && d_wheelTested) {
			// TODO : report Sensor issues
			return std::make_unique<IdleMode>(Self(), now);
		}
		return nullptr;
	}
};

class DispenseMode: public Mode {
private:
	uint d_want,d_startCount;
	int d_startPosition;
	int d_direction;

	uint d_positionTravelled = 0;
public:
	inline DispenseMode(Controller &controller, uint want)
	    : Mode{controller}
	    , d_want{want}
	    , d_startCount{controller.d_counter.PelletCount()}
	    , d_startPosition{controller.d_wheel.Position()} {

		static int direction = 1;
		if (direction == 1 && d_startPosition >= 20 ) {
			direction = -1;
				}else if (direction == -1 && d_startPosition <= -20 ) {
			direction = 1;
		}
		d_direction = direction;

		Self().d_pelletSensor.SetEnabled(true);
		Self().d_wheel.Start(d_direction);
	}

	~DispenseMode() {
		Self().d_wheel.Stop();
	}

	std::unique_ptr<Mode> operator()(absolute_time_t now) override {
		if (Self().d_wheel.HasValue()) {
			d_positionTravelled++;
		}

		if (Self().d_counter.HasValue()) {
			if ((Self().d_counter.Value() - d_startCount) >= d_want) {
				// TODO report fine
				return std::make_unique<IdleMode>(Self(), now);
			}
		}

		if (d_positionTravelled >= 3 * d_want) {
			// TODO report empty:
			return
		}
	}

	std::unique_ptr<Mode> IdleMode::operator()(absolute_time_t now) override {
		auto ellapsed = absolute_time_diff_us(d_start, now);
		if (ellapsed >= 5 * 1000 * 1000) {
			Self().d_counter.SetEnabled(false);
		}

		if (ellapsed >= 60 * 1000 * 1000) {
			return std::make_unique<SelfCheckMode>(Self());
		}

		// TODO process in commands

		return nullptr;
}

Controller::Controller(const StaticConfig &staticConfig, const Config &config)

    : d_button{staticConfig.TestButton}
    , d_wheelSensor{staticConfig.WheelSensor}
    , d_pelletSensor{staticConfig.PelletSensor}
    , d_wheel{staticConfig.Wheel}
    , d_counter{staticConfig.Counter} {}

Controller::~Controller() = default;

void Controller::Process(absolute_time_t now) {
	processErrors();
	auto newMode = (*d_mode)(now);
	if (newMode) {
		d_mode = std::move(newMode);
	}
	}

	void Controller::Dispense(
	    uint                              count,
	    const std::function<void(Error)> &callback
	) {}

	void Controller::processErrors() {
		if (d_wheelSensor.HasError()) {
			ErrorReporter::Get().Report(
			    Error::WHEEL_CONTROLLER_SENSOR_ISSUE,
			    10 * 1000
			);
		}

		if (d_pelletSensor.HasError()) {
			ErrorReporter::Get().Report(
			    Error::PELLET_COUNTER_SENSOR_ISSUE,
			    10 * 1000
			);
		}

		if (d_wheel.HasError()) {
			ErrorReporter::Get().Report(d_wheel.Err(), 5 * 1000 * 1000);
		}

		if (d_counter.HasError()) {
			ErrorReporter::Get().Report(d_counter.Err(), 5 * 1000 * 1000);
		}
	}
