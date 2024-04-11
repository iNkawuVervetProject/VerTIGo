#include "Display.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include <cstdio>

Display::Display() {

	printf("\033[2J\n\n");
}

void Display::Print(const absolute_time_t currentTime) {
	printf("\033[Auptime: %d\n", to_ms_since_boot(currentTime) / 1000);
}
