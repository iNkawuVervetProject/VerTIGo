#pragma once

#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/queue.h"

#include <cstdint>

class Display {
public:
	struct State {
		bool            ButtonPressed = false;
		int32_t         PressCount    = 0;
		int32_t         WheelValue    = 0;
		absolute_time_t Time          = nil_time;
	};

	inline Display::State &State() {
		return d_state;
	}

	void Print(const absolute_time_t time);

	inline static Display &Get() {
		static Display instance;
		return instance;
	};

private:
	static void printLoop();

	Display();

	struct State d_state;
	queue_t      d_queue;
};
