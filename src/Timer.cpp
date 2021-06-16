/* Copyright 2013–2018 Brian Sidebotham <brian.sidebotham@gmail.com>
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "assert.h"
#include "BCM2711int.h"
#include "IRQ.h"
#include "MMIO.h"
#include "printf.h"
#include "Timer.h"

namespace Armaz::Timers {
	Timer timer;

	void Timer::init(bool /* calibrate */) {
		if (connected)
			return;
		connected = true;
		Interrupts::connect(ARM_IRQLOCAL0_CNTPNS, handler, this);
		uint64_t cntfrq;
		asm volatile("mrs %0, CNTFRQ_EL0" : "=r"(cntfrq));
		printf("cntfrq = %llu\n", cntfrq);
		assert(cntfrq % HZ == 0);
		clockTicksPerHzTick = cntfrq / HZ;

		uint64_t cntpct;
		asm volatile("mrs %0, CNTPCT_EL0" : "=r"(cntpct));
		asm volatile("msr CNTP_CVAL_EL0, %0" :: "r"(cntpct + clockTicksPerHzTick));
		asm volatile("msr CNTP_CTL_EL0, %0" :: "r"(1));
	}

	void Timer::handler() {
		static unsigned i = 0;
		printf("Timer. %u\n", ++i);
		uint64_t cval;
		asm volatile("mrs %0, CNTP_CVAL_EL0" : "=r"(cval));
		asm volatile("msr CNTP_CVAL_EL0, %0" :: "r"(cval + clockTicksPerHzTick));
	}

	void Timer::handler(void *param) {
		assert(param);
		((Timer *) param)->handler();
	}

	void Timer::disconnect() {
		if (!connected)
			return;
		Interrupts::disconnect(ARM_IRQLOCAL0_CNTPNS);
		connected = false;
	}
}
