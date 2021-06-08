#pragma once

// Credit: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/include/peripherals/timer.h

#include <stdint.h>

namespace Armaz::Timer {
	extern unsigned interval;

	void init(unsigned new_interval);
	void handle();

	constexpr uintptr_t TIMER_CS  = 0x3000;
	constexpr uintptr_t TIMER_CLO = 0x3004;
	constexpr uintptr_t TIMER_CHI = 0x3008;
	constexpr uintptr_t TIMER_C0  = 0x300c;
	constexpr uintptr_t TIMER_C1  = 0x3010;
	constexpr uintptr_t TIMER_C2  = 0x3014;
	constexpr uintptr_t TIMER_C3  = 0x3018;

	constexpr uint32_t TIMER_CS_M0 = 1 << 0;
	constexpr uint32_t TIMER_CS_M1 = 1 << 1;
	constexpr uint32_t TIMER_CS_M2 = 1 << 2;
	constexpr uint32_t TIMER_CS_M3 = 1 << 3;
}
