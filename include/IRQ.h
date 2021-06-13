#pragma once

#include <stdint.h>

namespace Armaz {
	struct IRQ2711 {
		volatile uint32_t irq0Pending0;
		volatile uint32_t irq0Pending1;
		volatile uint32_t irq0Pending2;
		volatile uint32_t res0;
		volatile uint32_t irq0Enable0;
		volatile uint32_t irq0Enable1;
		volatile uint32_t irq0Enable2;
		volatile uint32_t res1;
		volatile uint32_t irq0Disable0;
		volatile uint32_t irq0Disable1;
		volatile uint32_t irq0Disable2;
	};

	struct IRQ2837 {
		volatile uint32_t irq0Pending0;
		volatile uint32_t irq0Pending1;
		volatile uint32_t irq0Pending2;
		volatile uint32_t fiq_control;
		volatile uint32_t irq0Enable1;
		volatile uint32_t irq0Enable2;
		volatile uint32_t irq0Enable0;
		volatile uint32_t res;
		volatile uint32_t irq0Disable1;
		volatile uint32_t irq0Disable2;
		volatile uint32_t irq0Disable0;
	};

#if RPI_VERSION == 4
	using IRQs = IRQ2711;
#else
	using IRQs = IRQ2837;
#endif
}
