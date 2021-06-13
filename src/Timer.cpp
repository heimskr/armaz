// Credit: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/timer.c

#include "MMIO.h"
#include "printf.h"
#include "Timer.h"

namespace Armaz::Timer {
	unsigned interval;
	static unsigned currentValue = 0;

	MMTimerModule *counter = (MMTimerModule *) counterBase;

	void init(unsigned new_interval) {
		currentValue = MMIO::read(TIMER_CLO);
		currentValue += interval = new_interval;
		MMIO::write(TIMER_C1, currentValue);
	}

	void initCounter(bool hdbg, uint32_t frequency, bool scaling) {
		counter->cntcr = 1 | (hdbg << 1) | (scaling << 2) | ((0x3ff & frequency) << 8);
	}

	SetScalingResult setScalingFactor(uint32_t scale) {
		if ((counter->cntcr & 1) == 1)
			return SetScalingResult::CounterEnabled;

		if ((counter->cntid & 0xf) == 0)
			return SetScalingResult::Unsupported;

		counter->cntscr = scale;
		return SetScalingResult::Success;
	}

	void handle() {
		currentValue += interval;
		MMIO::write(TIMER_C1, currentValue);
		MMIO::write(TIMER_CS, TIMER_CS_M1);
		printf("Timer\n");
	}
}
