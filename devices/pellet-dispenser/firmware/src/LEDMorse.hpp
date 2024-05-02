#pragma once

#include "pico/time.h"
#include "pico/types.h"

#include <string>
#include <vector>

#include "utils/Processor.hpp"

enum class Signal {
	PAUSE = 0, // INSIDE char
	BREAK,     // BETWEEN char
	SPACE,     // BETWEEN word
	DIT,
	DAH,
};

class LEDMorse : public Processor {
public:
	LEDMorse(uint pin, uint baseDuration_us = 500 * 1000);

	void Display(const std::string &text);

	void Process(absolute_time_t time) override;

private:
	using Sequence = std::vector<Signal>;

	uint                     d_pin;
	Sequence                 d_sequence;
	Sequence::const_iterator d_current;
	uint                     d_baseDuration_us;
	absolute_time_t          d_next = nil_time;
};
