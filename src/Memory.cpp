#include "Memory.h"

// Credit: https://github.com/rsta2/circle/blob/master/lib/memory64.cpp

namespace Armaz::Memory {
	uintptr_t getCoherentPage(unsigned slot) {
		return MEM_COHERENT_REGION + slot * PAGE_SIZE;
	}
}
