#include "ARM.h"
#include "GIC.h"
#include "GIC400.h"
#include "IRQ.h"
#include "MMIO.h"
#include "printf.h"
#include "RPi.h"
#include "Timer.h"
#include "UART.h"

using namespace Armaz;

extern "C" void main() {
	MMIO::init();
	UART::init();

	// IRQs *irqs = (IRQs *) (MMIO::base + MMIO::IRQ_BASIC_PENDING);

#if RASPPI == 4
	GIC400::init((void *) 0xff840000);
#endif

	Interrupts::init();



	// MMIO::write(MMIO::ENABLE_IRQS_1, ARM::SYSTEM_TIMER_IRQ_0);
	// MMIO::write(MMIO::ENABLE_BASIC_IRQS, ARM::SYSTEM_TIMER_IRQ_0);
	// irqs->irq0Enable0 = 1 << 29;
	// uint32_t freq;
	// asm volatile("mrs %0, cntfrq_el0" : "=r" (freq));
	// asm volatile("msr cntv_tval_el0, %0" :: "r"(freq));
	// asm volatile("msr cntv_ctl_el0, %0" :: "r"(1));
	// Timer::init(200000);
	ARM::enableIRQs();
	// Timer::init(2);

	printf("Hello, world!\n");
	printf("Execution level: %d\n", ARM::getEL());
	printf("vectors: 0x%llx\n", &vectors);


	/*
	uintptr_t addr = 0;
	for (;;) {
		unsigned char ch = UART::read();
		if ('0' <= ch && ch <= '9') {
			addr = addr * 16 + ch - '0';
			UART::write(ch);
		} else if ('a' <= ch && ch <= 'f') {
			addr = addr * 16 + 0xa + ch - 'a';
			UART::write(ch);
		} else if (ch == '\r') {
			if (addr % 8) {
				printf("\nUnaligned address: 0x%llx -> 0x%02x\n", addr, *(volatile uint8_t *) addr);
			} else {
				printf("\n*0x%llx = ", addr);
				printf("0x%016llx / 0x%02x\n", *(volatile uint64_t *) addr, *(volatile uint8_t *) addr);
			}
			addr = 0;
		} else if (ch == '!') {
			ARM::setMMU(!ARM::getMMU());
			printf("Virtual memory is now %s.\n", ARM::getMMU()? "enabled" : "disabled");
		} else {
			UART::write(ch);
		}
		// asm volatile("wfe");
	}
	//*/
	for (;;) asm volatile("wfe");
}
