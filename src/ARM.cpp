#include "ARM.h"

namespace Armaz::ARM {
	int getEL() {
		int reg;
		asm volatile("mrs %0, CurrentEL" : "=r"(reg));
		return reg >> 2;
	}

	void delay(int32_t count) {
		asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n" : "=r"(count): [count]"0"(count) : "cc");
	}
}
