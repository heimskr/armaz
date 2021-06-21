#include "ARM.h"
#include "MMIO.h"
#include "printf.h"

namespace Armaz::ARM {
	int getEL() {
		int reg;
		asm volatile("mrs %x0, CurrentEL" : "=r"(reg));
		return reg >> 2;
	}

	uint32_t getSctlr() {
		uint32_t reg;
		asm volatile("mrs %x0, sctlr_el1" : "=r"(reg));
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
		asm volatile("msr sctlr_el1, %x0" :: "r"(value));
	}

	void invalidEntry(int type, unsigned long esr, unsigned long address) {
		printf("Invalid entry: %d, %lx, %lx\n", type, esr, address);
	}

	void handleIRQ() {
		unsigned irq = MMIO::read(MMIO::IRQ_PENDING_1);
		printf("IRQ: %u\n", irq);
	}

	void handleFIQ() {
		// unsigned irq = MMIO::read(MMIO::IRQ_PENDING_1);
		printf("FIQ\n");
	}

	void handleInvalid() {
		printf("Invalid\n");
	}

	void delay(int32_t count) {
		asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n" : "=r"(count) : [count] "0"(count) : "cc");
	}
}
