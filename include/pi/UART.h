#pragma once

#include <stdint.h>

namespace Armaz::UART {
	void init();
	void write(unsigned char);
	void write(const char *);
	unsigned char read();
	bool isOutputQueueEmpty();
	bool isReadByteReady();
	bool isWriteByteReady();
	void loadOutputFifo();
	void drain();
	void update();

	constexpr uintptr_t GPIO_BASE = 0x200000;
	constexpr uintptr_t GPFSEL0   = GPIO_BASE;
	constexpr uintptr_t GPSET0    = GPIO_BASE + 0x1c;
	constexpr uintptr_t GPCLR0    = GPIO_BASE + 0x28;
	constexpr uintptr_t GPPUPPDN0 = GPIO_BASE + 0xe4;

	constexpr uintptr_t AUX_BASE        = 0x215000;
	constexpr uintptr_t AUX_IRQ         = AUX_BASE;
	constexpr uintptr_t AUX_ENABLES     = AUX_BASE + 0x04;
	constexpr uintptr_t AUX_MU_IO_REG   = AUX_BASE + 0x40;
	constexpr uintptr_t AUX_MU_IER_REG  = AUX_BASE + 0x44;
	constexpr uintptr_t AUX_MU_IIR_REG  = AUX_BASE + 0x48;
	constexpr uintptr_t AUX_MU_LCR_REG  = AUX_BASE + 0x4c;
	constexpr uintptr_t AUX_MU_MCR_REG  = AUX_BASE + 0x50;
	constexpr uintptr_t AUX_MU_LSR_REG  = AUX_BASE + 0x54;
	constexpr uintptr_t AUX_MU_MSR_REG  = AUX_BASE + 0x58;
	constexpr uintptr_t AUX_MU_SCRATCH  = AUX_BASE + 0x5c;
	constexpr uintptr_t AUX_MU_CNTL_REG = AUX_BASE + 0x60;
	constexpr uintptr_t AUX_MU_STAT_REG = AUX_BASE + 0x64;
	constexpr uintptr_t AUX_MU_BAUD_REG = AUX_BASE + 0x68;
	constexpr uintptr_t AUX_UART_CLOCK  = 500000000;
	constexpr uintptr_t UART_MAX_QUEUE  = 0x4000;

	constexpr int auxMuBaud(int baud) {
		return AUX_UART_CLOCK / (baud * 8) - 1;
	}
}
