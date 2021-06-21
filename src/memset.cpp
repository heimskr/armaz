/* Copyright (C) 1991-2020 Free Software Foundation, Inc.
   This file is derived from the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

// #include "memory/memset.h"
// #include "Kernel.h"

#include <stddef.h>
#include <stdint.h>

#define CAREFUL_MEMSET

using op_t = unsigned long long int;

extern "C" void * memset(void *dstpp, int c, size_t len) {
#ifdef CAREFUL_MEMSET
	char *dest = reinterpret_cast<char *>(dstpp);
	while (reinterpret_cast<uintptr_t>(dest) % 8 && len) {
		*dest++ = c;
		--len;
	}

	if (8 <= len) {
		size_t c8 = c;
		c8 |= c8 << 8;
		c8 |= c8 << 16;
		c8 |= c8 << 32;
		uint64_t *dest8 = reinterpret_cast<uint64_t *>(dest);
		while (8 <= len) {
			*dest8++ = c8;
			len -= 8;
		}

		dest = reinterpret_cast<char *>(dest8);
	}

	for (size_t i = 0; i < len; ++i)
		*dest++ = c;
#else
	long int dstp = (long int) dstpp;
	if (len >= 8) {
		size_t xlen;
		op_t cccc;

		cccc = (unsigned char) c;
		cccc |= cccc << 8;
		cccc |= cccc << 16;
		if (sizeof(op_t) > 4)
			// Do the shift in two steps to avoid warning if long has 32 bits.
			cccc |= (cccc << 16) << 16;

		// There are at least some bytes to set. No need to test for LEN == 0 in this alignment loop.
		while (dstp % sizeof(op_t) != 0) {
			((char *) dstp)[0] = c;
			++dstp;
			--len;
		}

		// Write 8 `op_t' per iteration until less than 8 `op_t' remain.
		xlen = len / (sizeof(op_t) * 8);
		while (xlen > 0) {
			((op_t *) dstp)[0] = cccc;
			((op_t *) dstp)[1] = cccc;
			((op_t *) dstp)[2] = cccc;
			((op_t *) dstp)[3] = cccc;
			((op_t *) dstp)[4] = cccc;
			((op_t *) dstp)[5] = cccc;
			((op_t *) dstp)[6] = cccc;
			((op_t *) dstp)[7] = cccc;
			dstp += 8 * sizeof(op_t);
			--xlen;
		}

		len %= sizeof(op_t) * 8;

		// Write 1 `op_t' per iteration until less than sizeof(op_t) bytes remain.
		xlen = len / sizeof(op_t);
		while (xlen > 0) {
			((op_t *) dstp)[0] = cccc;
			dstp += sizeof(op_t);
			--xlen;
		}

		len %= sizeof(op_t);
	}

	// Write the last few bytes.
	while (len > 0) {
		((char *) dstp)[0] = c;
		++dstp;
		--len;
	}
#endif
	return dstpp;
}