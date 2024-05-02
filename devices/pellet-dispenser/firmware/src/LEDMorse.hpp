#pragma once

#include "boards/pico.h"
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
	constexpr static uint BaseDuration_us = 100 * 1000;
	constexpr static uint LEDPin          = PICO_DEFAULT_LED_PIN;

	inline static LEDMorse &Get() {
		static LEDMorse instance;
		return instance;
	}

	inline static void Display(const std::string &text) {
		Get().display(text);
	}

	void Process(absolute_time_t time) override;

private:
	using Sequence = std::vector<Signal>;

	LEDMorse();
	void setSignal(absolute_time_t now);
	void display(const std::string &text);

	Sequence                 d_sequence;
	Sequence::const_iterator d_current;
	absolute_time_t          d_next = nil_time;
};
