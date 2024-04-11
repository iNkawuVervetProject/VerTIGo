#pragma once

#include "pico/types.h"

struct Button {
	bool Pressed;
	bool Process(absolute_time_t curTime);
	Button(uint pin);

private:
	uint            pin;
	absolute_time_t last;
};
