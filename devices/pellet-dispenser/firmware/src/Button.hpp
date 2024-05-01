#pragma once

#include "pico/time.h"
#include "pico/types.h"
#include "utils/Processor.hpp"
#include "utils/Publisher.hpp"
#include <optional>

enum class ButtonState {
	RELEASED,
	PRESSED,
	LONG_PRESS,
};

class Button : public Processor, public Publisher<ButtonState> {
public:
	ButtonState State = ButtonState::RELEASED;
	Button(uint pin);
	void Process(absolute_time_t time) override;

private:
	static constexpr uint LongPress_us = 300 * 1000; // 300ms
	static constexpr uint Debounce_us  = 5 * 1000;   // 5ms;

	uint            d_pin;
	absolute_time_t d_last = nil_time;
};
