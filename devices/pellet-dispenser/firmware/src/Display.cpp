#include "Display.hpp"

#include "pico/multicore.h"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/queue.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

void formatHeader() {
	std::string title = "Pellet Dispenser Rev A version 0.0.0";
	printf(
	    "\n\033[30;46m%*s%*s\033[m\n\n\n",
	    40 + title.size() / 2,
	    title.c_str(),
	    40 - title.size() / 2,
	    ""
	);
}

void formatState(const struct Display::State &s) {
	std::array<char[200], 1> lines;

	sprintf(
	    lines[0],
	    "uptime: %d button: {%s,%d}, wheel: {%d,min:%d,max:%d,%d}",
	    to_ms_since_boot(s.Time) / 1000,
	    s.ButtonPressed ? "true" : "false",
	    s.PressCount,
	    s.WheelValue,
	    s.WheelMin,
	    s.WheelMax,
	    s.WheelCount
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

void Display::printLoop() {

	formatHeader();
	int i{0};

	struct Display::State toDisplay;
	auto	             &queue = Display::Get().d_queue;
	while (true) {
		queue_remove_blocking(&queue, &toDisplay);
		formatState(toDisplay);
	}
}

Display::Display() {
	queue_init(&d_queue, sizeof(struct State), 1);

	multicore_launch_core1(printLoop);
}

void Display::update(absolute_time_t time) {
	struct State discard;
	while (queue_try_remove(&d_queue, &discard)) {
	}

	d_state.Time = time;
	queue_try_add(&d_queue, &d_state);
}
