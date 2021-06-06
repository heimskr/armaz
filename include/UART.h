#pragma once

#include <stdint.h>

namespace Armaz::UART {
	void init();
	void put(unsigned char);
	void put(const char *);
	unsigned char get();

	// The offsets for each register.
	constexpr uintptr_t GPIO_BASE = 0x200000;

	// Controls actuation of pull up/down to all GPIO pins.
	constexpr uintptr_t GPPUD = (GPIO_BASE + 0x94);

	// Controls actuation of pull up/down for specific GPIO pin.
	constexpr uintptr_t GPPUDCLK0 = (GPIO_BASE + 0x98);

	// The base address for UART.
	// 0xfe201000 for raspi4, 0x3f201000 raspi2 and raspi3, and 0x20201000 for raspi1.
	constexpr uintptr_t UART0_BASE = GPIO_BASE + 0x1000;

	// The offsets for each register for the UART.
	constexpr uintptr_t UART0_DR     = UART0_BASE + 0x00;
	constexpr uintptr_t UART0_RSRECR = UART0_BASE + 0x04;
	constexpr uintptr_t UART0_FR     = UART0_BASE + 0x18;
	constexpr uintptr_t UART0_ILPR   = UART0_BASE + 0x20;
	constexpr uintptr_t UART0_IBRD   = UART0_BASE + 0x24;
	constexpr uintptr_t UART0_FBRD   = UART0_BASE + 0x28;
	constexpr uintptr_t UART0_LCRH   = UART0_BASE + 0x2c;
	constexpr uintptr_t UART0_CR     = UART0_BASE + 0x30;
	constexpr uintptr_t UART0_IFLS   = UART0_BASE + 0x34;
	constexpr uintptr_t UART0_IMSC   = UART0_BASE + 0x38;
	constexpr uintptr_t UART0_RIS    = UART0_BASE + 0x3c;
	constexpr uintptr_t UART0_MIS    = UART0_BASE + 0x40;
	constexpr uintptr_t UART0_ICR    = UART0_BASE + 0x44;
	constexpr uintptr_t UART0_DMACR  = UART0_BASE + 0x48;
	constexpr uintptr_t UART0_ITCR   = UART0_BASE + 0x80;
	constexpr uintptr_t UART0_ITIP   = UART0_BASE + 0x84;
	constexpr uintptr_t UART0_ITOP   = UART0_BASE + 0x88;
	constexpr uintptr_t UART0_TDR    = UART0_BASE + 0x8c;

	// The offsets for Mailbox registers.
	constexpr uint16_t MBOX_BASE   = 0xb880;
	constexpr uint16_t MBOX_READ   = MBOX_BASE + 0x00;
	constexpr uint16_t MBOX_STATUS = MBOX_BASE + 0x18;
	constexpr uint16_t MBOX_WRITE  = MBOX_BASE + 0x20;
}
