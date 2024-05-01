#include "DispenserController.hpp"
#include "Display.hpp"
#include "Error.hpp"
#include "Log.hpp"
#include "PelletCounter.hpp"
#include "WheelController.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include "utils/Publisher.hpp"
#include <algorithm>
#include <memory>
#include <sstream>

class Mode {
public:
	inline Mode(DispenserController &controller)
	    : d_controller{controller} {}

	virtual ~Mode() = default;

	virtual std::unique_ptr<Mode> operator()(absolute_time_t) = 0;

protected:
	inline DispenserController &Self() const {
		return d_controller;
	}

	DispenserController &d_controller;
};

class IdleMode : public Mode {
private:
	absolute_time_t d_start;

	std::function<void(uint)> d_onCounterDisable;

	const DispenserController::Config &d_config;

	inline static void Noop(uint) {}

public:
	inline IdleMode(
	    DispenserController      &controller,
	    absolute_time_t           start,
	    std::function<void(uint)> onCounterDisable = Noop
	)
	    : Mode{controller}
	    , d_start{start}
	    , d_onCounterDisable{onCounterDisable}
	    , d_config{controller.d_config} {
		Infof("Dispenser: ready");
	}

	~IdleMode() {
		d_onCounterDisable(Self().d_counter.PelletCount());
	}

	std::unique_ptr<Mode> operator()(absolute_time_t now) override;
};

class SelfCheckMode : public Mode {
private:
	bool d_pelletTested = false;
	bool d_wheelTested  = false;
	bool d_pelletGood   = false;
	bool d_wheelGood    = false;

public:
	inline SelfCheckMode(DispenserController &controller)
	    : Mode{controller} {
		Infof("Dispenser: Self-Check started");
		Self().d_pelletSensor.SetEnabled(true);
		Self().d_wheelSensor.SetEnabled(true);
	}

	std::unique_ptr<Mode> operator()(absolute_time_t now) override {
		if (Self().d_pelletSensor.HasError() ||
		    Self().d_pelletSensor.HasValue()) {
			d_pelletTested = true;

			d_pelletGood = !Self().d_pelletSensor.HasError();
			Self().d_pelletSensor.SetEnabled(false);
		}

		if (Self().d_wheelSensor.HasError() ||
		    Self().d_wheelSensor.HasValue()) {

			d_wheelTested = true;
			d_wheelGood   = !Self().d_wheelSensor.HasError();
			Self().d_wheelSensor.SetEnabled(false);
		}

		if (d_pelletTested == false || d_wheelTested == false) {
			return nullptr;
		}

		Self().d_sane = d_wheelGood && d_pelletGood;

		if (Self().d_sane == false) {
			Warnf("Dispenser: disabled after failed self-check");
		}

		return std::make_unique<IdleMode>(Self(), now);
	}
};

class DispenseMode : public Mode {
private:
	uint d_want, d_startCount;
	int  d_startPosition;
	int  d_direction;

	uint d_positionTravelled = 0;
	uint d_directionChange   = 0;

	const DispenserController::Config &d_config;

	DispenserController::DispenseCallback d_callback;

	inline static int direction = 1;

public:
	inline DispenseMode(
	    DispenserController                         &controller,
	    uint                                         want,
	    const DispenserController::DispenseCallback &callback
	)
	    : Mode{controller}
	    , d_want{want}
	    , d_startCount{controller.d_counter.PelletCount()}
	    , d_startPosition{controller.d_wheel.Position()}
	    , d_config{controller.d_config}
	    , d_callback{callback} {

		if (direction == 1 &&
		    d_startPosition >= d_config.ToggleDirectionThreshold) {
			direction = -1;
		} else if (direction == -1 && -d_startPosition >= d_config.ToggleDirectionThreshold) {
			direction = 1;
		}
		d_direction = direction;

		Self().d_pelletSensor.SetEnabled(true);
		Self().d_wheel.Start(d_direction);
		Infof("Dispenser: dispensing %d pellet(s)", want);
	}

	~DispenseMode() {
		Self().d_wheel.Stop();
		direction = d_direction;
	}

	std::unique_ptr<Mode> operator()(absolute_time_t now) override {
		if (Self().d_wheel.HasValue()) {
			d_positionTravelled++;
		}

		if (Self().d_counter.HasValue()) {
			if ((Self().d_counter.Value() - d_startCount) >= d_want) {
				return std::make_unique<IdleMode>(
				    Self(),
				    now,
				    [callback   = d_callback,
				     startCount = d_startCount](uint count) {
					    callback(count - startCount, Error::NO_ERROR);
				    }
				);
			}
		}

		if (d_positionTravelled >= d_config.MaxSlotRatio * d_want) {
			return std::make_unique<IdleMode>(
			    Self(),
			    now,
			    [callback = d_callback, startCount = d_startCount](uint count) {
				    callback(count - startCount, Error::DISPENSER_EMPTY);
			    }
			);
		}

		auto wheelError = Self().d_wheel.Err();
		if (wheelError == Error::WHEEL_CONTROLLER_MOTOR_FAULT) {
			if (d_directionChange >= d_config.MaxDirectionChange) {
				return std::make_unique<IdleMode>(
				    Self(),
				    now,
				    [callback   = d_callback,
				     startCount = d_startCount](uint count) {
					    callback(count - startCount, Error::DISPENSER_JAMMED);
				    }
				);
			}
			d_direction = d_direction * -1;
			++d_directionChange;
			Self().d_wheel.Start(d_direction);
		}

		return nullptr;
	}
};

