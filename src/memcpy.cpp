#include <stddef.h>
#include <stdint.h>

#include "util.h"

extern "C" void * memcpy(void *dest, const void *src, size_t n) {
	uintptr_t destp = reinterpret_cast<uintptr_t>(dest);
	uintptr_t srcp  = reinterpret_cast<uintptr_t>(src);
	char *destc = reinterpret_cast<char *>(dest);
	const char *srcc = reinterpret_cast<const char *>(src);

	if (n == 0)
		return dest;

	if (destp % 8 != 0 && destp % 8 == srcp % 8) {
		while (destp++ % 8) {
			if (n-- == 0)
				return dest;
			*destc++ = *srcc++;
		}
	} else if (destp % 4 != 0 && destp % 4 == srcp % 4) {
		while (destp++ % 4) {
			if (n-- == 0)
				return dest;
			*destc++ = *srcc++;
		}
	} else if (destp % 2 != 0 && destp % 2 == srcp % 2) {
		while (destp++ % 2) {
			if (n-- == 0)
				return dest;
			*destc++ = *srcc++;
		}
	}

	if (n == 0)
		return dest;

	if (destp % 8 == 0 && srcp % 8 == 0) {
		uint64_t *dest8 = reinterpret_cast<uint64_t *>(destc);
		const uint64_t *src8 = reinterpret_cast<const uint64_t *>(srcc);
		for (; 8 <= n; n -= 8)
			*dest8++ = *src8++;
		destc = reinterpret_cast<char *>(dest8);
		srcc  = reinterpret_cast<const char *>(src8);
	} else if (destp % 4 == 0 && srcp % 4 == 0) {
		uint32_t *dest4 = reinterpret_cast<uint32_t *>(destc);
		const uint32_t *src4 = reinterpret_cast<const uint32_t *>(srcc);
		for (; 4 <= n; n -= 4)
			*dest4++ = *src4++;
		destc = reinterpret_cast<char *>(dest4);
		srcc  = reinterpret_cast<const char *>(src4);
	} else if (destp % 2 == 0 && srcp % 2 == 0) {
		uint16_t *dest2 = reinterpret_cast<uint16_t *>(destc);
		const uint16_t *src2 = reinterpret_cast<const uint16_t *>(srcc);
		for (; 2 <= n; n -= 2)
			*dest2++ = *src2++;
		destc = reinterpret_cast<char *>(dest2);
		srcc  = reinterpret_cast<const char *>(src2);
	}

	for (size_t i = 0; i < n; ++i)
		*destc++ = *srcc++;
	
	return dest;
}
