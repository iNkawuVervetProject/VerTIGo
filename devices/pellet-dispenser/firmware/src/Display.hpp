#pragma once

#include "Error.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/queue.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

#include "Button.hpp"
#include "utils/Queue.hpp"

class Display {
public:
	struct PelletSensorState {
		uint Count = 0, Last = 2000, Min = 2000, Max = 0;
	};

	struct TestButtonState {
		ButtonState State      = ButtonState::RELEASED;
		uint        PressCount = 0;
	};

	struct State {

		TestButtonState   TestButton;
		int               WheelIndex = 0;
		PelletSensorState Pellet;
		absolute_time_t   Time = nil_time;
	};

	static inline Display::State &State() {
		return Get().d_state;
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

	struct Message {
		size_t          Start;
		absolute_time_t Time;
	};

	static void formatHeader();
	static void printTime(absolute_time_t time);

	void  formatMessage();
	void  formatState();

	Display();

	void update(absolute_time_t time);

	struct State d_state;
	Error        d_last = Error::NO_ERROR;

	Queue<struct State, 1, true> d_queue;
};