inline void debugWheelConfig(const WheelController::Config &config) {
	Debugf(
	    "speed:%d rampup:%d rewind:%d cooldown:%d",
	    config.Speed,
	    config.RampUpDuration_us,
	    config.RewindPulse_us,
	    config.SensorCooldown_us
	);
}

class CalibrateMode : public Mode {
public:
	constexpr static uint CoarseStep    = 5 * 1000;
	constexpr static uint FineStep      = 500;
	constexpr static uint PositionCount = 3;

	struct State {
		std::vector<DispenserController::CalibrationResult::Point> Results,
		    Coarse;

		uint Speed = 1024;

		uint Step = CoarseStep;

		uint Start = 0;
		uint Max   = 60 * 1000;
	};

	CalibrateMode(
	    DispenserController                          &controller,
	    const State	                              &state,
	    const DispenserController::CalibrateCallback &callback
	)
	    : Mode{controller}
	    , d_callback{callback}
	    , d_saved{controller.d_wheelConfig}
	    , d_state{state} {
		debugWheelConfig(controller.d_wheelConfig);
		controller.d_counter.SetEnabled(true);
		controller.d_wheelConfig.Speed             = state.Speed;
		auto rewind                                = next();
		controller.d_wheelConfig.RewindPulse_us    = rewind;
		controller.d_wheelConfig.SensorCooldown_us = 3000 * 1000;
		debugWheelConfig(controller.d_wheelConfig);
		controller.d_wheel.Start(1);
		d_start = get_absolute_time();

		Infof(
		    "Dispenser: calibrating speed:%d, rewind:%dus",
		    state.Speed,
		    controller.d_wheelConfig.RewindPulse_us
		);
	}

	~CalibrateMode() {}

	std::unique_ptr<Mode> operator()(absolute_time_t time) override {
		if (Self().d_counter.HasValue()) {
			Errorf("Dispenser calibration: dispenser not empty");
			return std::make_unique<IdleMode>(
			    Self(),
			    time,
			    [callback = d_callback](uint) {
				    callback({}, Error::DISPENSER_CALIBRATION_ERROR);
			    }
			);
		}

		if (Self().d_wheelSensor.Err() == Error::IR_SENSOR_READOUT_ERROR ||
		    Self().d_wheel.Err() != Error::NO_ERROR) {
			Errorf("Dispenser calibration: error with wheel sensor or motor");
			d_callback({}, Error::DISPENSER_CALIBRATION_ERROR);
			return std::make_unique<IdleMode>(Self(), time);
		}

		if (Self().d_wheel.HasValue()) {
			++d_position;
			if (d_position == PositionCount) {
				Self().d_wheel.Stop();
			}
		}

		if (Self().d_wheel.Ready() && Self().d_wheelSensor.Enabled() == false) {
			return nextStep(time);
		}

		return nullptr;
	};

private:
	std::unique_ptr<Mode> nextStep(absolute_time_t now) {
		auto &results = d_state.Results;
		results.push_back({
		    .Rewind_us = next(),
		    .Position  = 2 * (d_position - PositionCount),
		});

		if (Self().d_wheel.WheelAligned() == false) {
			results.back().Position += 1;
		}

		Infof(
		    "rewind:%d position: %d",
		    results.back().Rewind_us,
		    results.back().Position
		);

		Self().d_wheelConfig = d_saved;

		bool cannotBeBetter = results.back().Position == 0;
		bool atEndOfRange   = results.back().Rewind_us >= d_state.Max;
		bool increasing     = false;
		if (results.size() >= 2) {
			increasing = results[results.size() - 2].Position == 1 &&
			             results.back().Position > 1;
		}

		if (cannotBeBetter || atEndOfRange || increasing) {
			if (d_state.Step == CoarseStep) {
				return fineCalibration();
			} else {
				return end(now);
			}
		}

		return std::make_unique<CalibrateMode>(Self(), d_state, d_callback);
	}

	    std::unique_ptr<Mode> fineCalibration() {
			const auto &minPoint = findMin();

			auto newState = State{
			    .Coarse = d_state.Results,
			    .Speed  = d_state.Speed,
			    .Step   = FineStep,
			    .Start  = minPoint.Rewind_us - 2 * CoarseStep,
			    .Max    = minPoint.Rewind_us,
			};

			if (newState.Max <= CoarseStep) {
				newState.Max   = CoarseStep;
				newState.Start = 0;
			}

			return std::make_unique<CalibrateMode>(
			    Self(),
			    newState,
			    d_callback
			);
		}

