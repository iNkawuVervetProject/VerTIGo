#include "hardware/DRV8848.hpp"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/types.h"

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
#include "Controller.hpp"
#include "Display.hpp"
#include "Error.hpp"
#include "Log.hpp"
#include "PelletCounter.hpp"
#include "WheelController.hpp"

#define DISPLAY_PERIOD_MS 200

static Config config;

int main() {
	stdio_init_all();
	auto endInit = make_timeout_time_us(10 * 1000);
	FlashStorage<Config>::Load(config);
	config.Wheel.Channel = DRV8848::OutputChannel::A;
	config.Wheel.Speed   = 1024;

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

	auto controller = Controller(
	    {
	        PelletCounter::StaticConfig{
	            PIOIRSensor<2>::Config{
	                .Pio       = pio0,
	                .SensorPin = 26,
	                .PeriodUS  = 500,
	            },
	            .IRPin           = 27,
	            .SensorEnablePin = 22,
	        },
	        WheelController::StaticConfig{
	            DRV8848::Config{
	                .nSleep = 2,
	                .nFault = 9,
	                .AIn1   = 3,
	                .AIn2   = 6,
	                .BIn1   = 8,
	                .BIn2   = 7,
	            },
	            PIOIRSensor<1>::Config{
	                .Pio       = pio0,
	                .SensorPin = 21,
	                .PeriodUS  = 500,
	            },
	            .SensorEnablePin = 20,

	        },
	        .TestButtonPin = 17,
	    },

	    config
	);

	while (true) {
		tud_task();
		auto now = get_absolute_time();
		ErrorReporter::Get().Process(now);

		controller.Process(now);

		if (absolute_time_diff_us(now, displayTimeout) <= 0) {
			Display::Update(now);
			displayTimeout = delayed_by_ms(displayTimeout, DISPLAY_PERIOD_MS);
		}
	}
}
