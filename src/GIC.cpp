// ----------------------------------------------------------
// GICv3 Helper Functions (basic)
//
// Copyright (C) Arm Limited, 2019 All rights reserved.
//
// The example code is provided to you as an aid to learning when working
// with Arm-based technology, including but not limited to programming tutorials.
// Arm hereby grants to you, subject to the terms and conditions of this Licence,
// a non-exclusive, non-transferable, non-sub-licensable, free-of-charge licence,
// to use and copy the Software solely for the purpose of demonstration and
// evaluation.
//
// You accept that the Software has not been tested by Arm therefore the Software
// is provided �as is�, without warranty of any kind, express or implied. In no
// event shall the authors or copyright holders be liable for any claim, damages
// or other liability, whether in action or contract, tort or otherwise, arising
// from, out of or in connection with the Software or the use of Software.
//
// ------------------------------------------------------------

// #include <stdio.h>
// #include <string.h>

#include "GIC.h"
#include "printf.h"

namespace Armaz::GIC {
	volatile DistIf  *gicDist  = nullptr;
	volatile RdistIf *gicRdist = nullptr;

	static bool gicAddrValid = false;
	static uint32_t gicMaxRd = 0;


	constexpr uintptr_t GICD_BASE = 0x8000000;
	constexpr uintptr_t GICC_BASE = 0x8010000;
	volatile uint32_t *GICD_CTLR = (volatile uint32_t *) GICD_BASE;
	volatile uint32_t *GICD_ISENABLER = (volatile uint32_t *) (GICD_BASE + 0x0100);
	volatile uint32_t *GICC_CTLR = (volatile uint32_t *) GICC_BASE;
	volatile uint32_t *GICC_PMR = (volatile uint32_t *) (GICC_BASE + 0x0004);
	volatile uint32_t *GICC_BPR = (volatile uint32_t *) (GICC_BASE + 0x0008);
	
	void setGICAddr(void *dist, void *rdist) {
		uint32_t index = 0;

		gicDist  = (DistIf  *) dist;
		gicRdist = (RdistIf *) rdist;
		gicAddrValid = true;

		// Now find the maximum RD ID that can be used. This is used for range checking in later functions.
		while ((gicRdist[index].lpis.GICR_TYPER[0] & (1 << 4)) == 0 && index < 1024) {
			// Keep incrementing until GICR_TYPER.Last reports no more RDs in block
			++index;
		}

		gicMaxRd = index;
	}

	uint32_t initGIC() {
		uint32_t rd;

		printf("[%s:%d]\n", __FILE__, __LINE__);
		// Set location of GIC
		setGICAddr((void *) 0x2f000000, (void *) 0x2f100000);
		printf("[%s:%d]\n", __FILE__, __LINE__);

		// Enable GIC
		enableGIC();
		printf("[%s:%d]\n", __FILE__, __LINE__);

		// Get the ID of the Redistributor connected to this PE
		rd = getRedistID(getAffinity());
		printf("[%s:%d]\n", __FILE__, __LINE__);

		// Mark this core as beign active
		wakeUpRedist(rd);
		printf("[%s:%d]\n", __FILE__, __LINE__);

		// Configure the CPU interface
		// This assumes that the SRE bits are already set
		setPriorityMask(0xff);
		printf("[%s:%d]\n", __FILE__, __LINE__);
		enableGroup0Ints();
		printf("[%s:%d]\n", __FILE__, __LINE__);
		enableGroup1Ints();
		printf("[%s:%d]\n", __FILE__, __LINE__);

		return rd;
	}

	bool enableGIC() {
		// Check that GIC pointers are valid
		if (!gicAddrValid)
			return true;

		// First set the ARE bits
		gicDist->GICD_CTLR = (1 << 5) | (1 << 4);

		// The split here is because the register layout is different once ARE==1

		// Now set the rest of the options
		gicDist->GICD_CTLR = 7 | (1 << 5) | (1 << 4);
		return false;
	}

	uint32_t getRedistID(uint32_t affinity) {
		uint32_t index = 0;

		// Check that GIC pointers are valid
		if (!gicAddrValid)
			return 0xffffffff;

		do {
			if (gicRdist[index].lpis.GICR_TYPER[1] == affinity)
				return index;
		} while (++index <= gicMaxRd);

		return 0xffffffff; // return -1 to signal not RD found
	}

	bool wakeUpRedist(uint32_t rd) {
		uint32_t tmp;

		// Check that GIC pointers are valid
		if (!gicAddrValid)
			return true;

		// Tell the Redistributor to wake-up by clearing ProcessorSleep bit
		tmp = gicRdist[rd].lpis.GICR_WAKER;
		tmp = tmp & ~2;
		gicRdist[rd].lpis.GICR_WAKER = tmp;

		// Poll ChildrenAsleep bit until re-distributor wakes
		do {
			tmp = gicRdist[rd].lpis.GICR_WAKER;
		} while ((tmp & 4) != 0);

		return false;
	}
}
