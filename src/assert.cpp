#include "assert.h"
#include "Kernel.h"

void assertion_failed(const char *expr, const char *file, unsigned line) {
	Armaz::Kernel::panic("Assertion failed on %s:%u: %s\n", file, line, expr);
}
