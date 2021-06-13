#include "ARM.h"
#include "GIC.h"
#include "IRQ.h"
#include "MMIO.h"
#include "printf.h"
#include "Timer.h"
#include "UART.h"

using namespace Armaz;

extern "C" void main() {
	UART::init();

	printf("Hello, world!\n");
	printf("Execution level: %d\n", ARM::getEL());


	// for (unsigned i = 0;; ++i) printf("%u", i % 10);

	uintptr_t addr = 0;

	for (;;) {
		UART::drain();
		unsigned char ch = UART::read();
		if ('0' <= ch && ch <= '9') {
			addr = addr * 16 + ch - '0';
			UART::write(ch);
		} else if ('a' <= ch && ch <= 'f') {
			addr = addr * 16 + 0xa + ch - 'a';
			UART::write(ch);
		} else if (ch == '\r') {
			if (addr % 8) {
				printf("\nUnaligned address: 0x%llx\n", addr);
			} else {
				printf("\n*0x%llx = ", addr);
				printf("0x%016llx / 0x%02x\n", *(uint64_t *) addr, *(uint8_t *) addr);
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
}
