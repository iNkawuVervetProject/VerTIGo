#include "hardware/DRV8848.hpp"
#include "hardware/PIOIRSensor.hpp"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/types.h"

#include <memory>
#include <stdio.h>

#ifdef USB_INTERFACE
#include "bsp/board.h"
#include "tusb.h"
#else
inline void board_init() {}

inline void tusb_init() {}

inline void tud_task() {}
#endif

#include "hardware/FlashStorage.hpp"

#include "Button.hpp"
#include "Config.hpp"
#include "DispenserController.hpp"
#include "Display.hpp"
#include "Error.hpp"
#include "Log.hpp"
#include "PelletCounter.hpp"
#include "WheelController.hpp"

#include <iomanip>
#include <sstream>

#define DISPLAY_PERIOD_MS 200

int main() {
	stdio_init_all();
	auto endInit = make_timeout_time_us(10 * 1000);

	Config config;
	FlashStorage<Config>::Load(config);

	board_init();
	tusb_init();

	// need to ensure a 10ms break before starting core1... (in the display
	// loop). Otherwise, core1 will stop.
	sleep_until(endInit);

	printf("\033[2J\033[m");

	auto displayTimeout = make_timeout_time_ms(DISPLAY_PERIOD_MS);

#ifndef NDEBUG
	Logger::Get().SetLevel(Logger::Level::DEBUG);
	Infof("Verbosity set to DEBUG");
#endif

	auto button = Button{17};

	auto wheelSensor = PIOIRSensor<1>(
	    {
	        .Pio       = pio0,
	        .SensorPin = 21,
	        .PeriodUS  = 500,
	    },
	    20U
	);

	auto pelletSensor = PIOIRSensor<2>(
	    {
	        .Pio       = pio0,
	        .SensorPin = 26,
	        .PeriodUS  = 500,
	    },
	    27U,
	    22U
	);

	auto motorDriver = DRV8848({
	    .nSleep = 2,
	    .nFault = 9,
	    .AIn1   = 3,
	    .AIn2   = 6,
	    .BIn1   = 8,
	    .BIn2   = 7,
	});

	auto wheel = WheelController(motorDriver, wheelSensor, config.Wheel);

	auto pellets = PelletCounter(pelletSensor, config.Pellet);

	auto dispenser = DispenserController(
	    DispenserController::StaticConfig{
	        .TestButton   = button,
	        .PelletSensor = pelletSensor,
	        .WheelSensor  = wheelSensor,
	        .Counter      = pellets,
	        .Wheel        = wheel,
	    },
	    config.Dispenser,
	    config.Wheel
	);

	Infof(
	    "speed:%d rewind:%d",
	    config.Wheel.Speed,
	    config.Wheel.RewindPulse_us
	);

	while (true) {
		tud_task();
		auto now = get_absolute_time();
		ErrorReporter::Get().Process(now);

		Processor::ProcessAll(now);

		if (absolute_time_diff_us(now, displayTimeout) <= 0) {
			Display::Update(now);
			displayTimeout = delayed_by_ms(displayTimeout, DISPLAY_PERIOD_MS);
		}
	}
}
