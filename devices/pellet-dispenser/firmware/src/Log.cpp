#include "Log.hpp"
#include <cstdio>
#include <optional>
#include <stdio.h>

void Logger::Logf(Level level, const char *fmt, va_list args) {
	if (level > d_level) {
		return;
	}

	auto now = get_absolute_time();

	auto written =
	    vsnprintf(d_buffer.data() + d_start, BufferSize - d_start, fmt, args);
	while (d_start + written >= BufferSize) {
		if (d_start == 0) {
			d_buffer[BufferSize - 1] = 0;
		} else {
			d_start = 0;
			written = vsnprintf(d_buffer.data(), BufferSize, fmt, args);
		}
	}
	va_end(args);

	Message m = {
	    .Value = d_buffer.data() + d_start,
	    .Time  = now,
	    .Level = level,
	};

	if (d_queue.TryAdd(std::move(m)) == true) {
		d_start += written + 1;
	}
}

std::optional<Logger::Message> Logger::Pop() {
	struct Message msg;
	if (d_queue.TryRemove(msg) == false) {
		return std::nullopt;
	}

	return msg;
}

Logger::Logger() {
	d_buffer[BufferSize] = 0;
}
