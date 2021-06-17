#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Armaz::Mailbox {
	constexpr ptrdiff_t BASE  = 0xb880;
	constexpr  uint64_t FULL  = 0x80000000;
	constexpr  uint64_t EMPTY = 0x40000000;
	uint32_t read(uint8_t channel);
	void write(uint8_t channel, uint32_t data);
	void flush(unsigned box = 0);
	uint32_t writeRead(uint8_t channel, uint32_t data);
}
