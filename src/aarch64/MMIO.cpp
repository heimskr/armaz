// Based on code from https://wiki.osdev.org/Raspberry_Pi_Bare_Bones

#include "aarch64/ARM.h"
#include "aarch64/MMIO.h"
#include "lib/printf.h"
#include "pi/RPi.h"

namespace Armaz::MMIO {
	uintptr_t base = 0;
	static bool initialized = false;

	void init() {
		if (!initialized) {
			initialized = true;
			switch (RPi::getModel()) {
				case 1:  base = 0x20000000; break;
				case 2:  base = 0x3f000000; break;
				case 3:  base = 0x3f000000; break;
#ifdef HIGH_PERIPHERAL_MODE
				case 4:  base = 0x47e000000; break;
#else
				case 4:  base = 0xfe000000; break;
#endif
				default: base = 0x20000000; break;
			}
		}
	}
}
