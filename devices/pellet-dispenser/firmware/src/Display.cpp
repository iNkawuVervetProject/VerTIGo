#include "Display.hpp"

#include "Error.hpp"
#include "pico/multicore.h"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/queue.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

static const char *resetState   = "\033[4A";
static const char *advanceState = "\n\n\n\n";

void formatHeader() {
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

void formatState(const struct Display::State &s) {
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
	    "\033[30;46m %20s: \033[m pressed:%s,count:%-39d\033[36m┃\n",
	    "Button",
	    s.ButtonPressed ? "1" : "0",
	    s.PressCount
	);

	printf("\033[30;46m %20s: \033[m %-55d\033[36m┃\n", "Wheel", s.WheelIndex);

	printf(
	    "███████████████████████━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	    "━━━━━━━━━┛\n"
	);
}

Error formatError(Error last, const std::vector<Display::TimedError> &errors) {
	if (errors.empty()) {
		return last;
	}
	printf("%s", resetState);
	for (const auto &[ts, e] : errors) {
		if (e == last) {
			continue;
		}
		last    = e;
		uint us = to_us_since_boot(ts);
		uint s  = us / 1000000;
		us -= s * 1000000;
		printf(
		    "\033[30;43m       %06d.%06ds: \033[33;40m %-54s ┃\n",
		    s,
		    us,
		    GetErrorDescription(e)
		);
	}
	printf("%s", advanceState);
	return last;
}

void Display::printLoop() {

	formatHeader();
	int i{0};

	struct Display::State toDisplay;

	auto &stateQueue = Display::Get().d_stateQueue;
	auto &errorQueue = Display::Get().d_errorQueue;

	std::vector<TimedError> newErrors;
	Error                   last = Error::NO_ERROR;
	while (true) {
		queue_remove_blocking(&stateQueue, &toDisplay);

		while (queue_is_empty(&errorQueue) == false) {
			newErrors.push_back({});
			queue_try_remove(&errorQueue, &newErrors.back());
		}

		last = formatError(last, newErrors);
		newErrors.clear();

		formatState(toDisplay);
	}
}

Display::Display() {
	queue_init(&d_stateQueue, sizeof(struct State), 1);
	queue_init(&d_errorQueue, sizeof(struct TimedError), 10);

	multicore_launch_core1(printLoop);
}

void Display::update(absolute_time_t time) {
	struct State discard;
	while (queue_try_remove(&d_stateQueue, &discard)) {
	}
	d_state.Time = time;
	queue_try_add(&d_stateQueue, &d_state);
}
