#pragma once

#include "Error.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/queue.h"

#include <cstdint>
#include <vector>

class Display {
public:
	struct TimedError {
		absolute_time_t Time;
		::Error         Error;
	};

	struct State {
		bool            ButtonPressed = false;
		int32_t         PressCount    = 0;
		int             WheelIndex    = 0;
		absolute_time_t Time          = nil_time;
	};

	static inline Display::State &State() {
		return Get().d_state;
	}

	static inline void PushError(const TimedError &error) {
		queue_try_add(&Get().d_errorQueue, &error);
	}

	static inline void Update(absolute_time_t time) {
		return Get().update(time);
	}

private:
	static void printLoop();

	inline static Display &Get() {
		static Display instance;
		return instance;
	};

	Display();

	void update(absolute_time_t time);

	struct State d_state;
	queue_t      d_stateQueue, d_errorQueue;
};
