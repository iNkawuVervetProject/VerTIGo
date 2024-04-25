#include "Error.hpp"
#include "Log.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include "utils/Defer.hpp"

void ErrorReporter::report(Error err, uint timeout_us) {
	auto index = size_t(err);
	if (index >= NumErrors) {
		Debugf("unknown error %d reported", index);
		return;
	}

	defer {
		auto timeout = make_timeout_time_us(timeout_us);

		d_firedErrors[index] = timeout;

		if (is_nil_time(d_next)) {
			d_next = timeout;
		} else {
			d_next = absolute_time_min(d_next, timeout);
		}
	};

	if (is_nil_time(d_firedErrors[index]) == false) {
		return;
	}

	Errorf("%s", GetErrorDescription(err));

	// TODO ? other callbacks or notifications?
}

void ErrorReporter::Process(absolute_time_t now) {
	if (is_nil_time(d_next) || absolute_time_diff_us(d_next, now) < 0) {
		return;
	}
	d_next = nil_time;
	for (auto &t : d_firedErrors) {
		if (is_nil_time(t)) {
			continue;
		}
		if (absolute_time_diff_us(t, now) >= 0) {
			t = nil_time;
		} else {
			if (is_nil_time(d_next)) {
				d_next = t;
			} else {
				d_next = absolute_time_min(d_next, t);
			}
		}
	}
}
