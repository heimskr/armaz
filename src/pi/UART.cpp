#include <stddef.h>
#include <string.h>

#include "assert.h"
#include "aarch64/MMIO.h"
#include "aarch64/Synchronize.h"
#include "board/BCM2711int.h"
#include "interrupts/IRQ.h"
#include "lib/printf.h"
#include "pi/GPIO.h"
#include "pi/UART.h"

namespace Armaz::UART {
	static volatile uint8_t inputQueue[UART_BUFFER_SIZE], outputQueue[UART_BUFFER_SIZE];
	static volatile unsigned txIn = 0, txOut = 0;
	static volatile unsigned rxIn = 0, rxOut = 0;
	Status status = Status::Normal;

	static void writeActual(unsigned char ch) {
		while (!(MMIO::read(AUX_MU_LSR_REG) & 0x20));
		MMIO::write(AUX_MU_IO_REG, (uint32_t) ch);
	}

	static void handler(void *) {
		dataMemBarrier();

		const uint32_t mis = MMIO::read(UART0_MIS);
		MMIO::write(UART0_ICR, mis);
		writeActual(mis == 64? '@' : mis == 32? '#' : '!');



		// *(volatile uint32_t *) 0xfe201044 = *(volatile uint32_t *) 0xfe201040;
		// printf("\e[31mUART handler\e[39m: %c\n", read());
		// printf("\e[31mUART handler\e[39m mis = 0x%x\n", mis);
		while (!(MMIO::read(UART0_FR) & FR_RXFE_MASK)) {
			uint32_t dr = MMIO::read(UART0_DR);
			if (((rxIn + 1) & UART_BUFFER_MASK) != rxOut) {
				inputQueue[rxIn++] = dr & 0xff;
				rxIn &= UART_BUFFER_MASK;
			} else if (status == Status::Normal)
				status = Status::Overrun;
			writeActual(dr & 0xff);
		}

		while (!(MMIO::read(UART0_FR) & FR_TXFF_MASK)) {
			if (txIn != txOut) {
				MMIO::write(UART0_DR, outputQueue[txOut++]);
				txOut &= UART_BUFFER_MASK;
			} else {
				MMIO::write(UART0_IMSC, MMIO::read(UART0_IMSC) & ~INT_TX);
				break;
			}
		}
	}

	void init(int baud) {
		MMIO::init();
		// MMIO::write(AUX_ENABLES, 1); // Enable UART1
		// // MMIO::write(AUX_MU_IER_REG, 0);
		// MMIO::write(AUX_MU_CNTL_REG, 0);
		// MMIO::write(AUX_MU_LCR_REG, 3); // 8 bits
		// MMIO::write(AUX_MU_MCR_REG, 0);
		// MMIO::write(AUX_MU_IER_REG, 2);
		// // MMIO::write(AUX_MU_IIR_REG, 0xc6); // Disable interrupts
		// MMIO::write(AUX_MU_BAUD_REG, auxMuBaud(115200));
		// GPIO::useAsAlt5(14);
		// GPIO::useAsAlt5(15);
		// MMIO::write(AUX_MU_CNTL_REG, 3); // Enable RX/TX

		unsigned clockrate = 48000000;
		assert(300 <= baud && baud <= 4000000);
		unsigned baud64 = baud * 16;
		unsigned intdiv = clockrate / baud64;
		assert(1 <= intdiv && intdiv <= 0xffff);
		unsigned fractdiv2 = (clockrate % baud64) * 8 / baud;
		unsigned fractdiv = fractdiv2 / 2 + fractdiv2 % 2;
		assert(fractdiv <= 0x3f);

		Interrupts::connect(ARM_IRQ_UART, handler, nullptr);

		GPIO::useAsAlt5(14);
		GPIO::useAsAlt5(15);
		MMIO::write(UART::UART0_IMSC, 0);
		MMIO::write(UART::UART0_ICR, 0x7ff);
		MMIO::write(UART::UART0_IBRD, intdiv);
		MMIO::write(UART::UART0_FBRD, fractdiv);
		MMIO::write(UART::UART0_IFLS, (1 << 0) | (1 << 3));
		MMIO::write(UART::UART0_LCRH, (3 << 5) | (1 << 4));
		MMIO::write(UART::UART0_IMSC, (1 << 4) | (1 << 6) | (1 << 10));
		dataSyncBarrier();
		MMIO::write(UART::UART0_CR, 1 | (1 << 8) | (1 << 9));
		// MMIO::write(UART::UART0_ICR, 0x7ff);
	}

	size_t write(const void *buffer, size_t count) {
		const char *cbuffer = reinterpret_cast<const char *>(buffer);
		size_t out = 0;

		while (count--) {
			if ((*cbuffer == '\n' && !write('\r')) || !write(*cbuffer++))
				break;
			++out;
		}

		while (txIn != txOut) {
			if (!(MMIO::read(UART::UART0_FR) & FR_TXFF_MASK)) {
				MMIO::write(UART::UART0_DR, outputQueue[txOut++]);
				txOut &= UART_BUFFER_MASK;
			} else {
				MMIO::write(UART::UART0_IMSC, MMIO::read(UART::UART0_IMSC) | INT_TX);
				break;
			}
		}

		return out;
	}

	bool write(char ch) {
		if (((txIn + 1) & UART_BUFFER_MASK) != txOut) {
			outputQueue[txIn++] = ch;
			txIn &= UART_BUFFER_MASK;
			return true;
		}
		return false;
	}



	// bool isOutputQueueEmpty() {
	// 	return outputQueueRead == outputQueueWrite;
	// }

	// bool isReadByteReady() {
	// 	return MMIO::read(AUX_MU_LSR_REG) & 1;
	// }

	// bool isWriteByteReady() {
	// 	return MMIO::read(AUX_MU_LSR_REG) & 0x20;
	// }

	// unsigned char read() {
	// 	while (!isReadByteReady());
	// 	return (unsigned char) MMIO::read(AUX_MU_IO_REG);
	// }

	// void loadOutputFifo() {
	// 	while (!isOutputQueueEmpty() && isWriteByteReady()) {
	// 		writeActual(outputQueue[outputQueueRead]);
	// 		outputQueueRead = (outputQueueRead + 1) & (UART_MAX_QUEUE - 1); // Don't overrun
	// 	}
	// }

	// void _write(unsigned char ch) {
	// 	if (ch == '\n')
	// 		write('\r');

	// 	uint32_t next = (outputQueueWrite + 1) & (UART_MAX_QUEUE - 1); // Don't overrun

	// 	while (next == outputQueueRead)
	// 		loadOutputFifo();

	// 	outputQueue[outputQueueWrite] = ch;
	// 	outputQueueWrite = next;
	// }

	// void write(unsigned char ch) {
	// 	if (ch == '\n')
	// 		writeActual('\r');
	// 	writeActual(ch);
	// }

	void write(const char *buffer) {
		write(buffer, strlen(buffer));
	}

	// void drain() {
	// 	while (!isOutputQueueEmpty())
	// 		loadOutputFifo();
	// }

	// void update() {
	// 	loadOutputFifo();
	// 	if (isReadByteReady()) {
	// 		unsigned char ch = read();
	// 		if (ch == '\r')
	// 			write('\n');
	// 		else
	// 			write(ch);
	// 	}
	// }
}
