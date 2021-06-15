// Credit: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/include/peripherals/irq.h

#pragma once

#include <stdint.h>

namespace Armaz::MMIO {
	extern uintptr_t base;
	void init();
	uint32_t read(uintptr_t reg);
	void write(uintptr_t reg, uint32_t data);

	constexpr uintptr_t IRQ_BASIC_PENDING  = 0xb200;
	constexpr uintptr_t IRQ_PENDING_1      = 0xb204;
	constexpr uintptr_t IRQ_PENDING_2      = 0xb208;
	constexpr uintptr_t FIQ_CONTROL        = 0xb20c;
	constexpr uintptr_t ENABLE_IRQS_1      = 0xb210;
	constexpr uintptr_t ENABLE_IRQS_2      = 0xb214;
	constexpr uintptr_t ENABLE_BASIC_IRQS  = 0xb218;
	constexpr uintptr_t DISABLE_IRQS_1     = 0xb21c;
	constexpr uintptr_t DISABLE_IRQS_2     = 0xb220;
	constexpr uintptr_t DISABLE_BASIC_IRQS = 0xb224;
}

void write32(uintptr_t addr, uint32_t data);
