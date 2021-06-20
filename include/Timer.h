#pragma once

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

#include <stddef.h>
#include <stdint.h>

namespace Armaz::Timers {
	/** When set to 1000, the timer seems to run at 18.5 Hz. This means that this value should be about 54 times the
	 *  actual desired frequency. Probably has something to do with the clock's 54 MHz frequency. */
	constexpr unsigned HZ = 540;

	constexpr unsigned CLOCKHZ = 1'000'000;

	/** See the documentation for the ARM side timer (Section 14 of the BCM2835 Peripherals PDF) */
	constexpr ptrdiff_t ARMTIMER_BASE = 0xb400;

	/** 0: 16-bit counters; 1: 23-bit counter */
	constexpr int ARMTIMER_CTRL_23BIT = 1 << 1;

	constexpr int ARMTIMER_CTRL_PRESCALE_1   = 0 << 2;
	constexpr int ARMTIMER_CTRL_PRESCALE_16  = 1 << 2;
	constexpr int ARMTIMER_CTRL_PRESCALE_256 = 2 << 2;

	/** 0: Timer interrupt disabled; 1: Timer interrupt enabled */
	constexpr int ARMTIMER_CTRL_INT_ENABLE  = 1 << 5;
	constexpr int ARMTIMER_CTRL_INT_DISABLE = 0 << 5;

	/** 0: Timer disabled; 1: Timer enabled */
	constexpr int ARMTIMER_CTRL_ENABLE  = 1 << 7;
	constexpr int ARMTIMER_CTRL_DISABLE = 0 << 7;

	inline void waitCycles(size_t count) {
		if (!count)
			return;
		while (count--)
			asm volatile("nop");
	}

	uint64_t getSystemTimer();
	void waitMicroseconds(size_t);
	unsigned getClockTicks();

	class Timer {
		private:
			unsigned clockTicksPerHzTick;
			bool connected = false;
			// void tuneMsDelay();
			// volatile unsigned ticks;

		public:
			Timer() {}
			void init(bool calibrate = false);
			void handler();
			static void handler(void *);
			void disconnect();
	};

	extern Timer timer;

	constexpr ptrdiff_t TIMER_CS  = 0x3000;
	constexpr ptrdiff_t TIMER_CLO = 0x3004;
	constexpr ptrdiff_t TIMER_CHI = 0x3008;
	constexpr ptrdiff_t TIMER_C0  = 0x300c;
	constexpr ptrdiff_t TIMER_C1  = 0x3010;
	constexpr ptrdiff_t TIMER_C2  = 0x3014;
	constexpr ptrdiff_t TIMER_C3  = 0x3018;

	constexpr uint32_t TIMER_CS_M0 = 1 << 0;
	constexpr uint32_t TIMER_CS_M1 = 1 << 1;
	constexpr uint32_t TIMER_CS_M2 = 1 << 2;
	constexpr uint32_t TIMER_CS_M3 = 1 << 3;
}
