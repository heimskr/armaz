#include "pi/RPi.h"

namespace Armaz::RPi {
	int getModel() {
		uint32_t reg;
		asm volatile("mrs %x0, midr_el1" : "=r"(reg));
		switch ((reg >> 4) & 0xfff) {
			case 0xb76: return 1;
			case 0xc07: return 2;
			case 0xd03: return 3;
			case 0xd08: return 4;
			default:    return 0;
		}
	}
}
