// Credit: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/timer.c

#include "MMIO.h"
#include "printf.h"
#include "Timer.h"

namespace Armaz::Timer {
	unsigned interval;
	static unsigned currentValue = 0;

	void init(unsigned new_interval) {
		currentValue = MMIO::read(TIMER_CLO);
		currentValue += interval = new_interval;
		MMIO::write(TIMER_C1, currentValue);
	}

	void handle() {
		currentValue += interval;
		MMIO::write(TIMER_C1, currentValue);
		MMIO::write(TIMER_CS, TIMER_CS_M1);
		printf("Timer\n");
	}
}
