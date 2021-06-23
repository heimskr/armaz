#include "util.h"

extern "C" void * memmove(void *dest, const void *src, size_t n) {
	if (reinterpret_cast<uintptr_t>(dest) < reinterpret_cast<uintptr_t>(src))
		return memcpy(dest, src, n);
	
	// Let's just be lazy and do it byte by byte for now.
	char *destc = reinterpret_cast<char *>(dest);
	const char *srcc = reinterpret_cast<const char *>(src);
	
	for (size_t i = n; 0 < i; --i)
		destc[i - 1] = srcc[i - 1];

	return dest;
}