		std::unique_ptr<Mode> end(absolute_time_t time) {
			const auto &minPoint = findMin();
			d_callback(
			    {
			        .CoarseSearch      = d_state.Coarse,
			        .FineSearch        = d_state.Results,
			        .Speed             = d_state.Speed,
			        .MinRewindPulse_us = minPoint.Rewind_us,
			        .Position          = minPoint.Position,
			    },
			    Error::NO_ERROR
			);
			return std::make_unique<IdleMode>(Self(), time);
		}

		const DispenserController::CalibrationResult::Point &findMin() {
			const auto &points = d_state.Results;
			auto        it     = std::min_element(
                points.begin(),
                points.end(),
                [](const auto &a, const auto &b) -> bool {
                    return a.Position < b.Position;
                }
            );
			if (it->Position == 0) {
				return *it;
			}
			auto next = std::find_if(
			    it,
			    points.end(),
			    [min = it->Position](const auto &el) {
				    return el.Position > min;
			    }
			);
			if (next == points.end()) {
				return *it;
			}
			return *next;
		}

		uint next() {
			if (d_state.Results.empty()) {
				return d_state.Start;
			}
			return d_state.Results.back().Rewind_us + d_state.Step;
		}

		uint16_t d_min = 0xffff, d_max = 0;

		DispenserController::CalibrateCallback d_callback;
		WheelController::Config                d_saved;

		absolute_time_t d_start;
		State           d_state;

		uint d_position = 0;
};

std::unique_ptr<Mode> IdleMode::operator()(absolute_time_t now) {
	auto ellapsed = absolute_time_diff_us(d_start, now);
	if (ellapsed >= d_config.PelletCounterCooldown_us) {
		Self().d_counter.SetEnabled(false);
		d_onCounterDisable(Self().d_counter.PelletCount());
		d_onCounterDisable = Noop;
	}

	if (ellapsed >= d_config.SelfCheckPeriod_us) {
		return std::make_unique<SelfCheckMode>(Self());
	}

	DispenserController::Command cmd;
	if (Self().d_queue.TryRemove(cmd) == false) {
		return nullptr;
	}

	return cmd();
}

DispenserController::DispenserController(
    const StaticConfig      &staticConfig,
    const Config            &config,
    WheelController::Config &wheelConfig
)

    : d_config{config}
    , d_wheelConfig{wheelConfig}
    , d_button{staticConfig.TestButton}
    , d_wheelSensor{staticConfig.WheelSensor}
    , d_pelletSensor{staticConfig.PelletSensor}
    , d_wheel{staticConfig.Wheel}
    , d_counter{staticConfig.Counter} {
	d_mode = std::make_unique<SelfCheckMode>(*this);
}

DispenserController::~DispenserController() = default;

void DispenserController::Process(absolute_time_t now) {
	processErrors();
	auto newMode = (*d_mode)(now);
	if (newMode) {
		d_mode = std::move(newMode);
	}
}

void DispenserController::Dispense(
    uint count, const std::function<void(uint, Error)> &callback
) {

	Command dispense = [this, count, callback]() -> std::unique_ptr<Mode> {
		if (d_sane == false) {
			callback(0, Error::DISPENSER_SELF_CHECK_FAIL);
			return nullptr;
		}

		return std::make_unique<DispenseMode>(*this, count, callback);
	};

	if (d_queue.TryAdd(std::move(dispense)) == false) {
		callback(0, Error::DISPENSER_QUEUE_FULL);
	}
}

void DispenserController::Calibrate(
    uint speed, const CalibrateCallback &callback
) {

	Command calibrate = [this, callback, speed]() -> std::unique_ptr<Mode> {
		if (d_sane == false) {
			callback({}, Error::DISPENSER_SELF_CHECK_FAIL);
			return nullptr;
		}

		return std::make_unique<CalibrateMode>(
		    *this,
		    CalibrateMode::State{.Speed = speed},
		    callback
		);
	};

	if (d_queue.TryAdd(std::move(calibrate)) == false) {
		callback({}, Error::DISPENSER_QUEUE_FULL);
	}
}

void DispenserController::processErrors() {
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

	if (d_wheelSensor.HasValue()) {
		Display::State().Wheel.SensorValue = d_wheelSensor.Value();
	}

	if (d_wheel.HasValue()) {
		Display::State().Wheel.Position = d_wheel.Value();
	}

	if (d_pelletSensor.HasValue()) {
		auto &state = Display::State().Pellet;
		state.Last  = d_pelletSensor.Value();
		state.Min   = std::min(state.Min, state.Last);
		state.Max   = std::max(state.Max, state.Last);
	}
}
