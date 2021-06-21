#include "assert.h"
#include "lib/printf.h"

void assertion_failed(const char *expr, const char *file, unsigned line) {
	printf("Assertion failed on %s:%u: %s\n", file, line, expr);
	for (;;)
		asm volatile("wfi");
}
