/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "GPIO.h"
#include "MMIO.h"
#include "printf.h"
#include "SD.h"
#include "Timer.h"
#include "UART.h"

constexpr ptrdiff_t EMMC_BASE        = 0x340000;
// constexpr ptrdiff_t EMMC_ARG2        = 0x300000;
constexpr ptrdiff_t EMMC_BLKSIZECNT  = EMMC_BASE + 0x04;
constexpr ptrdiff_t EMMC_ARG1        = EMMC_BASE + 0x08;
constexpr ptrdiff_t EMMC_CMDTM       = EMMC_BASE + 0x0c;
constexpr ptrdiff_t EMMC_RESP0       = EMMC_BASE + 0x10;
constexpr ptrdiff_t EMMC_RESP1       = EMMC_BASE + 0x14;
constexpr ptrdiff_t EMMC_RESP2       = EMMC_BASE + 0x18;
constexpr ptrdiff_t EMMC_RESP3       = EMMC_BASE + 0x1c;
constexpr ptrdiff_t EMMC_DATA        = EMMC_BASE + 0x20;
constexpr ptrdiff_t EMMC_STATUS      = EMMC_BASE + 0x24;
constexpr ptrdiff_t EMMC_CONTROL0    = EMMC_BASE + 0x28;
constexpr ptrdiff_t EMMC_CONTROL1    = EMMC_BASE + 0x2c;
constexpr ptrdiff_t EMMC_INTERRUPT   = EMMC_BASE + 0x30;
constexpr ptrdiff_t EMMC_INT_MASK    = EMMC_BASE + 0x34;
constexpr ptrdiff_t EMMC_INT_EN      = EMMC_BASE + 0x38;
// constexpr ptrdiff_t EMMC_CONTROL2    = EMMC_BASE + 0x3c;
constexpr ptrdiff_t EMMC_SLOTISR_VER = EMMC_BASE + 0xfc;

// Command flags
constexpr uint32_t CMD_NEED_APP    = 0x80000000;
constexpr uint32_t CMD_RSPNS_48    = 0x00020000;
constexpr uint32_t CMD_ERRORS_MASK = 0xfff9c004;
constexpr uint32_t CMD_RCA_MASK    = 0xffff0000;

// Commands
constexpr uint32_t CMD_GO_IDLE       = 0x00000000;
constexpr uint32_t CMD_ALL_SEND_CID  = 0x02010000;
constexpr uint32_t CMD_SEND_REL_ADDR = 0x03020000;
constexpr uint32_t CMD_CARD_SELECT   = 0x07030000;
constexpr uint32_t CMD_SEND_IF_COND  = 0x08020000;
constexpr uint32_t CMD_STOP_TRANS    = 0x0c030000;
constexpr uint32_t CMD_READ_SINGLE   = 0x11220010;
constexpr uint32_t CMD_READ_MULTI    = 0x12220032;
constexpr uint32_t CMD_SET_BLOCKCNT  = 0x17020000;
constexpr uint32_t CMD_APP_CMD       = 0x37000000;
constexpr uint32_t CMD_SET_BUS_WIDTH = 0x06020000 | CMD_NEED_APP;
constexpr uint32_t CMD_SEND_OP_COND  = 0x29020000 | CMD_NEED_APP;
constexpr uint32_t CMD_SEND_SCR      = 0x33220010 | CMD_NEED_APP;

// Status register settings
constexpr uint16_t SR_READ_AVAILABLE = 0x0800;
constexpr uint16_t SR_DAT_INHIBIT    = 0x0002;
constexpr uint16_t SR_CMD_INHIBIT    = 0x0001;
constexpr uint16_t SR_APP_CMD        = 0x0020;

// Interrupt register settings
constexpr uint32_t INT_DATA_TIMEOUT = 0x00100000;
constexpr uint32_t INT_CMD_TIMEOUT  = 0x00010000;
constexpr uint32_t INT_READ_RDY     = 0x00000020;
constexpr uint32_t INT_CMD_DONE     = 0x00000001;

constexpr uint32_t INT_ERROR_MASK = 0x017e8000;

// constexpr uint32_t C0_SPI_MODE_EN = 0x00100000;
// constexpr uint32_t C0_HCTL_HS_EN  = 0x00000004;
constexpr uint32_t C0_HCTL_DWITDH = 0x00000002;

