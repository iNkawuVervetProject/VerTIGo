#include "Log.hpp"
#include "pico/util/queue.h"
#include <optional>

void Logger::Logf(Level level, const char *fmt, ...) {
	if (level > d_level) {
		return;
	}

	auto    now = get_absolute_time();
	va_list args;
	va_start(args, fmt);

	auto written =
	    snprintf(d_buffer.data() + d_start, BufferSize - d_start, fmt, args);
	while (d_start + written >= BufferSize) {
		if (d_start == 0) {
			d_buffer[BufferSize - 1] = 0;
		} else {
			d_start = 0;
			written = snprintf(d_buffer.data(), BufferSize, fmt, args);
		}
	}
	va_end(args);

	Header h = {.Start = d_start, .Time = now, .Level = level};
	if (queue_try_add(&d_queue, &h) == true) {
		d_start += written + 1;
	}
}

std::optional<Logger::Message> Logger::Pop() {
	struct Header h;
	if (queue_try_remove(&d_queue, &h) == false) {
		return std::nullopt;
	}

	return Message{
	    .Value = d_buffer.data() + h.Start,
	    .Time  = h.Time,
	    .Level = h.Level,
	};
}

Logger::Logger() {
	d_buffer[BufferSize] = 0;
	queue_init(&d_queue, sizeof(struct Header), 16);
}
