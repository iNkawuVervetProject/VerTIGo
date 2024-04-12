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

	// need a 10ms break before starting core1.... :(
	sleep_ms(10);
	Display::Get();

	auto displayTimeout = get_absolute_time();

	auto testButton = Button(17);

	auto wheelSensor = BitBangIRSensor(21, 20);

	wheelSensor.SetEnabled(true);

	while (true) {
		auto curTime = get_absolute_time();

		// Critical task here

		if (testButton.Process(curTime)) {
			Display::Get().State().ButtonPressed = testButton.Pressed;
			Display::Get().State().PressCount += testButton.Pressed ? 1 : 0;
		}

		auto newValue = wheelSensor.Process(curTime);
		if (newValue.has_value()) {
			Display::Get().State().WheelValue = newValue.value();
		}

		if (absolute_time_diff_us(curTime, displayTimeout) <= 0) {
			Display::Get().Print(curTime);
			displayTimeout = delayed_by_ms(displayTimeout, DISPLAY_PERIOD_MS);
		}
	}
}
