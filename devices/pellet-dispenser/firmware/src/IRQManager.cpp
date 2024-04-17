#include "IRQManager.hpp"
#include "hardware/gpio.h"

void IRQManager::Register(uint gpio, uint32_t events, IRQHandler &&handler) {

	if (gpio >= 32) {
		return;
	}

	gpio_set_irq_enabled(gpio, events, true);
	d_handlers[gpio] = std::move(handler);
}

void IRQManager::Unregister(uint gpio) {
	if (gpio >= 32) {
		return;
	}

	gpio_set_irq_enabled(
	    gpio,
	    GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE | GPIO_IRQ_LEVEL_HIGH |
	        GPIO_IRQ_LEVEL_HIGH,
	    false
	);
	d_handlers[gpio] = [](uint, uint32_t) {};
}

void IRQManager::HandleIRQ(uint gpio, uint32_t events) {
	Instance().d_handlers[gpio](gpio, events);
}

IRQManager::IRQManager() {
	gpio_set_irq_callback(&IRQManager::HandleIRQ);
}
