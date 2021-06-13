#pragma once

#include <stdint.h>

namespace Armaz::GPIO {
	bool call(uint32_t pin, uint32_t value, uintptr_t base, uint32_t field_size, uint32_t field_max);
	bool set(uint32_t pin_number, uint32_t value);
	bool clear(uint32_t pin_number, uint32_t value);
	bool pull(uint32_t pin_number, uint32_t value);
	bool function(uint32_t pin_number, uint32_t value);
	void useAsAlt3(uint32_t pin_number);
	void useAsAlt5(uint32_t pin_number);

	constexpr uint32_t MAX_PIN       = 53;
	constexpr uint32_t FUNCTION_OUT  =  1;
	constexpr uint32_t FUNCTION_ALT5 =  2;
	constexpr uint32_t FUNCTION_ALT3 =  7;

	constexpr uint32_t PULL_NONE = 0;
	constexpr uint32_t PULL_DOWN = 1;
	constexpr uint32_t PULL_UP   = 2;

	constexpr uintptr_t GPFSEL0   = 0x200000;
	constexpr uintptr_t GPSET0    = 0x20001C;
	constexpr uintptr_t GPCLR0    = 0x200028;
	constexpr uintptr_t GPPUPPDN0 = 0x2000E4;
}
