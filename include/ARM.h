#pragma once

#include <stdint.h>

namespace Armaz::ARM {
	int getEL();
	uint32_t getSctlr();
	void setSctlr(uint32_t);
	bool getMMU();
	void setMMU(bool enabled);
	void delay(int32_t count);

	constexpr uint32_t SCTLR_MMU_ENABLED = 1;
}
