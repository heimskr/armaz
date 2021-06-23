#pragma once

// Credit: https://github.com/rsta2/circle

#include <stdint.h>

namespace Armaz::UART {
	enum class Status {Normal, Overrun};

	extern Status status;

	void init(int baud = 115200);
	bool write(char);
	void write(const char *);
	size_t write(const void *, size_t);
	unsigned char read();

	constexpr size_t UART_BUFFER_SIZE = 2048;
	constexpr size_t UART_BUFFER_MASK = UART_BUFFER_SIZE - 1;

	constexpr ptrdiff_t GPIO_BASE = 0x200000;
	constexpr ptrdiff_t GPFSEL0   = GPIO_BASE;
	constexpr ptrdiff_t GPSET0    = GPIO_BASE + 0x1c;
	constexpr ptrdiff_t GPCLR0    = GPIO_BASE + 0x28;
	constexpr ptrdiff_t GPPUPPDN0 = GPIO_BASE + 0xe4;

	constexpr ptrdiff_t AUX_BASE        = 0x215000;
	constexpr ptrdiff_t AUX_IRQ         = AUX_BASE;
	constexpr ptrdiff_t AUX_ENABLES     = AUX_BASE + 0x04;
	constexpr ptrdiff_t AUX_MU_IO_REG   = AUX_BASE + 0x40;
	constexpr ptrdiff_t AUX_MU_IER_REG  = AUX_BASE + 0x44;
	constexpr ptrdiff_t AUX_MU_IIR_REG  = AUX_BASE + 0x48;
	constexpr ptrdiff_t AUX_MU_LCR_REG  = AUX_BASE + 0x4c;
	constexpr ptrdiff_t AUX_MU_MCR_REG  = AUX_BASE + 0x50;
	constexpr ptrdiff_t AUX_MU_LSR_REG  = AUX_BASE + 0x54;
	constexpr ptrdiff_t AUX_MU_MSR_REG  = AUX_BASE + 0x58;
	constexpr ptrdiff_t AUX_MU_SCRATCH  = AUX_BASE + 0x5c;
	constexpr ptrdiff_t AUX_MU_CNTL_REG = AUX_BASE + 0x60;
	constexpr ptrdiff_t AUX_MU_STAT_REG = AUX_BASE + 0x64;
	constexpr ptrdiff_t AUX_MU_BAUD_REG = AUX_BASE + 0x68;
	constexpr ptrdiff_t AUX_UART_CLOCK  = 500000000;

	constexpr ptrdiff_t UART0_BASE = 0x201000;
	constexpr ptrdiff_t UART0_DR   = UART0_BASE;
	constexpr ptrdiff_t UART0_FR   = UART0_BASE + 0x18;
	constexpr ptrdiff_t UART0_IBRD = UART0_BASE + 0x24;
	constexpr ptrdiff_t UART0_FBRD = UART0_BASE + 0x28;
	constexpr ptrdiff_t UART0_LCRH = UART0_BASE + 0x2c;
	constexpr ptrdiff_t UART0_CR   = UART0_BASE + 0x30;
	constexpr ptrdiff_t UART0_IFLS = UART0_BASE + 0x34;
	constexpr ptrdiff_t UART0_IMSC = UART0_BASE + 0x38;
	constexpr ptrdiff_t UART0_MIS  = UART0_BASE + 0x40;
	constexpr ptrdiff_t UART0_ICR  = UART0_BASE + 0x44;

	constexpr uint32_t FR_TXFE_MASK = 1 << 7;
	constexpr uint32_t FR_RXFF_MASK = 1 << 6;
	constexpr uint32_t FR_TXFF_MASK = 1 << 5;
	constexpr uint32_t FR_RXFE_MASK = 1 << 4;
	constexpr uint32_t FR_BUSY_MASK = 1 << 3;

	constexpr uint32_t INT_OE   = 1 << 10;
	constexpr uint32_t INT_BE   = 1 << 9;
	constexpr uint32_t INT_PE   = 1 << 8;
	constexpr uint32_t INT_FE   = 1 << 7;
	constexpr uint32_t INT_RT   = 1 << 6;
	constexpr uint32_t INT_TX   = 1 << 5;
	constexpr uint32_t INT_RX   = 1 << 4;
	constexpr uint32_t INT_DSRM = 1 << 3;
	constexpr uint32_t INT_DCDM = 1 << 2;
	constexpr uint32_t INT_CTSM = 1 << 1;

	constexpr int auxMuBaud(int baud) {
		return AUX_UART_CLOCK / (baud * 8) - 1;
	}
}
