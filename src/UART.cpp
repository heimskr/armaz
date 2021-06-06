// Based on code from https://wiki.osdev.org/Raspberry_Pi_Bare_Bones

#include <stddef.h>

#include "MMIO.h"
#include "RPi.h"
#include "UART.h"

namespace Armaz::UART {
	void init() {
		MMIO::init();

		// Disable UART0.
		MMIO::write(UART0_CR, 0);

		// Setup the GPIO pin 14 && 15.

		// Disable pull up/down for all GPIO pins & delay for 150 cycles.
		MMIO::write(GPPUD, 0);
		RPi::delay(150);

		// Disable pull up/down for pin 14,15 & delay for 150 cycles.
		MMIO::write(GPPUDCLK0, (1 << 14) | (1 << 15));
		RPi::delay(150);

		// Write 0 to GPPUDCLK0 to make it take effect.
		MMIO::write(GPPUDCLK0, 0);

		// Clear pending interrupts.
		MMIO::write(UART0_ICR, 0x7ff);

		// A Mailbox message with set clock rate of PL011 to 3MHz tag
		volatile static uint32_t __attribute__((aligned(16))) mbox[9] = {
			9 * 4, 0, 0x38002, 12, 8, 2, 3000000, 0, 0
		};

		// Set integer & fractional part of baud rate.
		// Divider = UART_CLOCK/(16 * Baud)
		// Fraction part register = (Fractional part * 64) + 0.5
		// Baud = 115200.

		// For Raspi3 and 4, the UART_CLOCK is system-clock dependent by default.
		// Set it to 3MHz so we can consistently set the baud rate.
		if (3 <= RPi::getModel()) {
			uintptr_t r = ((uintptr_t) &mbox & ~0xf) | 8;

			// Wait until we can talk to the VC.
			while (MMIO::read(MBOX_STATUS) & 0x80000000);

			// Send our message to property channel and wait for the response.
			MMIO::write(MBOX_WRITE, r);
			while ((MMIO::read(MBOX_STATUS) & 0x40000000) || MMIO::read(MBOX_READ) != r);
		}

		// Divider = 3000000 / (16 * 115200) ~= 1.6276 ~= 1.
		MMIO::write(UART0_IBRD, 1);

		// Fractional part register = (.627 * 64) + 0.5 = 40.628 ~= 40.
		MMIO::write(UART0_FBRD, 40);

		// Enable FIFO & 8 bit data transmission (1 stop bit, no parity).
		MMIO::write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

		// Mask all interrupts.
		MMIO::write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

		// Enable UART0, receive & transfer part of UART.
		MMIO::write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
	}

	void put(unsigned char ch) {
		// Wait for UART to become ready to transmit.
		while (MMIO::read(UART0_FR) & (1 << 5));
		MMIO::write(UART0_DR, ch);
	}

	void put(const char *str) {
		for (size_t i = 0; str[i] != '\0'; ++i)
			put((unsigned char) str[i]);
	}

	unsigned char get() {
		// Wait for UART to have received something.
		while (MMIO::read(UART0_FR) & (1 << 4));
		return MMIO::read(UART0_DR);
	}
}
