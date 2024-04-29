#include "Controller.hpp"
#include "Display.hpp"
#include "Error.hpp"
#include "Log.hpp"
#include "hardware/gpio.h"
#include "pico/types.h"

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

class ModeBase : public Mode {
public:
	ModeBase(Controller &controller)
	    : Mode(controller){};

	virtual std::unique_ptr<Mode> operator()(absolute_time_t) override {

		return nullptr;
	}

}

Controller::Controller(const StaticConfig &staticConfig, const Config &config)

    : d_button{staticConfig.TestButton}
    , d_wheelSensor{staticConfig.WheelSensor}
    , d_pelletSensor{staticConfig.PelletSensor}
    , d_wheel{staticConfig.Wheel}
    , d_counter{staticConfig.Counter} {
}

Controller::~Controller() = default;

void Controller::Process(absolute_time_t now) {
	auto newMode = (*d_mode)(now);
	if (newMode) {
		d_mode = newMode;
	}
}

void Controller::Dispense(
    uint count, const std::function<void(Error)> &callback
) {}
