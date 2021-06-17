#include "ARM.h"
#include "assert.h"
#include "BCM2836.h"
#include "GIC.h"
#include "GIC400.h"
#include "IRQ.h"
#include "MMIO.h"
#include "printf.h"
#include "PropertyTags.h"
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
	// Timers::timer.init();

	printf("Hello, world!\n");

	PropertyTagMemory mem;
	if (PropertyTags::getTag(PROPTAG_GET_ARM_MEMORY, &mem, sizeof(mem))) {
		printf("Base: 0x%x\n", mem.baseAddress);
		printf("Size: %u\n", mem.size);
	} else {
		printf("Reading failed.\n");
		printf("Base: 0x%x\n", mem.baseAddress);
		printf("Size: %u\n", mem.size);
	}

	for (;;) asm volatile("wfi");
}

extern "C" void main_secondary() {
	printf("main_secondary unimplemented!\n");
	for (;;) asm volatile("wfi");
}
