#include "ARM.h"
#include "MMIO.h"
#include "printf.h"

namespace Armaz::ARM {
	int getEL() {
		int reg;
		asm volatile("mrs %0, CurrentEL" : "=r"(reg));
		return reg >> 2;
	}

	uint32_t getSctlr() {
		uint32_t reg;
		asm volatile("mrs %0, sctlr_el1" : "=r"(reg));
		return reg;
	}

	bool getMMU() {
		return (getSctlr() & SCTLR_MMU_ENABLED) == SCTLR_MMU_ENABLED;
	}

	void setMMU(bool enabled) {
		uint32_t sctlr = getSctlr();
		if (enabled)
			sctlr |= SCTLR_MMU_ENABLED;
		else
			sctlr &= ~SCTLR_MMU_ENABLED;
		setSctlr(sctlr);
	}

	void setSctlr(uint32_t value) {
		asm volatile("msr sctlr_el1, %0" :: "r"(value));
	}

	void invalidEntry(uint64_t n) {
		printf("Invalid entry: %llu\n", n);
	}

	void handleIRQ() {
		unsigned irq = MMIO::read(MMIO::IRQ_PENDING_1);
		printf("IRQ: %u\n", irq);
	}

	void delay(int32_t count) {
		asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n" : "=r"(count) : [count] "0"(count) : "cc");
	}
}
