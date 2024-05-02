#include "LEDMorse.hpp"
#include "Log.hpp"
#include "boards/pico.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "pico/types.h"
#include <array>
#include <atomic>
#include <cstring>
#include <vector>

LEDMorse::LEDMorse() {

	gpio_init(LEDPin);
	gpio_set_dir(LEDPin, true);
	gpio_put(LEDPin, false);

	d_current = d_sequence.end();
}

void LEDMorse::Process(absolute_time_t now) {
	if (d_sequence.empty()) {
		return;
	}

	if (is_nil_time(d_next) == true) {
		Debugf("LED: starting display");
		d_current = d_sequence.begin();
		setSignal(now);
	}

	if (absolute_time_diff_us(d_next, now) < 0) {
		return;
	}
	Tracef("LED: next signal");

	d_current++;
	if (d_current == d_sequence.end()) {
		d_current = d_sequence.begin();
	}
	setSignal(now);
}

void LEDMorse::setSignal(absolute_time_t now) {
	constexpr static bool values[5]    = {false, false, false, true, true};
	constexpr static uint durations[5] = {1, 3, 7, 1, 3};

	auto next  = durations[size_t(*d_current)] * BaseDuration_us;
	auto value = values[size_t(*d_current)];
	d_next     = delayed_by_us(now, next);
	gpio_put(BaseDuration_us, value);

	uint ms = next / 1000;
	next -= 1000 * ms;
	Tracef("LED: state:%s next:%d.%03d", value ? "ON" : "OFF", ms, next);
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
    ".--.",  "--.-",  ".-.",   "...",   "-",     "..-",
    "...-",  ".--",   "-..-",  "-.--",  "--..",  "",
    "",      "",      "",      "", // 80-95
    "",      ".-",    "-...",  "-.-.",  "-..",   ".",
    "..-.",  "--.",   "....",  "..",    ".----", "-.-",
    ".-..",  "--",    "-.",    "---", // 96-111
    ".--.",  "--.-",  ".-.",   "...",   "-",     "..-",
    "...-",  ".--",   "-..-",  "-.--",  "--..",  "",
    "",      "",      "",      "", // 112-127
};

void debugSequence(const std::vector<Signal> &sequence) {
	std::string display;
	for (auto s : sequence) {
		switch (s) {
		case Signal::PAUSE:
			display += " ";
			break;
		case Signal::BREAK:
			display += ",";
			break;
		case Signal::SPACE:
			display += " ; ";
			break;
		case Signal::DIT:
			display += ".";
			break;
		case Signal::DAH:
			display += "-";
			break;
		default:
			break;
		}
	}
	Infof("LED: displaying '%s'", display.c_str());
}

void LEDMorse::display(const std::string &text) {
	gpio_put(LEDPin, false);
	d_next = nil_time;
	d_sequence.clear();
	if (text.empty()) {
		return;
	}

	auto mustBreak = false;

	for (const auto &c : text) {
		if (c >= 128) {
			continue;
		}

		if (c == ' ') {
			d_sequence.push_back(Signal::SPACE);
			mustBreak = false;
			continue;
		}

		if (mustBreak == false) {
			mustBreak = true;
		} else {
			d_sequence.push_back(Signal::BREAK);
		}

		const char *code = morseCode[c];
		for (size_t i = 0; i < 6; ++i) {
			if (code[i] == 0) {
				break;
			}
			if (i > 0) {
				d_sequence.push_back(Signal::PAUSE);
			}
			switch (code[i]) {
			case '.':
				d_sequence.push_back(Signal::DIT);
				break;
			case '-':
				d_sequence.push_back(Signal::DAH);
				break;
			default:
				break;
			}
		}
	}
	d_sequence.push_back(Signal::SPACE);
	d_sequence.push_back(Signal::SPACE);
	debugSequence(d_sequence);
}
