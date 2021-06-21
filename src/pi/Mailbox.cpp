#include "assert.h"
#include "Mailbox.h"
#include "MMIO.h"
#include "printf.h"

#define STATUS_OFFSET(box) ((box) == 1? 0x38 : 0x18)
#define RW_OFFSET(box)     ((box) == 1? 0x20 : 0x00)

namespace Armaz::Mailbox {
	uint32_t read(uint8_t channel) {
		for (;;) {
			while (MMIO::read(BASE + STATUS_OFFSET(0)) & EMPTY);
			const uint32_t data = MMIO::read(BASE + RW_OFFSET(0));
			if ((data & 0xf) == channel)
				return data & ~0xf;
		}
	}

	void write(uint8_t channel, uint32_t data) {
		assert((data & 0xf) == 0);
		while (MMIO::read(BASE + STATUS_OFFSET(1)) & FULL);
		MMIO::write(BASE + RW_OFFSET(1), data | channel);
	}

	void flush(unsigned box) {
		assert(box == 0 || box == 1);
		const uint8_t status_offset = STATUS_OFFSET(box);
		const uint8_t read_offset   = RW_OFFSET(box);
		while (!(MMIO::read(BASE + status_offset) & EMPTY)) {
			MMIO::read(BASE + read_offset);
			// TODO: wait 20 milliseconds
		}
	}

	uint32_t writeRead(uint8_t channel, uint32_t data) {
		// TODO: spinlocks
		flush(0);
		write(channel, data);
		auto result = read(channel);
		return result;
	}
}
