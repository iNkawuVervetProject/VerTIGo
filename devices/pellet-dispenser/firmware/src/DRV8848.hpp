#pragma once

#include "pico/types.h"

class DRV8848 {
public:
	struct Config {
		uint nSleep, nFault, AIn1, AIn2, BIn1, BIn2;
	};

	DRV8848(const Config &config);

	void SetEnabled(bool on);
	void SetChannelA(int value);
	void SetChannelB(int value);

	bool HasFault() const;

private:
	class Channel {
	public:
		Channel(uint in1, uint in2);

		void Set(int value);

	private:
		uint d_slices[2], d_channels[2];
	};

	uint    d_nSleep, d_nFault;
	Channel d_A, d_B;
};