// constexpr uint32_t C1_SRST_DATA  = 0x04000000;
// constexpr uint32_t C1_SRST_CMD   = 0x02000000;
constexpr uint32_t C1_SRST_HC    = 0x01000000;
// constexpr uint32_t C1_TOUNIT_DIS = 0x000f0000;
constexpr uint32_t C1_TOUNIT_MAX = 0x000e0000;
// constexpr uint32_t C1_CLK_GENSEL = 0x00000020;
constexpr uint32_t C1_CLK_EN     = 0x00000004;
constexpr uint32_t C1_CLK_STABLE = 0x00000002;
constexpr uint32_t C1_CLK_INTLEN = 0x00000001;

constexpr uint32_t HOST_SPEC_NUM       = 0xff0000;
constexpr uint32_t HOST_SPEC_NUM_SHIFT = 16;
// constexpr uint32_t HOST_SPEC_V3        = 2;
constexpr uint32_t HOST_SPEC_V2        = 1;
// constexpr uint32_t HOST_SPEC_V1        = 0;

constexpr uint32_t SCR_SD_BUS_WIDTH_4  = 0x00000400;
constexpr uint32_t SCR_SUPP_SET_BLKCNT = 0x02000000;
constexpr uint32_t SCR_SUPP_CCS        = 0x00000001;

constexpr uint32_t ACMD41_VOLTAGE      = 0x00ff8000;
constexpr uint32_t ACMD41_CMD_COMPLETE = 0x80000000;
constexpr uint32_t ACMD41_CMD_CCS      = 0x40000000;
constexpr uint32_t ACMD41_ARG_HC       = 0x51ff8000;


namespace Armaz::SD {
	static uint64_t sd_scr[2], sd_rca, sd_hv;
	static int sd_error;

	/** Wait for data or command ready */
	static int status(uint32_t mask) {
		int timeout = 500000;
		while ((MMIO::read(EMMC_STATUS) & mask) && !(MMIO::read(EMMC_INTERRUPT) & INT_ERROR_MASK) && timeout--)
			Timers::waitMicroseconds(1);
		return (timeout <= 0 || (MMIO::read(EMMC_INTERRUPT) & INT_ERROR_MASK))? ERROR : SUCCESS;
	}

	/** Wait for interrupt */
	static int waitForInterrupt(uint32_t mask) {
		uint32_t r, m = mask | INT_ERROR_MASK;
		int timeout = 1000000;
		while (!(MMIO::read(EMMC_INTERRUPT) & m) && timeout--)
			Timers::waitMicroseconds(1);
		r = MMIO::read(EMMC_INTERRUPT);

		if (timeout <= 0 || (r & INT_CMD_TIMEOUT) || (r & INT_DATA_TIMEOUT) ) {
			MMIO::write(EMMC_INTERRUPT, r);
			return TIMEOUT;
		} else if (r & INT_ERROR_MASK) {
			MMIO::write(EMMC_INTERRUPT, r);
			return ERROR;
		}

		MMIO::write(EMMC_INTERRUPT, mask);
		return SUCCESS;
	}

