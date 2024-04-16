#include "Button.hpp"
#include "DRV8848.hpp"
#include "Display.hpp"
#include "IRSensor.hpp"
#include "PIOIRSensor.hpp"
#include "WheelController.hpp"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/types.h"

#include <stdio.h>

#define DISPLAY_PERIOD_MS 200

int main() {
	stdio_init_all();

	// need to ensure a 10ms break before starting core1... (in the display
	// loop). Otherwise, core1 will stop.
	printf("\033[2J\033[m");
	sleep_ms(10);

	auto displayTimeout = get_absolute_time();

	auto testButton = Button(17);

	auto wheel = WheelController({
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
	    .Speed           = 500,
	});

	while (true) {
		auto curTime = get_absolute_time();

		// Critical task here
		if (testButton.Process(curTime)) {
			Display::State().ButtonPressed = testButton.Pressed;
			Display::State().PressCount += testButton.Pressed ? 1 : 0;
			if (testButton.Pressed) {
				wheel.Move(1);
			}
		}

		wheel.Process(curTime);

		if (absolute_time_diff_us(curTime, displayTimeout) <= 0) {
			Display::State().WheelIndex = wheel.Position();
			Display::Update(curTime);
			displayTimeout = delayed_by_ms(displayTimeout, DISPLAY_PERIOD_MS);
		}
	}
}
