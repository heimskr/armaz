#pragma once

#include <stddef.h>
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

	constexpr ptrdiff_t GPFSEL0   = 0x200000;
	constexpr ptrdiff_t GPFSEL1   = 0x200004;
	constexpr ptrdiff_t GPFSEL2   = 0x200008;
	constexpr ptrdiff_t GPFSEL3   = 0x20000C;
	constexpr ptrdiff_t GPFSEL4   = 0x200010;
	constexpr ptrdiff_t GPFSEL5   = 0x200014;
	constexpr ptrdiff_t GPSET0    = 0x20001C;
	constexpr ptrdiff_t GPSET1    = 0x200020;
	constexpr ptrdiff_t GPCLR0    = 0x200028;
	constexpr ptrdiff_t GPLEV0    = 0x200034;
	constexpr ptrdiff_t GPLEV1    = 0x200038;
	constexpr ptrdiff_t GPEDS0    = 0x200040;
	constexpr ptrdiff_t GPEDS1    = 0x200044;
	constexpr ptrdiff_t GPHEN0    = 0x200064;
	constexpr ptrdiff_t GPHEN1    = 0x200068;
	constexpr ptrdiff_t GPPUD     = 0x200094;
	constexpr ptrdiff_t GPPUDCLK0 = 0x200098;
	constexpr ptrdiff_t GPPUDCLK1 = 0x20009C;
	constexpr ptrdiff_t GPPUPPDN0 = 0x2000E4;
}
