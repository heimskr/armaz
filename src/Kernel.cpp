#include "Kernel.h"
#include "lib/printf.h"
#include "pi/UART.h"

namespace Armaz::Kernel {
	void __attribute__((noreturn)) panic(const char *format, ...) {
		UART::write("\e[2m[\e[22;31mPANIC\e[39;2m]\e[22m ");
		va_list var;
		va_start(var, format);
		vprintf(format, var);
		va_end(var);
		UART::write('\n');
		perish();
	}

	void __attribute__((noreturn)) perish() {
		for (;;) // TODO: ensure all cores are halted
			asm volatile("wfi");
	}
}
