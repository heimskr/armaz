// Credit: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/include/peripherals/irq.h

#pragma once

#include <stdint.h>

namespace Armaz::ARM {
	int getEL();
	uint32_t getSctlr();
	void setSctlr(uint32_t);
	bool getMMU();
	void setMMU(bool enabled);
	void initVector();
	void initGIC();
	void enableIRQs();
	void disableIRQs();
	void invalidEntry(int type, unsigned long esr, unsigned long address);
	void handleIRQ();
	void handleFIQ();
	void handleInvalid();
	void delay(int32_t count);

	constexpr uint32_t SCTLR_MMU_ENABLED = 1;

	constexpr uint32_t SYSTEM_TIMER_IRQ_0 = 1 << 0;
	constexpr uint32_t SYSTEM_TIMER_IRQ_1 = 1 << 1;
	constexpr uint32_t SYSTEM_TIMER_IRQ_2 = 1 << 2;
	constexpr uint32_t SYSTEM_TIMER_IRQ_3 = 1 << 3;
}
