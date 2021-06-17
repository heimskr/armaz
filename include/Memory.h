#pragma once

#include <stdint.h>

#include "MemoryMap.h"

// Credit: https://github.com/rsta2/circle/blob/master/include/circle/memory.h

namespace Armaz::Memory {

	constexpr unsigned SLOT_PROP_MAILBOX = 0;
	constexpr unsigned SLOT_GPIO_VIRTBUF = 1;
	constexpr unsigned SLOT_TOUCHBUF     = 2;

	constexpr unsigned SLOT_VCHIQ_START = MEGABYTE / PAGE_SIZE / 2;
	constexpr unsigned SLOT_VCHIQ_END   = MEGABYTE / PAGE_SIZE - 1;

#if RASPPI >= 4
	constexpr unsigned SLOT_XHCI_START = MEGABYTE / PAGE_SIZE;
	constexpr unsigned SLOT_XHCI_END   = 4 * MEGABYTE / PAGE_SIZE - 1;
#endif

	uintptr_t getCoherentPage(unsigned slot);
}