	/** Send a command */
	static int sendCommand(uint32_t code, uint32_t arg) {
		int result = 0;
		sd_error = SUCCESS;

		if (code & CMD_NEED_APP) {
			result = sendCommand(CMD_APP_CMD | (sd_rca? CMD_RSPNS_48 : 0), sd_rca);
			if (sd_rca && !result) {
				UART::write("ERROR: failed to send SD APP command\n");
				sd_error = ERROR;
				return SUCCESS;
			}
			code &= ~CMD_NEED_APP;
		}

		if (status(SR_CMD_INHIBIT) == SUCCESS) {
			UART::write("ERROR: EMMC busy\n");
			sd_error = TIMEOUT;
			return SUCCESS;
		}

		printf("EMMC: Sending command 0x%x, arg = 0x%x\n", code, arg);

		MMIO::write(EMMC_INTERRUPT, MMIO::read(EMMC_INTERRUPT));
		MMIO::write(EMMC_ARG1, arg); MMIO::write(EMMC_CMDTM, code);

		if (code == CMD_SEND_OP_COND)
			Timers::waitMicroseconds(1000);
		else if (code == CMD_SEND_IF_COND || code == CMD_APP_CMD)
			Timers::waitMicroseconds(100);
		
		if ((result = waitForInterrupt(INT_CMD_DONE)) == SUCCESS) {
			UART::write("ERROR: failed to send EMMC command\n");
			sd_error = result;
			return SUCCESS;
		}
	
		result = MMIO::read(EMMC_RESP0);

		if (code == CMD_GO_IDLE || code == CMD_APP_CMD)
			return 0;

		if (code == (CMD_APP_CMD|CMD_RSPNS_48))
			return result & SR_APP_CMD;

		if (code == CMD_SEND_OP_COND)
			return result;

		if (code == CMD_SEND_IF_COND)
			return result == static_cast<int>(arg)? SUCCESS : ERROR;

		if (code == CMD_ALL_SEND_CID) {
			result |= MMIO::read(EMMC_RESP3);
			result |= MMIO::read(EMMC_RESP2);
			result |= MMIO::read(EMMC_RESP1);
			return result;
		}

		if (code == CMD_SEND_REL_ADDR) {
			sd_error = (((result & 0x1fff)) | ((result & 0x2000) << 6) | ((result & 0x4000) << 8)
			          | ((result & 0x8000) << 8)) & CMD_ERRORS_MASK;
			return result & CMD_RCA_MASK;
		}

		return result & CMD_ERRORS_MASK;
	}

	/** Reads a block from the SD card and returns the number of bytes read. Returns 0 on error. */
	int readBlock(uint32_t lba, uint8_t *buffer, uint32_t num) {
		int result;
		uint32_t c = 0;

		if (num < 1)
			num = 1;

		printf("sd_readblock: lba = 0x%x, num = 0x%x\n", lba, num);

		if (status(SR_DAT_INHIBIT)) {
			sd_error = static_cast<int>(TIMEOUT);
			return 0;
		}

		uint32_t *buffer32 = reinterpret_cast<uint32_t *>(buffer);
		if (sd_scr[0] & SCR_SUPP_CCS) {
			if (1 < num && (sd_scr[0] & SCR_SUPP_SET_BLKCNT)) {
				sendCommand(CMD_SET_BLOCKCNT, num);
				if (sd_error)
					return 0;
			}

			MMIO::write(EMMC_BLKSIZECNT, (num << 16) | 512);
			sendCommand(num == 1? CMD_READ_SINGLE : CMD_READ_MULTI, lba);
			if (sd_error)
				return 0;
		} else
			MMIO::write(EMMC_BLKSIZECNT, (1 << 16) | 512);

		while (c < num) {
			if (!(sd_scr[0] & SCR_SUPP_CCS)) {
				sendCommand(CMD_READ_SINGLE, (lba + c) * 512);
				if (sd_error)
					return 0;
			}

			if ((result = waitForInterrupt(INT_READ_RDY)) != SUCCESS) {
				UART::write("\rERROR: Timeout waiting for ready to read\n");
				sd_error = result;
				return 0;
			}

			for (unsigned char d = 0; d < 128; ++d)
				buffer32[d] = MMIO::read(EMMC_DATA);

			++c;
			buffer32 += 128;
		}

		if (1 < num && !(sd_scr[0] & SCR_SUPP_SET_BLKCNT) && (sd_scr[0] & SCR_SUPP_CCS))
			sendCommand(CMD_STOP_TRANS, 0);

		return sd_error != 0 || c != num? 0 : 512 * num;
	}

