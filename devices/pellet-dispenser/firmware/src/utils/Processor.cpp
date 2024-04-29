#include "Processor.hpp"

std::vector<Processor *> Processor::s_instances;

Processor::Processor() {
	s_instances.push_back(this);
}

Processor::~Processor() {
	for (auto it = s_instances.begin(); it != s_instances.end(); ++it) {
		if (*it != this) {
			continue;
		}
		s_instances.erase(it);
		return;
	}
}

void Processor::ProcessAll(absolute_time_t time) {
	for (auto &p : s_instances) {
		p->Process(time);
	}
}
