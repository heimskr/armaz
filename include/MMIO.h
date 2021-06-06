#pragma once

#include <stdint.h>

namespace Armaz::MMIO {
	extern uintptr_t base;
	void init();
	uint32_t read(uintptr_t reg);
	void write(uintptr_t reg, uint32_t data);
}
