#pragma once

#include "pico/types.h"
#include <vector>

class Processor {
public:
	Processor();
	virtual ~Processor();

	Processor(const Processor &other)            = delete;
	Processor &operator=(const Processor &other) = delete;

	virtual void Process(absolute_time_t time) = 0;

	static void ProcessAll(absolute_time_t time);

private:
	static std::vector<Processor *> s_instances;
};
