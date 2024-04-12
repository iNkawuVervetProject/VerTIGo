#include "Button.hpp"
#include "Display.hpp"
#include "IRSensor.hpp"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include <stdio.h>

#define DISPLAY_PERIOD_MS 200

int main() {
	stdio_init_all();

	// need to ensure a 10ms break before starting core1... (in the display
	// loop). Otherwise, core1 will stop.
	sleep_ms(10);

	auto displayTimeout = get_absolute_time();

	auto testButton = Button(17);

	auto wheelSensor = BitBangIRSensor(21, 20);

	wheelSensor.SetEnabled(true);

	while (true) {
		auto curTime = get_absolute_time();

		// Critical task here
		if (testButton.Process(curTime)) {
			Display::State().ButtonPressed = testButton.Pressed;
			Display::State().PressCount += testButton.Pressed ? 1 : 0;
		}

		auto newValue = wheelSensor.Process(curTime);
		if (newValue.has_value()) {
			Display::State().WheelValue = newValue.value();
		}

		if (absolute_time_diff_us(curTime, displayTimeout) <= 0) {
			Display::Update(curTime);
			displayTimeout = delayed_by_ms(displayTimeout, DISPLAY_PERIOD_MS);
		}
	}
}
