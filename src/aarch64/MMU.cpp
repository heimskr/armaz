#include <cstdint>

#include "aarch64/MMU.h"
#include "aarch64/SysRegs.h"
#include "aarch64/Synchronize.h"

namespace Armaz::MMU {
	void enable() {
		uint32_t sctlr;
		asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
		sctlr = (sctlr & ~(SCTLR_EL1_WXN | SCTLR_EL1_A)) | SCTLR_EL1_I | SCTLR_EL1_C | SCTLR_EL1_M;
		asm volatile("msr sctlr_el1, %0" :: "r"(sctlr));
	}

	void disable() {
		uint32_t sctlr;
		asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
		sctlr &= ~(SCTLR_EL1_M | SCTLR_EL1_C);
		asm volatile("msr sctlr_el1, %0" :: "r"(sctlr) : "memory");
		dataSyncBarrier();
		instructionSyncBarrier();
		cleanDataCache();
		invalidateDataCache();
		asm volatile("tlbi vmalle1" ::: "memory");
		dataSyncBarrier();
		instructionSyncBarrier();
	}
}
