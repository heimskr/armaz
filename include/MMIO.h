// Credit: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/include/peripherals/irq.h

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Armaz::MMIO {
	extern uintptr_t base;
	void init();
	uint32_t read(ptrdiff_t reg);
	void write(ptrdiff_t reg, uint32_t data);

	constexpr ptrdiff_t IRQ_BASIC_PENDING  = 0xb200;
	constexpr ptrdiff_t IRQ_PENDING_1      = 0xb204;
	constexpr ptrdiff_t IRQ_PENDING_2      = 0xb208;
	constexpr ptrdiff_t FIQ_CONTROL        = 0xb20c;
	constexpr ptrdiff_t ENABLE_IRQS_1      = 0xb210;
	constexpr ptrdiff_t ENABLE_IRQS_2      = 0xb214;
	constexpr ptrdiff_t ENABLE_BASIC_IRQS  = 0xb218;
	constexpr ptrdiff_t DISABLE_IRQS_1     = 0xb21c;
	constexpr ptrdiff_t DISABLE_IRQS_2     = 0xb220;
	constexpr ptrdiff_t DISABLE_BASIC_IRQS = 0xb224;
}

unsigned read32(uintptr_t addr);
void write32(uintptr_t addr, uint32_t data);
