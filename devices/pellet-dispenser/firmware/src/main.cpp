#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/types.h"

#include "bsp/board.h"
#include "tusb.h"

#include "hardware/FlashStorage.hpp"

#include "Button.hpp"
#include "Config.hpp"
#include "Display.hpp"
#include "WheelController.hpp"

#include <stdio.h>

#define DISPLAY_PERIOD_MS 200

static Config config;

int main() {
	stdio_init_all();
	auto endInit = make_timeout_time_us(10 * 1000);
	FlashStorage<Config>::Load(config);

	board_init();
	tusb_init();

	// need to ensure a 10ms break before starting core1... (in the display
	// loop). Otherwise, core1 will stop.
	sleep_until(endInit);

	printf("\033[2J\033[m");

	auto displayTimeout = make_timeout_time_ms(DISPLAY_PERIOD_MS);

	auto testButton = Button(17);

	auto wheel = WheelController(
	    {
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
	    config.Wheel
	);

	wheel.Start();

	while (true) {
		tud_task();
		auto curTime = get_absolute_time();

		// Critical task here
		if (testButton.Process(curTime)) {

			Display::State().ButtonPressed = testButton.Pressed;
			Display::State().PressCount += testButton.Pressed ? 1 : 0;
			if (testButton.Pressed) {
				wheel.Start();
			}
		}

		auto [newPos, error] = wheel.Process(curTime);
		if (newPos.has_value()) {
			Display::State().WheelIndex = newPos.value();
			wheel.Stop();
		}
		if (error != Error::NO_ERROR) {
			Display::PushError({.Time = curTime, .Error = error});
		}

		if (absolute_time_diff_us(curTime, displayTimeout) <= 0) {
			Display::State().WheelIndex = wheel.Position();
			Display::Update(curTime);
			displayTimeout = delayed_by_ms(displayTimeout, DISPLAY_PERIOD_MS);
		}
	}
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(
    uint8_t           instance,
    uint8_t           report_id,
    hid_report_type_t report_type,
    uint8_t          *buffer,
    uint16_t          reqlen
) {
	// TODO not Implemented
	(void)instance;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)reqlen;

	return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(
    uint8_t           instance,
    uint8_t           report_id,
    hid_report_type_t report_type,
    uint8_t const    *buffer,
    uint16_t          bufsize
) {
	(void)instance;
}
