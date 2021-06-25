#include "assert.h"
#include "Log.h"
#include "Memory.h"
#include "aarch64/ARM.h"
#include "aarch64/MMIO.h"
#include "aarch64/Timer.h"
#include "aarch64/Synchronize.h"
#include "board/BCM2836.h"
#include "board/BCM2711int.h"
#include "fs/tfat/ThornFAT.h"
#include "interrupts/GIC400.h"
#include "interrupts/IRQ.h"
#include "lib/printf.h"
#include "pi/GPIO.h"
#include "pi/MemoryMap.h"
#include "pi/PropertyTags.h"
#include "pi/RPi.h"
#include "pi/UART.h"
#include "storage/EMMC.h"
#include "storage/GPT.h"
#include "storage/MBR.h"
#include "storage/Partition.h"

#include <memory>
#include <string>
#include <vector>

using namespace Armaz;

extern "C" void main() {
	MMIO::init();
	// UART::init();

	write32(ARM_LOCAL_PRESCALER, 39768216u);

#if RASPPI == 4
	GIC400::init((void *) 0xff840000);
#endif

	extern void(*__init_start)();
	extern void(*__init_end)();
	for (void (**func)() = &__init_start; func < &__init_end; ++func)
		(**func)();

	Interrupts::init();
	UART::init();
	printf("Hello, world!\n");

	Memory::Allocator memory;
	memory.setBounds((char *) MEM_HIGHMEM_START, (char *) MEM_HIGHMEM_END);

	// Timers::timer.init();

	PropertyTagMemory mem;
	if (PropertyTags::getTag(PROPTAG_GET_ARM_MEMORY, &mem, sizeof(mem))) {
		Log::info("Base: 0x%x", mem.baseAddress);
		Log::info("Size: %u", mem.size);
	} else {
		Log::error("Reading memory failed.");
	}

	PropertyTagBoard board;
	if (PropertyTags::getTag(PROPTAG_GET_BOARD_REVISION, &board, sizeof(board))) {
		Log::info("Revision: 0x%x", board.board);
	} else {
		Log::error("Reading revision failed.");
	}

	if (PropertyTags::getTag(PROPTAG_GET_BOARD_MODEL, &board, sizeof(board))) {
		Log::info("Model: 0x%x", board.board);
	} else {
		Log::error("Reading model failed.");
	}

	EMMCDevice device;
	if (device.initialize()) {
		MBR mbr;
		if (device.read(&mbr, sizeof(mbr)) == -1ul) {
			Log::error("Failed to read from EMMCDevice.\n");
		} else {
			// mbr.debug();
			// Partition partition(device, mbr.thirdEntry);
			// auto driver = std::make_unique<ThornFAT::ThornFATDriver>(&partition);
			// if (driver->readdir("/", [&](const char *str, off_t offset) {
			// 	printf("- %s @ %lld\n", str, offset);
			// })) {
			// 	printf("readdir failed.\n");
			// } else {
			// 	printf("readdir succeeded.\n");
			// 	if (driver->create("/foo", 0666)) {
			// 		printf("create failed.\n");
			// 	} else {
			// 		printf("create succeeded.\n");
			// 		if (driver->readdir("/", [&](const char *str, off_t offset) {
			// 			printf("- %s @ %lld\n", str, offset);
			// 		})) {
			// 			printf("readdir failed.\n");
			// 		} else {
			// 			printf("readdir succeeded.\n");
			// 		}
			// 	}
			// }
		}
	} else
		Log::error("Failed to initialize EMMCDevice.");

	std::string input;
	char ch;

	for (;;) {
		while (UART::read(&ch, 1) == 1) {
			if (ch == '\n') {
				printf("\n\"%s\"\n", input.c_str());
				input.clear();
			} else if (ch == 127) {
				input.pop_back();
				printf("\e[D \e[D");
			} else {
				input += ch;
				UART::write(ch);
			}
		}

		asm volatile("wfi");
	}
}

extern "C" void main_secondary() {
	printf("main_secondary unimplemented!\n");
	for (;;) asm volatile("wfi");
}
