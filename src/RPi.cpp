#include "RPi.h"

namespace Armaz::RPi {
	int getModel() {
		uint32_t reg;
		asm volatile("mrs %0, midr_el1" : "=r"(reg));
		switch ((reg >> 4) & 0xfff) {
			case 0xb76: return 1;
			case 0xc07: return 2;
			case 0xd03: return 3;
			case 0xd08: return 4;
			default:    return 0;
		}
	}

	void delay(int32_t count) {
		asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n" : "=r"(count): [count]"0"(count) : "cc");
	}
}
