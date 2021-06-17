//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdint.h>

#include "Synchronize.h"

namespace Armaz {
	void dataMemBarrier() {
		asm volatile("dmb sy" ::: "memory");
	}

	void dataSyncBarrier() {
		asm volatile("dsb sy" ::: "memory");
	}

	void cleanDataCache() {
		// clean L1 data cache
		for (unsigned nset = 0; nset < L1_DATA_CACHE_SETS; ++nset)
			for (unsigned nway = 0; nway < L1_DATA_CACHE_WAYS; ++nway) {
				uint64_t nsetWayLevel =
					nway << L1_SETWAY_WAY_SHIFT | nset << L1_SETWAY_SET_SHIFT | 0 << SETWAY_LEVEL_SHIFT;
				asm volatile ("dc csw, %0" :: "r"(nsetWayLevel) : "memory");
			}

		// clean L2 unified cache
		for (unsigned nset = 0; nset < L2_CACHE_SETS; ++nset)
			for (unsigned nway = 0; nway < L2_CACHE_WAYS; ++nway) {
				uint64_t nsetWayLevel =
					nway << L2_SETWAY_WAY_SHIFT | nset << L2_SETWAY_SET_SHIFT | 1 << SETWAY_LEVEL_SHIFT;
				asm volatile ("dc csw, %0" : : "r" (nsetWayLevel) : "memory");
			}

		dataSyncBarrier();
	}

	void invalidateInstructionCache() {
		asm volatile("ic iallu" ::: "memory");
	}

	void instructionSyncBarrier() {
		asm volatile("isb" ::: "memory");
	}

	void syncDataAndInstructionCache() {
		cleanDataCache();

		invalidateInstructionCache();
		dataSyncBarrier();

		instructionSyncBarrier();
	}
}
