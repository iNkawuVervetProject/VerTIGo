#pragma once

#include "Error.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include "utils/Processor.hpp"
#include "utils/Queue.hpp"
#include <array>

class ErrorReporter : public Processor {
public:
	inline static ErrorReporter &Get() {
		static ErrorReporter reporter;
		return reporter;
	}

	static inline void Report(Error err, uint timeout_us) {
		Get().report(err, timeout_us);
	};

	void Process(absolute_time_t time) override;

	struct LoggedError {
		absolute_time_t Time;
		enum Error      Error;
	} __attribute__((packed));

	using ErrorQueue = Queue<LoggedError, 64, true>;

	inline static ErrorQueue &ErrorLog() {
		return Get().d_errorLog;
	}

private:
	void report(Error e, uint timeout_us);

	void dispatchNewError(Error e, absolute_time_t now);

	void pushStateToLED();

	std::array<absolute_time_t, NumErrors> d_firedErrors;

	absolute_time_t d_next = nil_time;

	ErrorQueue d_errorLog;
};
