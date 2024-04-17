#pragma once

#include "pico/types.h"

class DRV8848 {
public:
	struct Config {
		uint nSleep, nFault, AIn1, AIn2, BIn1, BIn2;
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

	DRV8848(const DRV8848 &other)            = delete;
	DRV8848 &operator=(const DRV8848 &other) = delete;

	void SetEnabled(bool on);

	inline const Channel &A() const {
		return d_A;
	}

	inline const Channel &B() const {
		return d_B;
	}

	bool HasFault() const;

private:
	void nFaultIRQ(uint, uint32_t event);

	uint    d_nSleep, d_nFault;
	Channel d_A, d_B;
};
