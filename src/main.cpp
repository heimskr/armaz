#include "ARM.h"
#include "MMIO.h"
#include "UART.h"
#include "printf.h"

using namespace Armaz;

void main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3) {
	(void) dtb_ptr32; (void) x1; (void) x2; (void) x3;

	UART::init();
	printf("Hello, world!\n");
	printf("Execution level: %d\n", ARM::getEL());

	uintptr_t n = 0;

	for (;;) {
		unsigned char ch = UART::get();
		if ('0' <= ch && ch <= '9') {
			n = n * 16 + ch - '0';
			UART::put(ch);
		} else if ('a' <= ch && ch <= 'f') {
			n = n * 16 + 0xa + ch - 'a';
			UART::put(ch);
		} else if (ch == '\r') {
			printf("\n*0x%llx = ", n);
			printf("0x%016llx / 0x%02x\n", *(uint64_t *) n, *(uint8_t *) n);
			n = 0;
		} else if (ch == '!') {
			ARM::setMMU(!ARM::getMMU());
			printf("Virtual memory is now %s.\n", ARM::getMMU()? "enabled" : "disabled");
		} else {
			UART::put(ch);
		}
		asm volatile("wfe");
	}
}
