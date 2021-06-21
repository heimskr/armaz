#include <stddef.h>

#include "aarch64/MMIO.h"
#include "pi/GPIO.h"
#include "pi/UART.h"

namespace Armaz::UART {

	static unsigned char outputQueue[UART_MAX_QUEUE];
	static uint32_t outputQueueWrite = 0;
	static uint32_t outputQueueRead  = 0;

	void init() {
		MMIO::init();
		MMIO::write(AUX_ENABLES, 1); // Enable UART1
		MMIO::write(AUX_MU_IER_REG, 0);
		MMIO::write(AUX_MU_CNTL_REG, 0);
		MMIO::write(AUX_MU_LCR_REG, 3); // 8 bits
		MMIO::write(AUX_MU_MCR_REG, 0);
		MMIO::write(AUX_MU_IER_REG, 0); // 5 to enable RX interrupts, 0 to disable?
		// MMIO::write(AUX_MU_IIR_REG, 0xc6); // Disable interrupts
		MMIO::write(AUX_MU_BAUD_REG, auxMuBaud(115200));
		GPIO::useAsAlt5(14);
		GPIO::useAsAlt5(15);
		MMIO::write(AUX_MU_CNTL_REG, 3); //enable RX/TX
	}

	bool isOutputQueueEmpty() {
		return outputQueueRead == outputQueueWrite;
	}

	bool isReadByteReady() {
		return MMIO::read(AUX_MU_LSR_REG) & 1;
	}

	bool isWriteByteReady() {
		return MMIO::read(AUX_MU_LSR_REG) & 0x20;
	}

	unsigned char read() {
		while (!isReadByteReady());
		return (unsigned char) MMIO::read(AUX_MU_IO_REG);
	}

	static void writeActual(unsigned char ch) {
		while (!isWriteByteReady()); 
		MMIO::write(AUX_MU_IO_REG, (uint32_t) ch);
	}

	void loadOutputFifo() {
		while (!isOutputQueueEmpty() && isWriteByteReady()) {
			writeActual(outputQueue[outputQueueRead]);
			outputQueueRead = (outputQueueRead + 1) & (UART_MAX_QUEUE - 1); // Don't overrun
		}
	}

	void _write(unsigned char ch) {
		if (ch == '\n')
			write('\r');

		uint32_t next = (outputQueueWrite + 1) & (UART_MAX_QUEUE - 1); // Don't overrun

		while (next == outputQueueRead)
			loadOutputFifo();

		outputQueue[outputQueueWrite] = ch;
		outputQueueWrite = next;
	}

	void write(unsigned char ch) {
		if (ch == '\n')
			writeActual('\r');
		writeActual(ch);
	}

	void write(const char *buffer) {
		while (*buffer)
			write(*buffer++);
	}

	void drain() {
		while (!isOutputQueueEmpty())
			loadOutputFifo();
	}

	void update() {
		loadOutputFifo();
		if (isReadByteReady()) {
			unsigned char ch = read();
			if (ch == '\r')
				write('\n');
			else
				write(ch);
		}
	}
}
