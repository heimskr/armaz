#include "aarch64/MMIO.h"
#include "pi/GPIO.h"

namespace Armaz::GPIO {
	bool call(uint32_t pin_number, uint32_t value, uintptr_t base, uint32_t field_size, uint32_t field_max) {
		uint32_t field_mask = (1 << field_size) - 1;
	
		if (field_max < pin_number || field_mask < value)
			return false;

		uint32_t num_fields = 32 / field_size;
		uintptr_t reg = base + ((pin_number / num_fields) * 4);
		uint32_t shift = (pin_number % num_fields) * field_size;

		uint32_t curval = MMIO::read(reg);
		curval &= ~(field_mask << shift);
		curval |= value << shift;
		MMIO::write(reg, curval);

		return true;
	}

	bool set(uint32_t pin_number, uint32_t value) {
		return call(pin_number, value, GPSET0, 1, MAX_PIN);
	}

	bool clear(uint32_t pin_number, uint32_t value) {
		return call(pin_number, value, GPCLR0, 1, MAX_PIN);
	}

	bool pull(uint32_t pin_number, uint32_t value) {
		return call(pin_number, value, GPPUPPDN0, 2, MAX_PIN);
	}

	bool function(uint32_t pin_number, uint32_t value) {
		return call(pin_number, value, GPFSEL0, 3, MAX_PIN);
	}

	void useAsAlt3(uint32_t pin_number) {
		pull(pin_number, PULL_NONE);
		function(pin_number, FUNCTION_ALT3);
	}

	void useAsAlt5(uint32_t pin_number) {
		pull(pin_number, PULL_NONE);
		function(pin_number, FUNCTION_ALT5);
	}
}
