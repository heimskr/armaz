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

#include "assert.h"
#include "Config.h"
#include "aarch64/Synchronize.h"
#include "interrupts/IRQ.h"
#include "pi/MemoryMap.h"

namespace Armaz {
	static constexpr uint32_t MAX_CRITICAL_LEVEL = 20;

	void cleanDataCache() {
		// clean L1 data cache
		for (uint32_t nset = 0; nset < L1_DATA_CACHE_SETS; ++nset)
			for (uint32_t nway = 0; nway < L1_DATA_CACHE_WAYS; ++nway) {
				uint64_t nsetWayLevel =
					nway << L1_SETWAY_WAY_SHIFT | nset << L1_SETWAY_SET_SHIFT | 0 << SETWAY_LEVEL_SHIFT;
				asm volatile ("dc csw, %0" :: "r"(nsetWayLevel) : "memory");
			}

		// clean L2 unified cache
		for (uint32_t nset = 0; nset < L2_CACHE_SETS; ++nset)
			for (uint32_t nway = 0; nway < L2_CACHE_WAYS; ++nway) {
				uint64_t nsetWayLevel =
					nway << L2_SETWAY_WAY_SHIFT | nset << L2_SETWAY_SET_SHIFT | 1 << SETWAY_LEVEL_SHIFT;
				asm volatile ("dc csw, %0" : : "r" (nsetWayLevel) : "memory");
			}

		dataSyncBarrier();
	}


	void invalidateDataCache() {
		// invalidate L1 data cache
		for (unsigned set = 0; set < L1_DATA_CACHE_SETS; ++set)
			for (unsigned way = 0; way < L1_DATA_CACHE_WAYS; ++way) {
				uint64_t set_way_level = (way << L1_SETWAY_WAY_SHIFT) | (set << L1_SETWAY_SET_SHIFT);
				asm volatile("dc isw, %0" :: "r"(set_way_level) : "memory");
			}

		// invalidate L2 unified cache
		for (unsigned set = 0; set < L2_CACHE_SETS; ++set)
			for (unsigned way = 0; way < L2_CACHE_WAYS; ++way) {
				uint64_t set_way_level = (way << L2_SETWAY_WAY_SHIFT) | (set << L2_SETWAY_SET_SHIFT)
					                   | (1 << SETWAY_LEVEL_SHIFT);
				asm volatile("dc isw, %0" :: "r"(set_way_level) : "memory");
			}

		dataSyncBarrier();
	}

	void syncDataAndInstructionCache() {
		cleanDataCache();

		invalidateInstructionCache();
		dataSyncBarrier();

		instructionSyncBarrier();
	}

#ifndef ARM_ALLOW_MULTI_CORE
	static volatile uint32_t criticalLevel = 0;
	static volatile uint32_t flags[MAX_CRITICAL_LEVEL];

	void enterCritical(Level level) {
		uint32_t current_flags;
		asm volatile("mrs %0, daif" : "=r"(current_flags));

		Interrupts::disableBoth();
		assert(criticalLevel < MAX_CRITICAL_LEVEL);
		flags[criticalLevel] = current_flags;
		criticalLevel = criticalLevel + 1;

		if (level == Level::IRQ)
			Interrupts::enableFIQs();

		dataMemBarrier();
	}

	void leaveCritical() {
		dataMemBarrier();
		Interrupts::disableFIQs();
		assert(0 < criticalLevel);
		criticalLevel = criticalLevel - 1;
		const uint32_t to_restore = flags[criticalLevel];
		asm volatile("msr daif, %0" :: "r"(to_restore));
	}
#else
	static volatile uint32_t criticalLevels[CORES] = {0};
	static volatile uint32_t flags[CORES][MAX_CRITICAL_LEVEL];

	void enterCritical(Level level) {
		uint64_t mpidr;
		asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
		const unsigned core = mpidr & (CORES - 1);

		uint32_t current_flags;
		asm volatile("mrs %0, daif" : "=r"(current_flags));

		// If we are already on Level::FIQ, we must not go back to IRQ_LEVEL here.
		assert(level == Level::FIQ || !(current_flags & 0x40));

		Interrupts::disableBoth();

		assert(criticalLevels[core] < MAX_CRITICAL_LEVEL);
		flags[core][criticalLevels[core]] = current_flags;
		criticalLevels[core] = criticalLevels[core] + 1;

		if (targetLevel == Level::IRQ)
			Interrupts::enableFIQs();

		dataMemBarrier();
	}

	void leaveCritical() {
		uint64_t mpidr;
		asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
		const unsigned core = mpidr & (CORES - 1);
		dataMemBarrier();
		Interrupts::disableFIQs();
		assert(0 < criticalLevels[core]);
		criticalLevels[core] = criticalLevels[core] - 1;
		const uint32_t to_restore = flags[core][criticalLevels[core]];
		asm volatile("msr daif, %0" :: "r"(to_restore));
	}
#endif
}
