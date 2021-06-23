// Credit: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/include/peripherals/irq.h

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Armaz::MMIO {
	extern uintptr_t base;
	void init();

	inline uint32_t read(ptrdiff_t reg) {
		return *(const volatile uint32_t *) (base + reg);
	}

	inline void write(ptrdiff_t reg, uint32_t data) {
		*(volatile uint32_t *) (base + reg) = data;
	}

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

inline uint32_t read32(uintptr_t addr) {
	return *(volatile uint32_t *) addr;
}

inline void write32(uintptr_t addr, uint32_t data) {
	*(volatile uint32_t *) addr = data;
}
