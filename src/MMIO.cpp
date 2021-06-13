// Based on code from https://wiki.osdev.org/Raspberry_Pi_Bare_Bones

#include "ARM.h"
#include "MMIO.h"
#include "RPi.h"

namespace Armaz::MMIO {
	uintptr_t base = 0;
	static bool initialized = false;

	void init() {
		if (!initialized) {
			initialized = true;
			base = 0xfe000000;
			// switch (RPi::getModel()) {
			// 	case 1:  base = 0x20000000; break;
			// 	case 2:  base = 0x3f000000; break;
			// 	case 3:  base = 0x3f000000; break;
			// 	case 4:  base = 0xfe000000; break;
			// 	default: base = 0x20000000; break;
			// }
		}
	}

	void write(uintptr_t reg, uint32_t data) {
		*(volatile uint32_t *) (base + reg) = data;
	}

	uint32_t read(uintptr_t reg) {
		return *(volatile uint32_t *) (base + reg);
	}
}
