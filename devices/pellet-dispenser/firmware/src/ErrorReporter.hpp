#pragma once



#include "pico/types.h"
class ErrorReporter {
public:
	static ErrorReporter & Get() {
		static ErrorReporter reporter;
		return reporter;
	};

	void Process(absolute_time_t time);

private:
	ErrorReporter() = default
};
