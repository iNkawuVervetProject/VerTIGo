#pragma once

#include "pico/types.h"
#include <cstdint>

struct Display {
	void Print(const absolute_time_t time);

	bool    ButtonPressed = false;
	int32_t PressCount    = 0;

	inline static Display &Get() {
		static Display instance;
		return instance;
	};

private:
	Display();
};
