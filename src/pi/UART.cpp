// Credit: https://github.com/rsta2/circle

#include <stddef.h>
#include <string.h>

#include "assert.h"
#include "Config.h"
#include "aarch64/MMIO.h"
#include "aarch64/Spinlock.h"
#include "aarch64/Synchronize.h"
#include "board/BCM2711int.h"
#include "interrupts/IRQ.h"
#include "lib/printf.h"
#include "pi/GPIO.h"
#include "pi/PropertyTags.h"
#include "pi/UART.h"

// #define UART_ACKNOWLEDGE_INTERRUPTS

#define IFLS_RXIFSEL_SHIFT 3
#define IFLS_RXIFSEL_MASK  (7 << IFLS_RXIFSEL_SHIFT)
#define IFLS_TXIFSEL_SHIFT 0
#define IFLS_TXIFSEL_MASK  (7 << IFLS_TXIFSEL_SHIFT)
#define IFLS_IFSEL_1_8     0
#define IFLS_IFSEL_1_4     1
#define IFLS_IFSEL_1_2     2
#define IFLS_IFSEL_3_4     3
#define IFLS_IFSEL_7_8     4
#define LCRH_SPS_MASK      (1 << 7)
#define LCRH_WLEN8_MASK    (3 << 5)
#define LCRH_WLEN7_MASK    (2 << 5)
#define LCRH_WLEN6_MASK    (1 << 5)
#define LCRH_WLEN5_MASK    (0 << 5)
#define LCRH_FEN_MASK      (1 << 4)
#define LCRH_STP2_MASK     (1 << 3)
#define LCRH_EPS_MASK      (1 << 2)
#define LCRH_PEN_MASK      (1 << 1)
#define LCRH_BRK_MASK      (1 << 0)
#define CR_CTSEN_MASK      (1 << 15)
#define CR_RTSEN_MASK      (1 << 14)
#define CR_OUT2_MASK       (1 << 13)
#define CR_OUT1_MASK       (1 << 12)
#define CR_RTS_MASK        (1 << 11)
#define CR_DTR_MASK        (1 << 10)
#define CR_RXE_MASK        (1 << 9)
#define CR_TXE_MASK        (1 << 8)
#define CR_LBE_MASK        (1 << 7)
#define CR_UART_EN_MASK    (1 << 0)

#define ALT_FUNC(device, gpio) ((Armaz::GPIO::Mode) (Armaz::UART::gpioConfig[device][gpio][1] + \
	(int) Armaz::GPIO::Mode::AlternateFunction0))

namespace Armaz::UART {
	Status status = Status::Normal;
	static volatile uint8_t inputQueue[UART_BUFFER_SIZE], outputQueue[UART_BUFFER_SIZE];
	static volatile unsigned txIn = 0, txOut = 0;
	static volatile unsigned rxIn = 0, rxOut = 0;
	static GPIO::Pin pin32, pin33, txPin, rxPin;
	static unsigned gpioConfig[6][2][2] = {
		// TXD      RXD
#if SERIAL_GPIO_SELECT == 14
		{{14,  0}, {15,  0}},
#elif SERIAL_GPIO_SELECT == 32
		{{32,  3}, {33,  3}},
#elif SERIAL_GPIO_SELECT == 36
		{{36,  2}, {37,  2}},
#else
	#error SERIAL_GPIO_SELECT must be 14, 32 or 36
#endif
#if RASPPI >= 4
		{{10000, 10000}, {10000, 10000}}, // unused
		{{ 0,  4}, { 1,  4}},
		{{ 4,  4}, { 5,  4}},
		{{ 8,  4}, { 9,  4}},
		{{12,  4}, {13,  4}}
#endif
	};

#ifdef UART_USE_FIQ
	static Spinlock spinlock {Level::FIQ};
#else
	static Spinlock spinlock {Level::IRQ};
#endif
	static Spinlock lineSpinlock {Level::Task};

	static void handler(void *) {
		dataMemBarrier();

		spinlock.acquire();

#ifdef UART_ACKNOWLEDGE_INTERRUPTS
		const uint32_t mis = MMIO::read(UART0_MIS);
		MMIO::write(UART0_ICR, mis);
#endif

		while (!(MMIO::read(UART0_FR) & FR_RXFE_MASK)) {
			const uint32_t dr = MMIO::read(UART0_DR);
			if (((rxIn + 1) & UART_BUFFER_MASK) != rxOut) {
				inputQueue[rxIn] = dr & 0xff;
				rxIn = (rxIn + 1) & UART_BUFFER_MASK;
			} else if (status == Status::Normal)
				status = Status::Overrun;
		}

		while (!(MMIO::read(UART0_FR) & FR_TXFF_MASK)) {
			if (txIn != txOut) {
				MMIO::write(UART0_DR, outputQueue[txOut]);
				txOut = (txOut + 1) & UART_BUFFER_MASK;
			} else {
				MMIO::write(UART0_IMSC, MMIO::read(UART0_IMSC) & ~INT_TX);
				break;
			}
		}

		spinlock.release();
	}

