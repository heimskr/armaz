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

void uart_handler(void *) {
	MMIO::write(UART::UART0_ICR, MMIO::read(UART::UART0_MIS));
	// *(volatile uint32_t *) 0xfe201044 = *(volatile uint32_t *) 0xfe201040;
	printf("\e[31mUART handler\e[39m: %c\n", UART::read());
}

extern "C" void main() {
	MMIO::init();
	// UART::init();


#if RASPPI == 4
	GIC400::init((void *) 0xff840000);
#endif


	MMIO::write(UART::UART0_IMSC, 0);
	MMIO::write(UART::UART0_ICR, 0x7ff);

	unsigned clockrate = 48000000;
	int baudrate = 115200;
	assert(300 <= baudrate && baudrate <= 4000000);
	unsigned baud64 = baudrate * 16;
	unsigned intdiv = clockrate / baud64;
	assert(1 <= intdiv && intdiv <= 0xFFFF);
	unsigned fractdiv2 = (clockrate % baud64) * 8 / baudrate;
	unsigned fractdiv = fractdiv2 / 2 + fractdiv2 % 2;
	assert(fractdiv <= 0x3F);

	MMIO::write(UART::UART0_IBRD, intdiv);
	MMIO::write(UART::UART0_FBRD, fractdiv);
	MMIO::write(UART::UART0_IFLS, 1 | (1 << 3));
	MMIO::write(UART::UART0_LCRH, (3 << 5) | (1 << 4));
	MMIO::write(UART::UART0_IMSC, (1 << 4) | (1 << 6) | (1 << 10));

	dataSyncBarrier();

	// *(volatile uint32_t *) 0xfe201038 = (1 << 4) | (1 << 6) | (1 << 10);
	// MMIO::write(UART::UART0_IMSC, 0b11111111111);
	// *(volatile uint32_t *) 0xfe201038 = 0b1111111111;
	// MMIO::write(UART::UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
	MMIO::write(UART::UART0_CR,   1 | (1 << 8) | (1 << 9));
	// printf("! 0x%llx\n", MMIO::base + UART::UART0_IMSC);


	printf("Hello, world!\n");

	Memory::Allocator memory;
	memory.setBounds((char *) MEM_HIGHMEM_START, (char *) MEM_HIGHMEM_END);

	write32(ARM_LOCAL_PRESCALER, 39768216u);

	extern void(*__init_start)();
	extern void(*__init_end)();
	for (void (**func)() = &__init_start; func < &__init_end; ++func)
		(**func)();
	printf("[%s:%d]\n", __FILE__, __LINE__);
	// MMIO::write(MMIO::ENABLE_IRQS_2, MMIO::read(MMIO::ENABLE_IRQS_2) | 0x02000000);
	printf("[%s:%d]\n", __FILE__, __LINE__);
	// *(volatile uint32_t *) 0xfe201038 = 0b1111111111;


	printf("[%s:%d]\n", __FILE__, __LINE__);
	// *(volatile uint32_t *) 0x7e201038 = ~0;
	printf("[%s:%d]\n", __FILE__, __LINE__);
	// Timers::timer.init();
	printf("[%s:%d]\n", __FILE__, __LINE__);
	Interrupts::init();
	printf("[%s:%d]\n", __FILE__, __LINE__);
	Interrupts::connect(ARM_IRQ_UART, uart_handler, nullptr);
	printf("[%s:%d]\n", __FILE__, __LINE__);

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

	for (;;) asm volatile("wfe");
}

extern "C" void main_secondary() {
	printf("main_secondary unimplemented!\n");
	for (;;) asm volatile("wfi");
}
