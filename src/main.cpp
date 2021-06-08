#include "ARM.h"
#include "MMIO.h"
#include "printf.h"
#include "Timer.h"
#include "UART.h"

using namespace Armaz;

void main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3) {
	(void) dtb_ptr32;
	(void) x1;
	(void) x2;
	(void) x3;

	UART::init();
	ARM::initVector();
	Timer::init(200000);
	MMIO::write(MMIO::ENABLE_IRQS_1, ARM::SYSTEM_TIMER_IRQ_1);
	ARM::enableIRQs();

	printf("Hello, world!\n");
	printf("Execution level: %d\n", ARM::getEL());

	uintptr_t addr = 0;

	for (;;) {
		unsigned char ch = UART::get();
		if ('0' <= ch && ch <= '9') {
			addr = addr * 16 + ch - '0';
			UART::put(ch);
		} else if ('a' <= ch && ch <= 'f') {
			addr = addr * 16 + 0xa + ch - 'a';
			UART::put(ch);
		} else if (ch == '\r') {
			printf("\n*0x%llx = ", addr);
			printf("0x%016llx / 0x%02x\n", *(uint64_t *) addr, *(uint8_t *) addr);
			addr = 0;
		} else if (ch == '!') {
			ARM::setMMU(!ARM::getMMU());
			printf("Virtual memory is now %s.\n", ARM::getMMU()? "enabled" : "disabled");
		} else {
			UART::put(ch);
		}
		asm volatile("wfe");
	}
}
