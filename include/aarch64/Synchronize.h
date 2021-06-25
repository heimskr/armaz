#pragma once

// Credit: https://github.com/rsta2/circle

#include <stdint.h>

namespace Armaz {
	enum class Level {Task, IRQ, FIQ};

	constexpr uint32_t SETWAY_LEVEL_SHIFT = 1;

	constexpr uint32_t L1_DATA_CACHE_SETS        = 256;
	constexpr uint32_t L1_DATA_CACHE_WAYS        =   2;
	constexpr uint32_t L1_SETWAY_WAY_SHIFT       =  31; // 32-Log2(L1_DATA_CACHE_WAYS)
	constexpr uint32_t L1_DATA_CACHE_LINE_LENGTH =  64;
	constexpr uint32_t L1_SETWAY_SET_SHIFT       =   6; // Log2(L1_DATA_CACHE_LINE_LENGTH)

	constexpr uint32_t L2_CACHE_SETS        = 1024;
	constexpr uint32_t L2_CACHE_WAYS        =   16;
	constexpr uint32_t L2_SETWAY_WAY_SHIFT  =   28; // 32-Log2(L2_CACHE_WAYS)
	constexpr uint32_t L2_CACHE_LINE_LENGTH =   64;
	constexpr uint32_t L2_SETWAY_SET_SHIFT  =    6; // Log2(L2_CACHE_LINE_LENGTH)

#define dataMemBarrier() asm volatile("dmb sy" ::: "memory")
#define dataSyncBarrier() asm volatile("dsb sy" ::: "memory")
	void cleanDataCache();
#define invalidateInstructionCache() asm volatile("ic iallu" ::: "memory")
#define instructionSyncBarrier() asm volatile("isb" ::: "memory")
	void syncDataAndInstructionCache();

	void enterCritical(Level);
	void leaveCritical();

#define peripheralEntry()
#define peripheralExit()
}