	/** Set SD clock frequency in Hz. */
	static int setFrequency(uint32_t frequency) {
		uint32_t divisor, c = 41666666 / frequency, x, shift = 32, h = 0;
		int timeout = 100000;

		while ((MMIO::read(EMMC_STATUS) & (SR_CMD_INHIBIT | SR_DAT_INHIBIT)) && timeout--)
			Timers::waitMicroseconds(1);

		if (timeout <= 0) {
			UART::write("ERROR: timeout waiting for inhibit flag\n");
			return ERROR;
		}

		MMIO::write(EMMC_CONTROL1, MMIO::read(EMMC_CONTROL1) & ~C1_CLK_EN);
		Timers::waitMicroseconds(10);

		x = c - 1;

		if (!x) {
			shift = 0;
		} else {
			if (!(x & 0xffff0000u)) {
				x <<= 16;
				shift -= 16;
			}

			if (!(x & 0xff000000u)) {
				x <<= 8;
				shift -= 8;
			}

			if (!(x & 0xf0000000u)) {
				x <<= 4;
				shift -= 4;
			}

			if (!(x & 0xc0000000u)) {
				x <<= 2;
				shift -= 2;
			}

			if (!(x & 0x80000000u)) {
				x <<= 1;
				shift -= 1;
			}

			if (shift > 0)
				--shift;

			if (shift > 7)
				shift = 7;
		}

		divisor = HOST_SPEC_V2 < sd_hv? c : (1 << shift);

		if (divisor <= 2) {
			divisor = 2;
			shift = 0;
		}

		printf("sd_clk divisor = 0x%x, shift = 0x%x\n", divisor, shift);

		if (HOST_SPEC_V2 < sd_hv)
			h = (divisor & 0x300) >> 2;

		divisor = ((divisor & 0x0ff) << 8) | h;
		MMIO::write(EMMC_CONTROL1, (MMIO::read(EMMC_CONTROL1) & 0xffff003f) | divisor);
		Timers::waitMicroseconds(10);
		MMIO::write(EMMC_CONTROL1, MMIO::read(EMMC_CONTROL1) | C1_CLK_EN);
		Timers::waitMicroseconds(10);

		timeout = 10000;
		while (!(MMIO::read(EMMC_CONTROL1) & C1_CLK_STABLE) && timeout--)
			Timers::waitMicroseconds(10);

		if (timeout <= 0) {
			UART::write("ERROR: failed to get stable clock\n");
			return ERROR;
		}

		return SUCCESS;
	}

