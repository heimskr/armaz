#include "ARM.h"
#include "assert.h"
#include "BCM2836.h"
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

	write32(ARM_LOCAL_PRESCALER, 39768216u);

	extern void (*__init_start) (void);
	extern void (*__init_end) (void);
	for (void (**func)(void) = &__init_start; func < &__init_end; ++func)
		(**func)();

#if RASPPI == 4
	GIC400::init((void *) 0xff840000);
#endif

	Interrupts::init();
	Timers::timer.init();

	printf("Hello, world!\n");

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

	for (;;) asm volatile("wfi");
}

extern "C" void main_secondary() {
	printf("main_secondary unimplemented!\n");
	for (;;) asm volatile("wfi");
}