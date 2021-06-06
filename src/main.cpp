#include "MMIO.h"
#include "UART.h"

using namespace Armaz;

void main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3) {
	(void) dtb_ptr32;
	(void) x1;
	(void) x2;
	(void) x3;

	UART::init();
	UART::put("Hello, world!\n");

	for (;;) {
		UART::put(UART::get());
		asm volatile("wfe");
	}
}
