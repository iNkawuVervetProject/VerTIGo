
#include "ErrorReporter.hpp"
#include "Error.hpp"
#include "LEDMorse.hpp"
#include "Log.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include <string>

void ErrorReporter::report(Error err, uint timeout_us) {
	auto index = size_t(err);
	if (index >= NumErrors) {
		Warnf("ErrorReporter: skipping unknown error %d", err);
		return;
	}

	auto now = get_absolute_time();

	auto timeout = delayed_by_us(now, timeout_us);

	auto isNew = is_nil_time(d_firedErrors[index]);

	d_firedErrors[index] = timeout;

	if (is_nil_time(d_next)) {
		d_next = timeout;
	} else {
		d_next = absolute_time_min(d_next, timeout);
	}

	if (isNew == false) {
		return;
	}

	dispatchNewError(err, now);
	pushStateToLED();
}

void ErrorReporter::Process(absolute_time_t now) {
	if (is_nil_time(d_next) || absolute_time_diff_us(d_next, now) < 0) {
		return;
	}

	d_next = nil_time;
	bool changed{false};
	for (auto &t : d_firedErrors) {
		if (is_nil_time(t)) {
			continue;
		}
		if (absolute_time_diff_us(t, now) >= 0) {
			t       = nil_time;
			changed = true;
		} else {
			if (is_nil_time(d_next)) {
				d_next = t;
			} else {
				d_next = absolute_time_min(d_next, t);
			}
		}
	}

	if (changed == false) {
		return;
	}
	pushStateToLED();
}

void ErrorReporter::dispatchNewError(Error err, absolute_time_t now) {
	LoggedError l;

	if (d_errorLog.Full() == true) {
		d_errorLog.TryRemove(l);
	}

	l.Error = err;
	l.Time  = now;

	d_errorLog.TryAdd(l);

	Errorf("%s", GetErrorDescription(err));
}

void ErrorReporter::pushStateToLED() {
	std::string toDisplay;
	for (size_t idx = 0; idx < d_firedErrors.size(); ++idx) {
		if (is_nil_time(d_firedErrors[idx])) {
			continue;
		}
		toDisplay += std::to_string(idx);
	}
	LEDMorse::Display(toDisplay);
}
