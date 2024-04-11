#include "Display.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include <array>
#include <cstdio>
#include <sstream>
#include <string>

Display::Display() {
	std::string title = "Pellet Dispenser Rev A version 0.0.0";
	printf(
	    "\033[2J\n\033[30;46m%*s%*s\033[m\n\n\n",
	    40 + title.size() / 2,
	    title.c_str(),
	    40 - title.size() / 2,
	    ""
	);
}

void Display::Print(const absolute_time_t currentTime) {
	std::array<char[200], 1> lines;
	sprintf(
	    lines[0],
	    "uptime: %d pressed: %s pressCount: %d",
	    to_ms_since_boot(currentTime) / 1000,
	    ButtonPressed ? "true" : "false",
	    PressCount
	);

	printf("\033[%dA\033[36m", lines.size() + 1);
	for (const auto &l : lines) {
		printf("┃\033[m%-78s\033[36m┃\n", l);
	}
	printf(
	    "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	    "━━━━━━━━━┛\n"
	);
}
