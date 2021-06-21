// Based on code from https://wiki.osdev.org/Raspberry_Pi_Bare_Bones

#include "ARM.h"
#include "MMIO.h"
#include "printf.h"
#include "RPi.h"

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
				case 4:  base = 0xfe000000; break;
				default: base = 0x20000000; break;
			}
		}
	}

	void write(ptrdiff_t reg, uint32_t data) {
		*(volatile uint32_t *) (base + reg) = data;
	}

	uint32_t read(ptrdiff_t reg) {
		return *(const volatile uint32_t *) (base + reg);
	}
}

unsigned read32(uintptr_t addr) {
	return *(volatile uint32_t *) addr;
}

void write32(uintptr_t addr, uint32_t data) {
	*(volatile uint32_t *) addr = data;
}
