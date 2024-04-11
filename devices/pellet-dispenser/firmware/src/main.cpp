#include "Display.hpp"
#include "pico/stdlib.h"
#include "pico/time.h"

#include <stdio.h>

#define DISPLAY_PERIOD_MS 200

int main() {
	setup_default_uart();

	auto displayTimeout = get_absolute_time();

	while (true) {
		auto curTime = get_absolute_time();
		// Critical task here

		if (absolute_time_diff_us(curTime, displayTimeout) <= 0) {
			Display::Get().Print(curTime);
			displayTimeout = delayed_by_ms(displayTimeout, DISPLAY_PERIOD_MS);
		}
	}
}
