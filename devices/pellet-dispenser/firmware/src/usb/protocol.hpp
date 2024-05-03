#pragma once

#include <cstdint>

#include "ErrorReporter.hpp"

namespace usb {

struct Command {
	enum Code {
		DISPENSE  = 0x71,
		CALIBRATE = 0x72,
	};

	uint8_t  Code;
	uint16_t Parameter;
} __attribute__((packed));

struct CommandReport {
	uint8_t  Code;
	uint16_t Value;
	uint8_t  Error;
} __attribute__((packed));

enum class FeatureReportType {
	ERROR_LOG    = 0x61,
	CURRENT_TIME = 0x62,
};

struct ErrorReport {
	static constexpr size_t MaxNumberOfReport =
	    (64 - 1) / sizeof(ErrorReporter::LoggedError);

	uint8_t                    Count;
	ErrorReporter::LoggedError Errors[MaxNumberOfReport];
} __attribute__((packed));

struct TimeReport {
	uint64_t CurrentTime;
};

}; // namespace usb
