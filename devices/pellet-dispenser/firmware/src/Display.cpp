#include "Display.hpp"

#include "pico/multicore.h"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/queue.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "Error.hpp"
#include "Log.hpp"
#include "utils/Defer.hpp"

static const char *resetState   = "\033[5A";
static const char *advanceState = "\n\n\n\n\n";

void Display::formatHeader() {
	std::string title = "Pellet Dispenser Rev A version 0.0.0";
	printf(
	    "\n\033[30;46m%*s%*s\033[m\n%s",
	    40 + title.size() / 2,
	    title.c_str(),
	    40 - title.size() / 2,
	    "",
	    advanceState
	);
}

void Display::formatState() {
	struct State s;
	d_queue.RemoveBlocking(s);
	int ms      = to_ms_since_boot(s.Time);
	int seconds = ms / 1000;
	ms          = ms - seconds * 1000;

	printf(
	    "%s\033[30;46m %20s: \033[m %06d.%03ds %43s\033[36m┃\n",
	    resetState,
	    "Up Time",
	    seconds,
	    ms,
	    ""
	);

	printf(
	    "\033[30;46m %20s: \033[m pressed:%d,count:%-39d\033[36m┃\n",
	    "Button",
	    uint(s.TestButton.State),
	    s.TestButton.PressCount
	);

	printf("\033[30;46m %20s: \033[m %-55d\033[36m┃\n", "Wheel", s.WheelIndex);

	printf(
	    "\033[30;46m %20s: \033[m count:%-4d last:%-4d min:%-4d max:%-20d "
	    "\033[36m┃\n",
	    "Pellet",
	    s.Pellet.Count,
	    s.Pellet.Last,
	    s.Pellet.Min,
	    s.Pellet.Max
	);

	printf(
	    "███████████████████████━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	    "━━━━━━━━━┛\n"
	);
}

void Display::formatMessage() {
	bool needReset = false;
	defer {
		if (needReset == true) {
			printf("%s", advanceState);
		}
	};

	static uint8_t colors[6] = {
	    1, // FATAL - RED
	    1, // ERROR - RED
	    3, // WARNING - YELLOW
	    6, // INFO - CYAN
	    7, // DEBUG - WHITE
	    4, // TRACE - BLUE
	};

	while (true) {

		auto msg = Logger::Get().Pop();
		if (msg.has_value() == false) {
			return;
		}

		if (needReset == false) {
			printf("%s", resetState);
			needReset = true;
		}

		const char *msgStr = msg.value().Value;
		size_t      n      = strlen(msgStr);
		auto        c      = colors[size_t(msg.value().Level)];

		for (size_t i = 0; i < n; i += 54) {
			printf("\033[30;4%dm", c);
			if (i == 0) {
				printTime(msg.value().Time);
			} else {
				printf("                       ");
			}
			printf("\033[m\033[3%dm %-.54s ", c, msgStr + i);
			if (i + 54 < n) {
				printf("┃\n");
			} else {
				printf(
				    "%.*s┃\n",
				    54 - n + i,
				    "                                                      "
				);
			}
		}
	}
}

void Display::printTime(absolute_time_t time) {
	uint us = to_us_since_boot(time);
	uint s  = us / 1000000;
	us -= s * 1000000;
	printf("       %06d.%06ds: ", s, us);
}

void Display::printLoop() {

	formatHeader();
	int i{0};

	auto &self = Get();

	while (true) {
		self.formatMessage();
		self.formatState();
	}
}

Display::Display() {
	multicore_launch_core1(printLoop);
}

void Display::update(absolute_time_t time) {
	struct State discard;
	while (d_queue.TryRemove(discard)) {
	}
	d_state.Time = time;
	d_queue.TryAdd(d_state);
}
