#include "Controller.hpp"
#include "Display.hpp"
#include "Error.hpp"
#include "Log.hpp"
#include "pico/types.h"

Controller::Controller(const StaticConfig &staticConfig, const Config &config)

    : d_testButton{staticConfig.TestButtonPin}
    , d_pelletCounter{staticConfig, config.Pellet}
    , d_wheelController{staticConfig, config.Wheel} {}

void Controller::Process(absolute_time_t now) {
	auto pressed = d_testButton.Process(now);

	if (pressed.has_value()) {
		auto &s = Display::State().TestButton;
		s.State = pressed.value();
		if (pressed.value() != Button::State::RELEASED) {
			s.PressCount++;
		}
		switch (pressed.value()) {
		case Button::State::PRESSED:
			Dispense(1);
			break;
		case Button::State::LONG_PRESS:
			break;
		default:
			break;
		}
	}

	auto [newPos, newSensorValue, error] = d_wheelController.Process(now);
	if (newPos.has_value()) {
		Display::State().WheelIndex = newPos.value();
		d_wheelController.Stop();
	}

	if (error != Error::NO_ERROR) {
		ErrorReporter::Report(
		    error,
		    500 * 1000
		); // 500 ms for wheel controller errors
	}

	auto pelletRes = d_pelletCounter.Process(now);

	if (pelletRes.SensorValue.has_value()) {
		auto  value = pelletRes.SensorValue.value();
		auto &state = Display::State().Pellet;
		state.Last  = value;
		state.Min   = std::min(state.Min, value);
		state.Max   = std::max(state.Max, value);
	}

	if (pelletRes.Count.has_value()) {
		Display::State().Pellet.Count = pelletRes.Count.value();
	}

	if (pelletRes.Error != Error::NO_ERROR) {
		ErrorReporter::Report(pelletRes.Error, 2 * 1000);
	}
}

void Controller::Dispense(
    uint count, const std::function<void(Error)> &callback
) {}