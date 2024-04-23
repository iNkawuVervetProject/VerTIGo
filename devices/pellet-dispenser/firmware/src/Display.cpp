#include "Display.hpp"

#include "Defer.hpp"
#include "Error.hpp"
#include "pico/multicore.h"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/queue.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

static const char *resetState   = "\033[4A";
static const char *advanceState = "\n\n\n\n";

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
	queue_remove_blocking(&d_stateQueue, &s);
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

Error Display::formatError(Error last) {
	if (queue_is_empty(&d_errorQueue)) {
		return last;
	}
	printf("%s", resetState);
	defer {
		printf("%s", advanceState);
	};
	struct Display::TimedError error;
	while (true) {
		if (queue_try_remove(&d_errorQueue, &error) == false) {
			break;
		}
		if (error.Error == last) {
			continue;
		}
		last = error.Error;
		printf("\033[30;43m");
		printTime(error.Time);
		printf("\033[33;40m %-54s ┃\n", GetErrorDescription(error.Error));
	}

	return last;
}

void Display::formatMessage() {
	if (queue_is_empty(&d_messageQueue)) {
		return;
	}
	printf("%s", resetState);
	defer {
		printf("%s", advanceState);
	};
	struct Message msg;
	while (true) {
		if (queue_try_remove(&d_messageQueue, &msg) == false) {
			break;
		}
		size_t n = strlen(d_buffer.data() + msg.Start);
		for (size_t i = 0; i < n; i += 54) {
			printf("\033[30;46m");
			if (i == 0) {
				printTime(msg.Time);
			} else {
				printf("                       ");
			}
			printf("\033[m %-.54s ", d_buffer.data() + msg.Start + i);
			if (i + 54 < n) {
				printf("\033[36m┃\n");
			} else {
				printf(
				    "%.*s\033[36m┃\n",
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

	auto &stateQueue = Display::Get().d_stateQueue;
	auto &self       = Get();

	Error last = Error::NO_ERROR;

	while (true) {
		last = self.formatError(last);
		self.formatMessage();
		self.formatState();
	}
}

Display::Display() {
	queue_init(&d_stateQueue, sizeof(struct State), 1);
	queue_init(&d_errorQueue, sizeof(struct TimedError), 10);
	queue_init(&d_messageQueue, sizeof(struct Message), 10);
	multicore_launch_core1(printLoop);
}

void Display::update(absolute_time_t time) {
	struct State discard;
	while (queue_try_remove(&d_stateQueue, &discard)) {
	}
	d_state.Time = time;
	queue_try_add(&d_stateQueue, &d_state);
}
