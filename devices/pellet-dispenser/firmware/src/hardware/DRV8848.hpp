#pragma once

#include "pico/time.h"
#include "pico/types.h"

#include <functional>
#include <memory>

class DRV8848 {
public:
	const static uint    PWM_RESOLUTION        = (1 << 10);
	constexpr static int FAULT_ON_THRESHOLD_US = 300 * 1000; // 300ms

	struct Config {
		uint nSleep, nFault, AIn1, AIn2, BIn1, BIn2;
	};

	enum class OutputChannel {
		A = 0,
		B = 1,
	};

	class Channel {
	public:
		Channel(uint in1, uint in2);

		void Set(int value) const;

	private:
		uint d_slices[2], d_channels[2];
	};

	DRV8848(const Config &config);
	~DRV8848();

	void SetEnabled(bool on);

	inline const class Channel &Channel(OutputChannel c) const {
		if (c == OutputChannel::A) {
			return d_A;
		}
		return d_B;
	};

	inline const class Channel &A() const {
		return d_A;
	}

	inline const class Channel &B() const {
		return d_B;
	}

	bool HasFault() const;

private:
	void nFaultIRQ(uint, uint32_t event);

	std::unique_ptr<std::function<void()>> d_irqCallback;

	uint            d_nSleep, d_nFault;
	class Channel   d_A, d_B;
	absolute_time_t d_faultStart = nil_time;
};