	/** Initialize EMMC to read SDHC card */
	int init() {
		int64_t result, timeout, ccs = 0;

		// Thanks to rst on the Raspberry Pi forums
		MMIO::write(EMMC_CONTROL0, MMIO::read(EMMC_CONTROL0) | (0xf << 8));

		// GPIO_CD
		MMIO::write(GPIO::GPFSEL4, MMIO::read(GPIO::GPFSEL4) & ~(7 << 21));

		MMIO::write(GPIO::GPPUD, 2);
		Timers::waitCycles(150);
		MMIO::write(GPIO::GPPUDCLK1, 1 << 15);
		Timers::waitCycles(150);
		MMIO::write(GPIO::GPPUD, 0);
		MMIO::write(GPIO::GPPUDCLK1, 0);
		MMIO::write(GPIO::GPHEN1, MMIO::read(GPIO::GPHEN1 | (1 << 15)));

		// GPIO_CLK, GPIO_CMD
		MMIO::write(GPIO::GPFSEL4, MMIO::read(GPIO::GPFSEL4 | ((7 << 24) | (7 << 27))));
		MMIO::write(GPIO::GPPUD, 2);
		Timers::waitCycles(150);
		MMIO::write(GPIO::GPPUDCLK1, (1 << 16) | (1 << 17));
		Timers::waitCycles(150);
		MMIO::write(GPIO::GPPUD, 0);
		MMIO::write(GPIO::GPPUDCLK1, 0);

		// GPIO_DAT0, GPIO_DAT1, GPIO_DAT2, GPIO_DAT3
		MMIO::write(GPIO::GPFSEL5, MMIO::read(GPIO::GPFSEL5) | (7 | (7 << 3) | (7 << 6) | (7 << 9)));
		MMIO::write(GPIO::GPPUD, 2);
		Timers::waitCycles(150);
		MMIO::write(GPIO::GPPUDCLK1, (1 << 18) | (1 << 19) | (1 << 20) | (1 << 21));
		Timers::waitCycles(150);
		MMIO::write(GPIO::GPPUD, 0);
		MMIO::write(GPIO::GPPUDCLK1, 0);

		sd_hv = (MMIO::read(EMMC_SLOTISR_VER) & HOST_SPEC_NUM) >> HOST_SPEC_NUM_SHIFT;
		UART::write("EMMC: GPIO set up\n");

		// Reset the card.
		MMIO::write(EMMC_CONTROL0, 0);
		MMIO::write(EMMC_CONTROL1, MMIO::read(EMMC_CONTROL1) | C1_SRST_HC);

		timeout = 10000;
		do {
			Timers::waitMicroseconds(10);
		} while ((MMIO::read(EMMC_CONTROL1) & C1_SRST_HC) && timeout--);

		if (timeout <= 0) {
			UART::write("ERROR: failed to reset EMMC\n");
			return ERROR;
		}

		UART::write("EMMC: reset okay\n");

		MMIO::write(EMMC_CONTROL1, MMIO::read(EMMC_CONTROL1) | (C1_CLK_INTLEN | C1_TOUNIT_MAX));
		Timers::waitMicroseconds(10);

		// Set clock to setup frequency.
		if ((result = setFrequency(400000)))
			return result;

		MMIO::write(EMMC_INT_EN,   0xffffffff);
		MMIO::write(EMMC_INT_MASK, 0xffffffff);
		sd_scr[0] = sd_scr[1] = sd_rca = sd_error = 0;

		sendCommand(CMD_GO_IDLE, 0);
		if (sd_error)
			return sd_error;

		sendCommand(CMD_SEND_IF_COND, 0x000001aa);
		if (sd_error)
			return sd_error;

		timeout = 6;
		result = 0;
		while (!(result & ACMD41_CMD_COMPLETE) && timeout--) {
			Timers::waitCycles(400);
			result = sendCommand(CMD_SEND_OP_COND, ACMD41_ARG_HC);
			UART::write("EMMC: CMD_SEND_OP_COND returned ");
			if (result & ACMD41_CMD_COMPLETE)
				UART::write("COMPLETE ");
			if (result & ACMD41_VOLTAGE)
				UART::write("VOLTAGE ");
			if (result & ACMD41_CMD_CCS)
				UART::write("CCS ");
			printf("0%llx\n", result);

			if (sd_error != TIMEOUT && sd_error != SUCCESS) {
				UART::write("ERROR: EMMC ACMD41 returned error\n");
				return sd_error;
			}
		}

		if (!(result & ACMD41_CMD_COMPLETE) || timeout <= 0)
			return TIMEOUT;

		if (!(result & ACMD41_VOLTAGE))
			return ERROR;

		if (result & ACMD41_CMD_CCS)
			ccs = SCR_SUPP_CCS;

		sendCommand(CMD_ALL_SEND_CID, 0);

		sd_rca = sendCommand(CMD_SEND_REL_ADDR, 0);
		printf("EMMC: CMD_SEND_REL_ADDR returned 0x%llx\n", sd_rca);

		if (sd_error)
			return sd_error;

		if ((result = setFrequency(25'000'000)))
			return result;

		sendCommand(CMD_CARD_SELECT,sd_rca);
		if (sd_error)
			return sd_error;

		if (status(SR_DAT_INHIBIT))
			return TIMEOUT;

		MMIO::write(EMMC_BLKSIZECNT, (1 << 16) | 8);
		sendCommand(CMD_SEND_SCR, 0);

		if (sd_error)
			return sd_error;

		if (waitForInterrupt(INT_READ_RDY))
			return TIMEOUT;

		int r = 0;
		timeout = 100000;
		while (r < 2 && timeout--) {
			if (MMIO::read(EMMC_STATUS) & SR_READ_AVAILABLE)
				sd_scr[r++] = MMIO::read(EMMC_DATA);
			else
				Timers::waitMicroseconds(1);
		}

		if (result != 2)
			return TIMEOUT;

		if (sd_scr[0] & SCR_SD_BUS_WIDTH_4) {
			sendCommand(CMD_SET_BUS_WIDTH, sd_rca | 2);
			if (sd_error)
				return sd_error;
			MMIO::write(EMMC_CONTROL0, MMIO::read(EMMC_CONTROL0) | C0_HCTL_DWITDH);
		}

		// Add software flag
		printf("EMMC: supports %s\n", sd_scr[0] & SCR_SUPP_SET_BLKCNT? "SET_BLKCNT" : "CCS");
		sd_scr[0] &= ~SCR_SUPP_CCS;
		sd_scr[0] |= ccs;
		return SUCCESS;
	}
}
