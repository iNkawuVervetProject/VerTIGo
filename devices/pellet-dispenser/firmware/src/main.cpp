#include "Button.hpp"
#include "Display.hpp"
#include "pico/stdlib.h"
#include "pico/time.h"

#include <stdio.h>

#define DISPLAY_PERIOD_MS 200

int main() {
	setup_default_uart();

	auto displayTimeout = get_absolute_time();

	auto testButton = Button(17);

	while (true) {
		auto curTime = get_absolute_time();

		// Critical task here

		if (testButton.Process(curTime)) {
			Display::Get().ButtonPressed = testButton.Pressed;
			Display::Get().PressCount += testButton.Pressed ? 1 : 0;
		}

		if (absolute_time_diff_us(curTime, displayTimeout) <= 0) {
			Display::Get().Print(curTime);
			displayTimeout = delayed_by_ms(displayTimeout, DISPLAY_PERIOD_MS);
		}
	}
}
