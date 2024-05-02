#include "LEDMorse.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include <array>
#include <atomic>

LEDMorse::LEDMorse(uint pin, uint baseDuration_us)
    : d_pin{pin} {

	gpio_init(d_pin);
	gpio_set_dir(d_pin, true);
	gpio_put(d_pin, false);

	d_current = d_sequence.end();
}

void LEDMorse::Process(absolute_time_t now) {
	constexpr static bool values[5]    = {false, false, false, true, true};
	constexpr static uint durations[5] = {1, 3, 7, 1, 3};

	if (d_sequence.empty()) {
		return;
	}

	if (is_nil_time(d_next) == true) {
		d_current = d_sequence.begin();
		d_next    = delayed_by_us(
            now,
            durations[size_t(*d_current)] * d_baseDuration_us
        );
		gpio_put(d_pin, values[size_t(*d_current)]);
	}

	if (absolute_time_diff_us(d_next, now) < 0) {
		return;
	}
	d_current++;
	if (d_current == d_sequence.end()) {
		d_current = d_sequence.begin();
	}

	d_next =
	    delayed_by_us(now, durations[size_t(*d_current)] * d_baseDuration_us);

	gpio_put(d_pin, values[size_t(*d_current)]);
}

static std::array<const char *, 128> morseCode = {
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "", // 00-15
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "", // 16-31
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "", // 32-47
    "----",  ".----", "..---", "...--", "....-", ".....",
    "-....", "--...", "---..", "----.", "",      "",
    "",      "",      "",      "", // 48-63
    "",      ".-",    "-...",  "-.-.",  "-..",   ".",
    "..-.",  "--.",   "....",  "..",    ".----", "-.-",
    ".-..",  "--",    "-.",    "---", // 64-79
    ".--.",  "--.-",  "",      "",      "",      "",
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "", // 80-95
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "", // 96-111
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "",      "",      "",
    "",      "",      "",      "", // 112-127
};

void LEDMorse::Display(const std::string &text) {
	gpio_put(d_pin, false);
	d_next = nil_time;
	if (text.empty()) {
		d_sequence.clear();
		d_current = d_sequence.end();
	}

	for (const auto &c : text) {
		for (const auto &s : morseCode[c]) {
			d_sequence.push_back(s);
			if (c != ' ') {
				d_sequence.push_back(Signal::BREAK);
			}
		}
	}
}
