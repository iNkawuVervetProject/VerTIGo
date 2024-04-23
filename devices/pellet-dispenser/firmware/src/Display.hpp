#pragma once

#include "Error.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include "pico/util/queue.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

class Display {
public:
	struct TimedError {
		absolute_time_t Time;
		::Error         Error;
	};

	struct State {
		bool            ButtonPressed = false;
		int32_t         PressCount    = 0;
		int             WheelIndex    = 0;
		absolute_time_t Time          = nil_time;
	};

	static inline Display::State &State() {
		return Get().d_state;
	}

	static inline void PushError(const TimedError &error) {
		if (error.Error == Error::NO_ERROR || error.Error == Get().d_last) {
			return;
		}
		Get().d_last = error.Error;
		queue_try_add(&Get().d_errorQueue, &error);
	}

	static inline void Update(absolute_time_t time) {
		return Get().update(time);
	}

	template <typename... Args>
	static inline void Printf(const char *fmt, Args &&...args) {
		auto  now     = get_absolute_time();
		auto &self    = Get();
		auto  written = snprintf(
            self.d_buffer.data() + self.d_offset,
            BufferSize - self.d_offset,
            fmt,
            std::forward<Args>(args)...
        );

		if (self.d_offset != 0 && self.d_offset + written >= BufferSize) {
			self.d_offset = 0;
			written       = snprintf(
                self.d_buffer.data(),
                BufferSize,
                fmt,
                std::forward<Args>(args)...
            );
		}
		Message msg = {.Start = self.d_offset, .Time = now};
		queue_try_add(&self.d_messageQueue, &msg);
		self.d_offset += written + 1;
	}

private:
	static void printLoop();

	inline static Display &Get() {
		static Display instance;
		return instance;
	};

	struct Message {
		size_t          Start;
		absolute_time_t Time;
	};

	static void formatHeader();
	static void printTime(absolute_time_t time);

	Error formatError(Error last);
	void        formatMessage();
	void        formatState();

	Display();

	void update(absolute_time_t time);

	struct State d_state;
	Error        d_last = Error::NO_ERROR;

	static constexpr size_t BufferSize = 4096;

	std::array<char, BufferSize> d_buffer;
	size_t                       d_offset = 0;
	queue_t                      d_stateQueue, d_errorQueue, d_messageQueue;
};