	void init(int baud) {
		MMIO::init();

		if (SERIAL_GPIO_SELECT == 14) {
			pin32.assign(32);
			pin32.setMode(GPIO::Mode::Input);
			pin33.assign(33);
			pin33.setMode(GPIO::Mode::Input);
		}

		unsigned device = 0;

		txPin.assign(gpioConfig[device][0][0]);
		txPin.setMode(ALT_FUNC(device, 0));

		rxPin.assign(gpioConfig[device][1][0]);
		rxPin.setMode(ALT_FUNC(device, 1));
		rxPin.setMode(GPIO::PullMode::Up);

		unsigned clockrate = 48000000;
		PropertyTagClockRate clock_tag;
		clock_tag.clockID = PropertyTagClockRate::UART;
		if (PropertyTags::getTag(PROPTAG_GET_CLOCK_RATE, &clock_tag, sizeof(clock_tag)))
			clockrate = clock_tag.rate;

		assert(300 <= baud && baud <= 4000000);
		const unsigned baud64 = baud * 16;
		const unsigned intdiv = clockrate / baud64;
		assert(1 <= intdiv && intdiv <= 0xffff);
		const unsigned fractdiv2 = (clockrate % baud64) * 8 / baud;
		const unsigned fractdiv = fractdiv2 / 2 + fractdiv2 % 2;
		assert(fractdiv <= 0x3f);

		Interrupts::connect(ARM_IRQ_UART, handler, nullptr);

		MMIO::write(UART0_IMSC, 0);
		MMIO::write(UART0_ICR, 0x7ff);
		MMIO::write(UART0_IBRD, intdiv);
		MMIO::write(UART0_FBRD, fractdiv);
		MMIO::write(UART0_IFLS, (IFLS_IFSEL_1_4 << IFLS_TXIFSEL_SHIFT) | (IFLS_IFSEL_1_4 << IFLS_RXIFSEL_SHIFT));
		MMIO::write(UART0_LCRH, LCRH_WLEN8_MASK | LCRH_FEN_MASK);
		MMIO::write(UART0_IMSC, INT_RX | INT_RT | INT_OE);
		dataSyncBarrier();
		MMIO::write(UART0_CR, CR_UART_EN_MASK | CR_TXE_MASK | CR_RXE_MASK);
	}

	size_t write(const void *buffer, size_t count) {
		const char *cbuffer = reinterpret_cast<const char *>(buffer);
		size_t out = 0;

		lineSpinlock.acquire();

		while (count--) {
			if (!write(*cbuffer++))
				break;
			++out;
		}

		lineSpinlock.release();
		spinlock.acquire();

		while (txIn != txOut) {
			if (!(MMIO::read(UART0_FR) & FR_TXFF_MASK)) {
				MMIO::write(UART0_DR, outputQueue[txOut]);
				txOut = (txOut + 1) & UART_BUFFER_MASK;
			} else {
				MMIO::write(UART0_IMSC, MMIO::read(UART0_IMSC) | INT_TX);
				break;
			}
		}

		spinlock.release();

		return out;
	}

	bool write(char ch) {
		if (ch == '\n')
			write('\r');

		spinlock.acquire();

		if (((txIn + 1) & UART_BUFFER_MASK) != txOut) {
			outputQueue[txIn] = ch;
			txIn = (txIn + 1) & UART_BUFFER_MASK;
			MMIO::write(UART0_IMSC, MMIO::read(UART0_IMSC) | INT_TX);
			spinlock.release();
			return true;
		}

		spinlock.release();
		return false;
	}

	void write(const char *buffer) {
		write(buffer, strlen(buffer));
	}

	size_t read(void *buffer, size_t count) {
		assert(count < UART_BUFFER_SIZE);

		uint8_t *cbuffer = (uint8_t *) buffer;
		assert(cbuffer);

		size_t result = 0;

		spinlock.acquire();

		if (status != Status::Normal)
			return 0;

		while (0 < count) {
			if (rxIn == rxOut)
				break;

			*cbuffer++ = inputQueue[rxOut];
			rxOut = (rxOut + 1) & UART_BUFFER_MASK;

			--count;
			++result;
		}

		spinlock.release();

		return result;
	}

	size_t availableToRead() {
		size_t result;
		spinlock.acquire();
		if (rxIn < rxOut)
			result = UART_BUFFER_SIZE + rxIn - rxOut;
		else
			result = rxIn - rxOut;
		spinlock.release();
		return result;
	}

	void flush() {
		while (MMIO::read(UART0_FR) & FR_BUSY_MASK);
	}
}
