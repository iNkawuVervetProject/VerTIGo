#pragma once

#include "pico/types.h"
#include <cstdint>

struct Display {
	void Print(const absolute_time_t time);

	inline static Display &Get() {
		static Display instance;
		return instance;
	};

private:
	Display();
};
