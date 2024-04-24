#pragma once

#include "pico/types.h"

#include <array>
#include <functional>

class IRQManager {
public:
	typedef std::function<void(uint, uint32_t)> IRQHandler;

	inline static IRQManager &Instance() {
		static IRQManager s_instance;
		return s_instance;
	}

	void Register(uint gpio, uint32_t events, IRQHandler &&handler);

	void Unregister(uint gpio);

private:
	static void HandleIRQ(uint gpio, uint32_t events);

	IRQManager();

	std::array<IRQHandler, 32> d_handlers;
};
