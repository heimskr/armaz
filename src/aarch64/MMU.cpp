#include "aarch64/MMU.h"
#include "aarch64/SysRegs.h"

namespace Armaz::MMU {
	void enable() {
		asm volatile("msr sctlr_el1, %0" :: "r"(SCTLR_VALUE_MMU_ENABLED));
	}

	void disable() {
		asm volatile("msr sctlr_el1, %0" :: "r"(SCTLR_VALUE_MMU_DISABLED));
	}
}
