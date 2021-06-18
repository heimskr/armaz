#include "ARM.h"
#include "assert.h"
#include "BCM2836.h"
#include "GIC.h"
#include "GIC400.h"
#include "IRQ.h"
#include "Memory.h"
#include "MemoryMap.h"
#include "MMIO.h"
#include "printf.h"
#include "PropertyTags.h"
#include "RPi.h"
#include "Timer.h"
#include "UART.h"

#include <string>
#include <vector>

using namespace Armaz;

extern "C" void main() {
	MMIO::init();
	UART::init();
	Memory::Allocator memory;
	memory.setBounds((char *) MEM_HIGHMEM_START, (char *) MEM_HIGHMEM_END);

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

	std::string strings[10];
	for (int i = 0; i < 10; ++i)
		strings[i] = std::to_string(i * 1000);
	for (int i = 0; i < 10; ++i)
		printf("%d -> [%s] @ 0x%llx\n", i, strings[i].c_str(), strings[i].c_str());

	std::vector<std::string> strvec;
	for (int i = 0;; ++i) {
		printf("!!! %d\n", i);
		strvec.push_back("42");
		printf("... %d\n", i);
		printf("Foo(%s) @ 0x%llx\n", strvec.back().c_str(), strvec.back().c_str());
	}

	PropertyTagMemory mem;
	if (PropertyTags::getTag(PROPTAG_GET_ARM_MEMORY, &mem, sizeof(mem))) {
		printf("Base: 0x%x\n", mem.baseAddress);
		printf("Size: %u\n", mem.size);
	} else {
		printf("Reading memory failed.\n");
	}

	PropertyTagBoard board;
	if (PropertyTags::getTag(PROPTAG_GET_BOARD_REVISION, &board, sizeof(board))) {
		printf("Revision: 0x%x\n", board.board);
	} else {
		printf("Reading revision failed.\n");
	}

	if (PropertyTags::getTag(PROPTAG_GET_BOARD_MODEL, &board, sizeof(board))) {
		printf("Model: 0x%x\n", board.board);
	} else {
		printf("Reading model failed.\n");
	}

	for (;;) asm volatile("wfi");
}

extern "C" void main_secondary() {
	printf("main_secondary unimplemented!\n");
	for (;;) asm volatile("wfi");
}
