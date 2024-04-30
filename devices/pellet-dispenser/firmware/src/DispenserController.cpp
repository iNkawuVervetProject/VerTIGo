#include "DispenserController.hpp"
#include "Display.hpp"
#include "Error.hpp"
#include "Log.hpp"
#include "WheelController.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include "utils/Publisher.hpp"
#include <memory>

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
	    , d_config{controller.d_config} {}

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

		if (!d_pelletTested || !d_wheelTested) {
			return nullptr;
		}

		if (!d_pelletGood) {
			Warnf("Pellet IR Sensor issue, disabling pellet dispenser");
		}

		if (!d_wheelGood) {
			Warnf("Wheel IR Sensor issue, disabling pellet dispenser");
		}

		Self().d_sane = d_wheelGood && d_pelletGood;

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
	    DispenserController	                     &controller,
	    uint                                want,
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

class CalibrateMode : public Mode {
public:

	struct State {
		std::optional<uint> High;
		std::optional<uint> Low =0;
		uint Next = 100 * 1000;
	};

	CalibrateMode(
	              DispenserController &controller,
	              const State & state,
	              const DispenserController::CalibrateCallback &callback
	)
	    : Mode{controller}
	    , d_callback{callback}
	    , d_saved{controller.d_wheelConfig}
	    , d_state{state} {
		controller.d_counter.SetEnabled(true);
		controller.d_wheelConfig.RewindPulse_us = state.Next;
		controller.d_wheel.Start(1);
		d_start = get_absolute_time();
	}

	~CalibrateMode() {
		Self().d_wheelConfig =  d_saved;
	}

	std::unique_ptr<Mode> operator()(absolute_time_t time) override {
		if(Self().d_counter.HasValue()) {
			Errorf("Dispenser calibration: dispenser not empty");
			return std::make_unique<IdleMode>(Self(),time,
			                                  [callback = d_callback](uint) {
				                                  callback({},Error::DISPENSER_CALIBRATION_ERROR);
			                                  });
		}

		if ( Self().d_wheelSensor.Err() == Error::IR_SENSOR_READOUT_ERROR
		     || Self().d_wheel.Err() == Error::WHEEL_CONTROLLER_MOTOR_FAULT ) {
			Errorf("Dispenser calibration: error with wheel sensor or motor");
			d_callback({},Error::DISPENSER_CALIBRATION_ERROR);
			return  std::make_unique<IdleMode>(Self(),time);
		}

		if ( Self().d_wheel.HasValue() ) {
			if ( ++d_position == 2 ) {
				Self().d_wheel.Stop();
			}
		}

		if (Self().d_wheelSensor.HasValue() ) {
			d_points.push_back({
					.Time =uint32_t(absolute_time_diff_us(d_start, time)),
					.Value = uint16_t(Self().d_wheelSensor.Value()),
				});
		}

		if (Self().d_wheel.Ready() && Self().d_wheelSensor.Enabled() == false ) {
			for ( const auto & p : d_points ) {
				Infof("%d,%d",p.Time,p.Value);
			}
			return std::make_unique<IdleMode>(Self(),time);
		}


		return nullptr;
	};
private:
	struct SensorPoint {
		uint32_t Time;
		uint16_t Value;
	};


	DispenserController::CalibrateCallback d_callback;
	WheelController::Config d_saved;


	std::vector<SensorPoint> d_points;
	absolute_time_t d_start;
	State d_state;

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

DispenserController::DispenserController(const StaticConfig &staticConfig, const Config &config, WheelController::Config & wheelConfig)

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

	if (d_sane == false) {
		callback(0, Error::DISPENSER_SELF_CHECK_FAIL);
		return;
	}

	Command dispense = [this, count, callback]() -> std::unique_ptr<Mode> {
		return std::make_unique<DispenseMode>(*this, count, callback);
	};

	if (d_queue.TryAdd(std::move(dispense)) == false) {
		callback(0, Error::DISPENSER_QUEUE_FULL);
	}
}

void DispenserController::Calibrate(const CalibrateCallback &callback) {

	if (d_sane == false) {
		callback({}, Error::DISPENSER_SELF_CHECK_FAIL);
		return;
	}

	Command calibrate = [this, callback]() -> std::unique_ptr<Mode> {
		return std::make_unique<CalibrateMode>(*this, CalibrateMode::State{},callback);
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
}
