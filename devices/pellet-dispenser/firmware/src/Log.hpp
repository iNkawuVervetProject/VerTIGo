#pragma once

#include "Button.hpp"
#include "pico/time.h"
#include "pico/types.h"
#include "utils/Queue.hpp"

#include <stdarg.h>

#include <array>
#include <cstdio>
#include <string>

class Logger {
public:
	static Logger &Get() {
		static Logger logger;
		return logger;
	}

	enum class Level {
		FATAL = 0,
		ERROR,
		WARNING,
		INFO,
		DEBUG,
		TRACE,
	};

	struct Message {
		const char     *Value;
		absolute_time_t Time;
		enum Level      Level;
	};

	std::optional<Message> Pop();

	void Logf(Level level, const char *fmt, va_list args);

	inline void SetLevel(Level lvl) {
		d_level = lvl;
	}

private:
	Logger();

	static constexpr size_t BufferSize = 4096 * 4;

	std::array<char, BufferSize + 1> d_buffer;
	size_t                           d_start = 0;
	Queue<Message, 64, true>         d_queue;
	Level                            d_level = Level::INFO;
};

inline static void Fatalf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

inline static void Fatalf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	panic((std::string("Fatal error: ") + fmt).c_str(), args);
	va_end(args);
}

inline static void Errorf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

inline static void Errorf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	Logger::Get().Logf(Logger::Level::ERROR, fmt, args);
	va_end(args);
}

inline static void Warnf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

inline static void Warnf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	Logger::Get().Logf(Logger::Level::WARNING, fmt, args);
	va_end(args);
}

inline static void Infof(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

inline static void Infof(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	Logger::Get().Logf(Logger::Level::INFO, fmt, args);
	va_end(args);
}

inline static void Debugf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

#ifndef NDEBUG
inline static void Debugf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	Logger::Get().Logf(Logger::Level::DEBUG, fmt, args);
	va_end(args);
}
#else
inline static void Debugf(const char *fmt, ...) {}
#endif

inline static void Tracef(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

#ifndef NDEBUG
inline static void Tracef(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	Logger::Get().Logf(Logger::Level::TRACE, fmt, args);
	va_end(args);
}
#else
inline static void Tracef(const char *fmt, ...) {}
#endif
