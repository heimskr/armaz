//
// emmc.cpp
//
// Provides an interface to the EMMC controller and commands for interacting
// with an sd card
//
// Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
//
// Modified for Circle by R. Stange
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// References:
//
// PLSS - SD Group Physical Layer Simplified Specification ver 3.00
// HCSS - SD Group Host Controller Simplified Specification ver 3.00
//
// Broadcom BCM2835 ARM Peripherals Guide
//
#include <assert.h>

#include "BCM2711.h"
#include "EMMC.h"
#include "Log.h"
#include "MMIO.h"
#include "RPi.h"
#include "Synchronize.h"
#include "Timer.h"

#ifndef USE_SDHOST
#include "PropertyTags.h"
#else
#include "mmc.h"
#include "mmcerror.h"
#endif

//
// Configuration options
//

#define EMMC_DEBUG
//#define EMMC_DEBUG2

//
// According to the BCM2835 ARM Peripherals Guide the EMMC STATUS register
// should not be used for polling. The original driver does not meet this
// specification in this point but the modified driver should do so.
// Define EMMC_POLL_STATUS_REG if you want the original function!
//
//#define EMMC_POLL_STATUS_REG

// Enable 1.8V support
//#define SD_1_8V_SUPPORT

// Enable High Speed/SDR25 mode
//#define SD_HIGH_SPEED

// Enable 4-bit support
#define SD_4BIT_DATA

// SD Clock Frequencies (in Hz)
#define SD_CLOCK_ID         400'000
#define SD_CLOCK_NORMAL  25'000'000
#define SD_CLOCK_HIGH    50'000'000
#define SD_CLOCK_100    100'000'000
#define SD_CLOCK_208    208'000'000

// Enable SDXC maximum performance mode
// Requires 150 mA power so disabled on the RPi for now
#define SDXC_MAXIMUM_PERFORMANCE

#ifndef USE_SDHOST

// Enable card interrupts
//#define SD_CARD_INTERRUPTS

// Allow old sdhci versions (may cause errors)
// Required for QEMU
#define EMMC_ALLOW_OLD_SDHCI

#if RASPPI <= 3
#define EMMC_BASE ARM_EMMC_BASE
#else
#define EMMC_BASE ARM_EMMC2_BASE
#endif

#define EMMC_ARG2           (EMMC_BASE + 0x00)
#define EMMC_BLKSIZECNT     (EMMC_BASE + 0x04)
#define EMMC_ARG1           (EMMC_BASE + 0x08)
#define EMMC_CMDTM          (EMMC_BASE + 0x0C)
#define EMMC_RESP0          (EMMC_BASE + 0x10)
#define EMMC_RESP1          (EMMC_BASE + 0x14)
#define EMMC_RESP2          (EMMC_BASE + 0x18)
#define EMMC_RESP3          (EMMC_BASE + 0x1C)
#define EMMC_DATA           (EMMC_BASE + 0x20)
#define EMMC_STATUS         (EMMC_BASE + 0x24)
#define EMMC_CONTROL0       (EMMC_BASE + 0x28)
#define EMMC_CONTROL1       (EMMC_BASE + 0x2C)
#define EMMC_INTERRUPT      (EMMC_BASE + 0x30)
#define EMMC_IRPT_MASK      (EMMC_BASE + 0x34)
#define EMMC_IRPT_EN        (EMMC_BASE + 0x38)
#define EMMC_CONTROL2       (EMMC_BASE + 0x3C)
#define EMMC_CAPABILITIES_0 (EMMC_BASE + 0x40)
#define EMMC_CAPABILITIES_1 (EMMC_BASE + 0x44)
#define EMMC_FORCE_IRPT     (EMMC_BASE + 0x50)
#define EMMC_BOOT_TIMEOUT   (EMMC_BASE + 0x70)
#define EMMC_DBG_SEL        (EMMC_BASE + 0x74)
#define EMMC_EXRDFIFO_CFG   (EMMC_BASE + 0x80)
#define EMMC_EXRDFIFO_EN    (EMMC_BASE + 0x84)
#define EMMC_TUNE_STEP      (EMMC_BASE + 0x88)
#define EMMC_TUNE_STEPS_STD (EMMC_BASE + 0x8C)
#define EMMC_TUNE_STEPS_DDR (EMMC_BASE + 0x90)
#define EMMC_SPI_INT_SPT    (EMMC_BASE + 0xF0)
#define EMMC_SLOTISR_VER    (EMMC_BASE + 0xFC)

#endif

#define SD_CMD_INDEX(a)          ((a) << 24)
#define SD_CMD_TYPE_NORMAL       0
#define SD_CMD_TYPE_SUSPEND	     (1 << 22)
#define SD_CMD_TYPE_RESUME       (2 << 22)
#define SD_CMD_TYPE_ABORT        (3 << 22)
#define SD_CMD_TYPE_MASK         (3 << 22)
#define SD_CMD_ISDATA            (1 << 21)
#define SD_CMD_IXCHK_EN	         (1 << 20)
#define SD_CMD_CRCCHK_EN         (1 << 19)
#define SD_CMD_RSPNS_TYPE_NONE   0         // For no response
#define SD_CMD_RSPNS_TYPE_136    (1 << 16) // For response R2 (with CRC), R3,4 (no CRC)
#define SD_CMD_RSPNS_TYPE_48     (2 << 16) // For responses R1, R5, R6, R7 (with CRC)
#define SD_CMD_RSPNS_TYPE_48B    (3 << 16) // For responses R1b, R5b (with CRC)
#define SD_CMD_RSPNS_TYPE_MASK   (3 << 16)
#define SD_CMD_MULTI_BLOCK       (1 << 5)
#define SD_CMD_DAT_DIR_HC        0
#define SD_CMD_DAT_DIR_CH        (1 << 4)
#define SD_CMD_AUTO_CMD_EN_NONE  0
#define SD_CMD_AUTO_CMD_EN_CMD12 (1 << 2)
#define SD_CMD_AUTO_CMD_EN_CMD23 (2 << 2)
#define SD_CMD_BLKCNT_EN         (1 << 1)
#define SD_CMD_DMA               1

#ifndef USE_SDHOST

#define SD_ERR_CMD_TIMEOUT   0
#define SD_ERR_CMD_CRC       1
#define SD_ERR_CMD_END_BIT   2
#define SD_ERR_CMD_INDEX     3
#define SD_ERR_DATA_TIMEOUT  4
#define SD_ERR_DATA_CRC	     5
#define SD_ERR_DATA_END_BIT  6
#define SD_ERR_CURRENT_LIMIT 7
#define SD_ERR_AUTO_CMD12    8
#define SD_ERR_ADMA          9
#define SD_ERR_TUNING        10
#define SD_ERR_RSVD          11

#define SD_ERR_MASK_CMD_TIMEOUT   (1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_CMD_CRC       (1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_CMD_END_BIT	  (1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CMD_INDEX     (1 << (16 + SD_ERR_CMD_INDEX))
#define SD_ERR_MASK_DATA_TIMEOUT  (1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_DATA_CRC      (1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_DATA_END_BIT  (1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CURRENT_LIMIT (1 << (16 + SD_ERR_CMD_CURRENT_LIMIT))
#define SD_ERR_MASK_AUTO_CMD12    (1 << (16 + SD_ERR_CMD_AUTO_CMD12))
#define SD_ERR_MASK_ADMA          (1 << (16 + SD_ERR_CMD_ADMA))
#define SD_ERR_MASK_TUNING        (1 << (16 + SD_ERR_CMD_TUNING))

#define SD_COMMAND_COMPLETE     1
#define SD_TRANSFER_COMPLETE    (1 << 1)
#define SD_BLOCK_GAP_EVENT      (1 << 2)
#define SD_DMA_INTERRUPT        (1 << 3)
#define SD_BUFFER_WRITE_READY   (1 << 4)
#define SD_BUFFER_READ_READY    (1 << 5)
#define SD_CARD_INSERTION       (1 << 6)
#define SD_CARD_REMOVAL         (1 << 7)
#define SD_CARD_INTERRUPT       (1 << 8)

#endif

#define SD_RESP_NONE        SD_CMD_RSPNS_TYPE_NONE
#define SD_RESP_R1          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R1b         (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R2          (SD_CMD_RSPNS_TYPE_136 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R3          SD_CMD_RSPNS_TYPE_48
#define SD_RESP_R4          SD_CMD_RSPNS_TYPE_136
#define SD_RESP_R5          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R5b         (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R6          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R7          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)

#define SD_DATA_READ        (SD_CMD_ISDATA | SD_CMD_DAT_DIR_CH)
#define SD_DATA_WRITE       (SD_CMD_ISDATA | SD_CMD_DAT_DIR_HC)

#define SD_CMD_RESERVED(a)  0xffffffff

#define FAIL             (lastCmdSuccess == 0)

#ifndef USE_SDHOST
#define TIMEOUT          (FAIL && (lastError == 0))
#define CMD_TIMEOUT      (FAIL && (lastError & (1 << 16)))
#define CMD_CRC          (FAIL && (lastError & (1 << 17)))
#define CMD_END_BIT      (FAIL && (lastError & (1 << 18)))
#define CMD_INDEX        (FAIL && (lastError & (1 << 19)))
#define DATA_TIMEOUT     (FAIL && (lastError & (1 << 20)))
#define DATA_CRC         (FAIL && (lastError & (1 << 21)))
#define DATA_END_BIT     (FAIL && (lastError & (1 << 22)))
#define CURRENT_LIMIT    (FAIL && (lastError & (1 << 23)))
#define ACMD12_ERROR     (FAIL && (lastError & (1 << 24)))
#define ADMA_ERROR       (FAIL && (lastError & (1 << 25)))
#define TUNING_ERROR     (FAIL && (lastError & (1 << 26)))
#else
#define TIMEOUT          (FAIL && (lastError == ETIMEDOUT))
#endif

#define SD_VER_UNKNOWN      0
#define SD_VER_1            1
#define SD_VER_1_1          2
#define SD_VER_2            3
#define SD_VER_3            4
#define SD_VER_4            5

namespace Armaz {
	const char *EMMCDevice::sdVersions[] = {
		"unknown",
		"1.0 or 1.01",
		"1.10",
		"2.00",
		"3.0x",
		"4.xx"
	};

#ifndef USE_SDHOST

#ifdef EMMC_DEBUG2
	const char *EMMCDevice::errIrpts[] = {
		"CMD_TIMEOUT",
		"CMD_CRC",
		"CMD_END_BIT",
		"CMD_INDEX",
		"DATA_TIMEOUT",
		"DATA_CRC",
		"DATA_END_BIT",
		"CURRENT_LIMIT",
		"AUTO_CMD12",
		"ADMA",
		"TUNING",
		"RSVD"
	};
#endif

#endif

	const uint32_t EMMCDevice::sdCommands[] = {
		SD_CMD_INDEX(0),
#ifdef USE_EMBEDDED_MMC_CM4
		SD_CMD_INDEX(1) | SD_RESP_R3,
#else
		SD_CMD_RESERVED(1),
#endif
		SD_CMD_INDEX(2) | SD_RESP_R2,
		SD_CMD_INDEX(3) | SD_RESP_R6,
		SD_CMD_INDEX(4),
		SD_CMD_INDEX(5) | SD_RESP_R4,
#ifdef USE_EMBEDDED_MMC_CM4
		SD_CMD_INDEX(6) | SD_RESP_R1,
#else
		SD_CMD_INDEX(6) | SD_RESP_R1 | SD_DATA_READ,
#endif
		SD_CMD_INDEX(7) | SD_RESP_R1b,
		SD_CMD_INDEX(8) | SD_RESP_R7,
		SD_CMD_INDEX(9) | SD_RESP_R2,
		SD_CMD_INDEX(10) | SD_RESP_R2,
		SD_CMD_INDEX(11) | SD_RESP_R1,
		SD_CMD_INDEX(12) | SD_RESP_R1b | SD_CMD_TYPE_ABORT,
		SD_CMD_INDEX(13) | SD_RESP_R1,
		SD_CMD_RESERVED(14),
		SD_CMD_INDEX(15),
		SD_CMD_INDEX(16) | SD_RESP_R1,
		SD_CMD_INDEX(17) | SD_RESP_R1 | SD_DATA_READ,
		SD_CMD_INDEX(18) | SD_RESP_R1 | SD_DATA_READ | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN | SD_CMD_AUTO_CMD_EN_CMD12, // SD_CMD_AUTO_CMD_EN_CMD12 not in original driver
		SD_CMD_INDEX(19) | SD_RESP_R1 | SD_DATA_READ,
		SD_CMD_INDEX(20) | SD_RESP_R1b,
		SD_CMD_RESERVED(21),
		SD_CMD_RESERVED(22),
		SD_CMD_INDEX(23) | SD_RESP_R1,
		SD_CMD_INDEX(24) | SD_RESP_R1 | SD_DATA_WRITE,
		SD_CMD_INDEX(25) | SD_RESP_R1 | SD_DATA_WRITE | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN | SD_CMD_AUTO_CMD_EN_CMD12, // SD_CMD_AUTO_CMD_EN_CMD12 not in original driver
		SD_CMD_RESERVED(26),
		SD_CMD_INDEX(27) | SD_RESP_R1 | SD_DATA_WRITE,
		SD_CMD_INDEX(28) | SD_RESP_R1b,
		SD_CMD_INDEX(29) | SD_RESP_R1b,
		SD_CMD_INDEX(30) | SD_RESP_R1 | SD_DATA_READ,
		SD_CMD_RESERVED(31),
		SD_CMD_INDEX(32) | SD_RESP_R1,
		SD_CMD_INDEX(33) | SD_RESP_R1,
		SD_CMD_RESERVED(34),
		SD_CMD_RESERVED(35),
		SD_CMD_RESERVED(36),
		SD_CMD_RESERVED(37),
		SD_CMD_INDEX(38) | SD_RESP_R1b,
		SD_CMD_RESERVED(39),
		SD_CMD_RESERVED(40),
		SD_CMD_RESERVED(41),
		SD_CMD_RESERVED(42) | SD_RESP_R1,
		SD_CMD_RESERVED(43),
		SD_CMD_RESERVED(44),
		SD_CMD_RESERVED(45),
		SD_CMD_RESERVED(46),
		SD_CMD_RESERVED(47),
		SD_CMD_RESERVED(48),
		SD_CMD_RESERVED(49),
		SD_CMD_RESERVED(50),
		SD_CMD_RESERVED(51),
		SD_CMD_RESERVED(52),
		SD_CMD_RESERVED(53),
		SD_CMD_RESERVED(54),
		SD_CMD_INDEX(55) | SD_RESP_R1,
		SD_CMD_INDEX(56) | SD_RESP_R1 | SD_CMD_ISDATA,
		SD_CMD_RESERVED(57),
		SD_CMD_RESERVED(58),
		SD_CMD_RESERVED(59),
		SD_CMD_RESERVED(60),
		SD_CMD_RESERVED(61),
		SD_CMD_RESERVED(62),
		SD_CMD_RESERVED(63)
	};

	const uint32_t EMMCDevice::sdACommands[] = {
		SD_CMD_RESERVED(0),
		SD_CMD_RESERVED(1),
		SD_CMD_RESERVED(2),
		SD_CMD_RESERVED(3),
		SD_CMD_RESERVED(4),
		SD_CMD_RESERVED(5),
		SD_CMD_INDEX(6) | SD_RESP_R1,
		SD_CMD_RESERVED(7),
		SD_CMD_RESERVED(8),
		SD_CMD_RESERVED(9),
		SD_CMD_RESERVED(10),
		SD_CMD_RESERVED(11),
		SD_CMD_RESERVED(12),
		SD_CMD_INDEX(13) | SD_RESP_R1,
		SD_CMD_RESERVED(14),
		SD_CMD_RESERVED(15),
		SD_CMD_RESERVED(16),
		SD_CMD_RESERVED(17),
		SD_CMD_RESERVED(18),
		SD_CMD_RESERVED(19),
		SD_CMD_RESERVED(20),
		SD_CMD_RESERVED(21),
		SD_CMD_INDEX(22) | SD_RESP_R1 | SD_DATA_READ,
		SD_CMD_INDEX(23) | SD_RESP_R1,
		SD_CMD_RESERVED(24),
		SD_CMD_RESERVED(25),
		SD_CMD_RESERVED(26),
		SD_CMD_RESERVED(27),
		SD_CMD_RESERVED(28),
		SD_CMD_RESERVED(29),
		SD_CMD_RESERVED(30),
		SD_CMD_RESERVED(31),
		SD_CMD_RESERVED(32),
		SD_CMD_RESERVED(33),
		SD_CMD_RESERVED(34),
		SD_CMD_RESERVED(35),
		SD_CMD_RESERVED(36),
		SD_CMD_RESERVED(37),
		SD_CMD_RESERVED(38),
		SD_CMD_RESERVED(39),
		SD_CMD_RESERVED(40),
		SD_CMD_INDEX(41) | SD_RESP_R3,
		SD_CMD_INDEX(42) | SD_RESP_R1,
		SD_CMD_RESERVED(43),
		SD_CMD_RESERVED(44),
		SD_CMD_RESERVED(45),
		SD_CMD_RESERVED(46),
		SD_CMD_RESERVED(47),
		SD_CMD_RESERVED(48),
		SD_CMD_RESERVED(49),
		SD_CMD_RESERVED(50),
		SD_CMD_INDEX(51) | SD_RESP_R1 | SD_DATA_READ,
		SD_CMD_RESERVED(52),
		SD_CMD_RESERVED(53),
		SD_CMD_RESERVED(54),
		SD_CMD_RESERVED(55),
		SD_CMD_RESERVED(56),
		SD_CMD_RESERVED(57),
		SD_CMD_RESERVED(58),
		SD_CMD_RESERVED(59),
		SD_CMD_RESERVED(60),
		SD_CMD_RESERVED(61),
		SD_CMD_RESERVED(62),
		SD_CMD_RESERVED(63)
	};

	// The actual command indices
#define GO_IDLE_STATE           0
#define SEND_OP_COND            1
#define ALL_SEND_CID            2
#define SEND_RELATIVE_ADDR      3
#define SET_DSR                 4
#define IO_SET_OP_COND          5
#define SWITCH_FUNC             6
#define SELECT_CARD             7
#define DESELECT_CARD           7
#define SELECT_DESELECT_CARD    7
#define SEND_IF_COND            8
#define SEND_CSD                9
#define SEND_CID                10
#define VOLTAGE_SWITCH          11
#define STOP_TRANSMISSION       12
#define SEND_STATUS             13
#define GO_INACTIVE_STATE       15
#define SET_BLOCKLEN            16
#define READ_SINGLE_BLOCK       17
#define READ_MULTIPLE_BLOCK     18
#define SEND_TUNING_BLOCK       19
#define SPEED_CLASS_CONTROL     20
#define SET_BLOCK_COUNT         23
#define WRITE_BLOCK             24
#define WRITE_MULTIPLE_BLOCK    25
#define PROGRAM_CSD             27
#define SET_WRITE_PROT          28
#define CLR_WRITE_PROT          29
#define SEND_WRITE_PROT         30
#define ERASE_WR_BLK_START      32
#define ERASE_WR_BLK_END        33
#define ERASE                   38
#define LOCK_UNLOCK             42
#define APP_CMD                 55
#define GEN_CMD                 56

#define IS_APP_CMD              0x80000000
#define ACMD(a)                 (a | IS_APP_CMD)
#define SET_BUS_WIDTH           (6 | IS_APP_CMD)
#define SD_STATUS               (13 | IS_APP_CMD)
#define SEND_NUM_WR_BLOCKS      (22 | IS_APP_CMD)
#define SET_WR_BLK_ERASE_COUNT  (23 | IS_APP_CMD)
#define SD_SEND_OP_COND         (41 | IS_APP_CMD)
#define SET_CLR_CARD_DETECT     (42 | IS_APP_CMD)
#define SEND_SCR                (51 | IS_APP_CMD)

#ifndef USE_SDHOST

#define SD_RESET_CMD            (1 << 25)
#define SD_RESET_DAT            (1 << 26)
#define SD_RESET_ALL            (1 << 24)

#define SD_GET_CLOCK_DIVIDER_FAIL 0xffffffff

#endif

#define SD_BLOCK_SIZE 512

	EMMCDevice::EMMCDevice():
		offset(0),
#ifdef USE_SDHOST
		host(timer_),
#else
		hciVersion(0),
#endif
		sdConfig(nullptr) {

		sdConfig = new SDConfiguration;
		assert(sdConfig);

#ifndef USE_SDHOST

#if RASPPI == 3
		RPi::Model model = RPi::getModel();
		// workaround if bootloader does not restore GPIO modes
		if (model == RPi::Model::Pi3B || model == RPi::Model::Pi3APlus || model == RPi::Model::Pi3BPlus) {
			for (unsigned i = 0; i <= 5; ++i) {
				gpio34_39[i].assignPin(34 + i);
				gpio34_39[i].setMode(GPIOModeInput, false);

				gpio48_53[i].assignPin(48 + i);
				gpio48_53[i].setMode(GPIOModeAlternateFunction3, false);
			}
		}
#endif

#endif
	}

	EMMCDevice::~EMMCDevice() {
#ifdef USE_SDHOST
		host.reset();
#endif
		delete sdConfig;
		// delete partitionManager;
	}

	bool EMMCDevice::initialize() {
#ifndef USE_SDHOST
#if RASPPI >= 4
		// disable 1.8V supply
		PropertyTagGPIOState gpio_state;
		gpio_state.gpio = EXP_GPIO_BASE + 4;
		gpio_state.state = 0;
		if (!PropertyTags::getTag(PROPTAG_SET_SET_GPIO_STATE, &gpio_state, sizeof(gpio_state), 8))
			return false;

		Timers::waitMicroseconds(5000);
#endif
#else
		if (!host.initialize())
			return false;
#endif

		peripheralEntry();

		if (cardInit() != 0)
			return false;

		peripheralExit();

		// const char device_name[] = "emmc1";

		// assert(partitionManager == nullptr);
		// partitionManager = new PartitionManager(this, device_name);
		// assert(partitionManager != nullptr);
		// if (!partitionManager->initialize())
		// 	return false;

		// CDeviceNameService::Get()->AddDevice(device_name, this, true);
		return true;
	}

	size_t EMMCDevice::read(void *buffer, size_t count) {
		if (offset % SD_BLOCK_SIZE != 0)
			return -1;
		uint32_t block = offset / SD_BLOCK_SIZE;

		peripheralEntry();

		if (doRead((uint8_t *) buffer, count, block) != count) {
			peripheralExit();
			return -1;
		}

		peripheralExit();
		return count;
	}

	size_t EMMCDevice::write(const void *buffer, size_t count) {
		if (offset % SD_BLOCK_SIZE != 0)
			return -1;
		uint32_t block = offset / SD_BLOCK_SIZE;

		peripheralEntry();

		if (doWrite((uint8_t *) buffer, count, block) != count) {
			peripheralExit();
			return -1;
		}

		peripheralExit();

		return count;
	}

	uint64_t EMMCDevice::seek(uint64_t new_offset) {
		offset = new_offset;
		return offset;
	}

#ifndef USE_SDHOST

	bool EMMCDevice::powerOn() {
		PropertyTagPowerState powerState;
		powerState.deviceID = DEVICE_ID_SD_CARD;
		powerState.state = POWER_STATE_ON | POWER_STATE_WAIT;
		if (!PropertyTags::getTag(PROPTAG_SET_POWER_STATE, &powerState, sizeof(powerState))
			|| (powerState.state & POWER_STATE_NO_DEVICE) || !(powerState.state & POWER_STATE_ON)) {
			Log::error("Device did not power on successfully");
			return false;
		}

		return true;
	}

	void EMMCDevice::powerOff() {
		// Power off the SD card
		uint32_t control0 = read32(EMMC_CONTROL0);
		control0 &= ~(1 << 8);	// Set SD Bus Power bit off in Power Control Register
		write32(EMMC_CONTROL0, control0);
	}

	// Get the current base clock rate in Hz
	uint32_t EMMCDevice::getBaseClock() {
		PropertyTagClockRate clock_rate;
#if RASPPI <= 3
		clock_rate.clockID = CLOCK_ID_EMMC;
#else
		clock_rate.clockID = CLOCK_ID_EMMC2;
#endif
		if (!PropertyTags::getTag(PROPTAG_GET_CLOCK_RATE, &clock_rate, sizeof(clock_rate))) {
			Log::error("Cannot get clock rate");
			clock_rate.rate = 0;
		}

#ifdef EMMC_DEBUG2
		Log::info("Base clock rate is %u Hz", clock_rate.rate);
#endif

		return clock_rate.rate;
	}

	// Set the clock dividers to generate a target value
	uint32_t EMMCDevice::getClockDivider(uint32_t base_clock, uint32_t target_rate) {
		// TODO: implement use of preset value registers

		uint32_t targeted_divisor = 1;
		if (target_rate <= base_clock) {
			targeted_divisor = base_clock / target_rate;
			if (base_clock % target_rate)
				--targeted_divisor;
		}

		// Decide on the clock mode to use
		// Currently only 10-bit divided clock mode is supported

#ifndef EMMC_ALLOW_OLD_SDHCI
		if (hciVersion >= 2) {
#endif
			// HCI version 3 or greater supports 10-bit divided clock mode
			// This requires a power-of-two divider

			// Find the first bit set
			int divisor = -1;
			for (int first_bit = 31; first_bit >= 0; --first_bit) {
				const uint32_t bit_test = 1 << first_bit;
				if (targeted_divisor & bit_test) {
					divisor = first_bit;
					targeted_divisor &= ~bit_test;
					if (targeted_divisor)
						// The divisor is not a power-of-two, increase it
						++divisor;
					break;
				}
			}

			if (divisor == -1)
				divisor = 31;

			if (divisor >= 32)
				divisor = 31;

			if (divisor != 0)
				divisor = (1 << (divisor - 1));

			if (divisor >= 0x400)
				divisor = 0x3ff;

			const uint32_t freq_select = divisor & 0xff;
			const uint32_t upper_bits = (divisor >> 8) & 0x3;
			const uint32_t ret = (freq_select << 8) | (upper_bits << 6) | (0 << 5);

#ifdef EMMC_DEBUG2
			int denominator = 1;
			if (divisor != 0)
				denominator = divisor * 2;
			int actual_clock = base_clock / denominator;
			Log::info("base_clock: %d, target_rate: %d, divisor: %08x, actual_clock: %d, ret: %08x", base_clock, target_rate, divisor, actual_clock, ret);
#endif

			return ret;
#ifndef EMMC_ALLOW_OLD_SDHCI
		} else {
			Log::error("Unsupported host version");
			return SD_GET_CLOCK_DIVIDER_FAIL;
		}
#endif
	}

	// Switch the clock rate whilst running
	bool EMMCDevice::switchClockRate(uint32_t base_clock, uint32_t target_rate) {
		// Decide on an appropriate divider
		uint32_t divider = getClockDivider(base_clock, target_rate);
		if (divider == SD_GET_CLOCK_DIVIDER_FAIL) {
			Log::info("Couldn't get a valid divider for target rate %d Hz", target_rate);
			return false;
		}

		// Wait for the command inhibit (CMD and DAT) bits to clear
		while (read32(EMMC_STATUS) & 3)
			usDelay(1000);

		// Set the SD clock off
		uint32_t control1 = read32(EMMC_CONTROL1);
		control1 &= ~(1 << 2);
		write32(EMMC_CONTROL1, control1);
		usDelay(2000);

		// Write the new divider
		control1 &= ~0xffe0; // Clear old setting + clock generator select
		control1 |= divider;
		write32(EMMC_CONTROL1, control1);
		usDelay(2000);

		// Enable the SD clock
		control1 |= 1 << 2;
		write32(EMMC_CONTROL1, control1);
		usDelay(2000);

#ifdef EMMC_DEBUG2
		Log::info("Successfully set clock rate to %d Hz", target_rate);
#endif

		return true;
	}

	bool EMMCDevice::resetCmd() {
		write32(EMMC_CONTROL1, read32(EMMC_CONTROL1) | SD_RESET_CMD);

		if (timeoutWait(EMMC_CONTROL1, SD_RESET_CMD, 0, 1000000) < 0) {
			Log::error("CMD line did not reset properly");
			return false;
		}

		return true;
	}

	bool EMMCDevice::resetDat() {
		uint32_t control1 = read32(EMMC_CONTROL1);
		control1 |= SD_RESET_DAT;
		write32(EMMC_CONTROL1, control1);

		if (timeoutWait(EMMC_CONTROL1, SD_RESET_DAT, 0, 1000000) < 0) {
			Log::error("DAT line did not reset properly");
			return false;
		}

		return true;
	}

	void EMMCDevice::issueCommandInt(uint32_t cmd_reg, uint32_t argument, int timeout) {
		lastCmdReg = cmd_reg;
		lastCmdSuccess = 0;

		// This is as per HCSS 3.7.1.1/3.7.2.2

#ifdef EMMC_POLL_STATUS_REG
		// Check Command Inhibit
		while (read32(EMMC_STATUS) & 1)
			usDelay(1000);

		// Is the command with busy?
		if ((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B)
			// With busy

			// Is is an abort command?
			if ((cmd_reg & SD_CMD_TYPE_MASK) != SD_CMD_TYPE_ABORT)
				// Not an abort command

				// Wait for the data line to be free
				while (read32(EMMC_STATUS) & 2)
					usDelay(1000);
#endif

		// Set block size and block count
		// For now, block size = 512 bytes, block count = 1,
		if (blocksToTransfer > 0xffff) {
			Log::info("blocksToTransfer too great (%d)", blocksToTransfer);
			lastCmdSuccess = 0;
			return;
		}

		write32(EMMC_BLKSIZECNT, blockSize | (blocksToTransfer << 16));

		// Set argument 1 reg
		write32(EMMC_ARG1, argument);

		// Set command reg
		write32(EMMC_CMDTM, cmd_reg);

		//usDelay(2000);

		// Wait for command complete interrupt
		timeoutWait(EMMC_INTERRUPT, 0x8001, 1, timeout);
		uint32_t irpts = read32(EMMC_INTERRUPT);

		// Clear command complete status
		write32(EMMC_INTERRUPT, 0xffff0001);

		// Test for errors
		if ((irpts & 0xffff0001) != 1) {
#ifdef EMMC_DEBUG2
			Log::warn("Error occured whilst waiting for command complete interrupt");
#endif
			lastError = irpts & 0xffff0000;
			lastInterrupt = irpts;

			return;
		}

		//usDelay(2000);

		// Get response data
		switch (cmd_reg & SD_CMD_RSPNS_TYPE_MASK) {
			case SD_CMD_RSPNS_TYPE_48:
			case SD_CMD_RSPNS_TYPE_48B:
				lastR0 = read32(EMMC_RESP0);
				break;

			case SD_CMD_RSPNS_TYPE_136:
				lastR0 = read32(EMMC_RESP0);
				lastR1 = read32(EMMC_RESP1);
				lastR2 = read32(EMMC_RESP2);
				lastR3 = read32(EMMC_RESP3);
				break;
		}

		// If with data, wait for the appropriate interrupt
		if (cmd_reg & SD_CMD_ISDATA) {
			uint32_t wr_irpt;
			int is_write = 0;
			if (cmd_reg & SD_CMD_DAT_DIR_CH) {
				wr_irpt = (1 << 5);     // read
			} else {
				is_write = 1;
				wr_irpt = (1 << 4);     // write
			}

#ifdef EMMC_DEBUG2
			if (blocksToTransfer > 1)
				Log::info("Multi block transfer");
#endif

			assert(((uintptr_t) buf & 3) == 0);
			uint32_t *data = (uint32_t *) buf;

			for (int block = 0; block < blocksToTransfer; ++block) {
				timeoutWait(EMMC_INTERRUPT, wr_irpt | 0x8000, 1, timeout);
				irpts = read32(EMMC_INTERRUPT);
				write32(EMMC_INTERRUPT, 0xffff0000 | wr_irpt);

				if ((irpts & (0xffff0000 | wr_irpt)) != wr_irpt) {
#ifdef EMMC_DEBUG2
					Log::warn("Error occured whilst waiting for data ready interrupt");
#endif
					lastError = irpts & 0xffff0000;
					lastInterrupt = irpts;

					return;
				}

				// Transfer the block
				assert(blockSize <= 1024);		// internal FIFO size of EMMC
				size_t length = blockSize;
				assert((length & 3) == 0);

				if (is_write)
					for (; length > 0; length -= 4)
						write32(EMMC_DATA, *data++);
				else
					for (; length > 0; length -= 4)
						*data++ = read32(EMMC_DATA);
			}

#ifdef EMMC_DEBUG2
			Log::info("Block transfer complete");
#endif
		}

		// Wait for transfer complete (set if read/write transfer or with busy)
		if ((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B || (cmd_reg & SD_CMD_ISDATA)) {
#ifdef EMMC_POLL_STATUS_REG
			// First check command inhibit (DAT) is not already 0
			if ((read32(EMMC_STATUS) & 2) == 0)
				write32(EMMC_INTERRUPT, 0xffff0002);
			else
#endif
			{
				timeoutWait(EMMC_INTERRUPT, 0x8002, 1, timeout);
				irpts = read32(EMMC_INTERRUPT);
				write32(EMMC_INTERRUPT, 0xffff0002);

				// Handle the case where both data timeout and transfer complete
				//  are set - transfer complete overrides data timeout: HCSS 2.2.17
				if (((irpts & 0xffff0002) != 2) && ((irpts & 0xffff0002) != 0x100002)) {
#ifdef EMMC_DEBUG
					Log::warn("Error occured whilst waiting for transfer complete interrupt");
#endif
					lastError = irpts & 0xffff0000;
					lastInterrupt = irpts;
					return;
				}

				write32(EMMC_INTERRUPT, 0xffff0002);
			}
		}

		// Return success
		lastCmdSuccess = 1;
	}

	void EMMCDevice::handleCardInterrupt() {
		// Handle a card interrupt

#ifdef EMMC_DEBUG2
		uint32_t status = read32(EMMC_STATUS);

		Log::info("Card interrupt");
		Log::info("controller status: %08x", status);
#endif

		// Get the card status
		if (cardRCA != CARD_RCA_INVALID) {
			issueCommandInt(sdCommands[SEND_STATUS], cardRCA << 16, 500000);
			if (FAIL) {
#ifdef EMMC_DEBUG
				Log::warn("Unable to get card status");
#endif
			} else {
#ifdef EMMC_DEBUG2
				Log::info("card status: %08x", lastR0);
#endif
			}
		} else {
#ifdef EMMC_DEBUG2
			Log::info("no card currently selected");
#endif
		}
	}

	void EMMCDevice::handleInterrupts() {
		uint32_t irpts = read32(EMMC_INTERRUPT);
		uint32_t reset_mask = 0;

		if (irpts & SD_COMMAND_COMPLETE) {
#ifdef EMMC_DEBUG2
			Log::info("spurious command complete interrupt");
#endif
			reset_mask |= SD_COMMAND_COMPLETE;
		}

		if (irpts & SD_TRANSFER_COMPLETE) {
#ifdef EMMC_DEBUG2
			Log::info("spurious transfer complete interrupt");
#endif
			reset_mask |= SD_TRANSFER_COMPLETE;
		}

		if (irpts & SD_BLOCK_GAP_EVENT) {
#ifdef EMMC_DEBUG2
			Log::info("spurious block gap event interrupt");
#endif
			reset_mask |= SD_BLOCK_GAP_EVENT;
		}

		if (irpts & SD_DMA_INTERRUPT) {
#ifdef EMMC_DEBUG2
			Log::info("spurious DMA interrupt");
#endif
			reset_mask |= SD_DMA_INTERRUPT;
		}

		if (irpts & SD_BUFFER_WRITE_READY) {
#ifdef EMMC_DEBUG2
			Log::info("spurious buffer write ready interrupt");
#endif
			reset_mask |= SD_BUFFER_WRITE_READY;
			resetDat();
		}

		if (irpts & SD_BUFFER_READ_READY) {
#ifdef EMMC_DEBUG2
			Log::info("spurious buffer read ready interrupt");
#endif
			reset_mask |= SD_BUFFER_READ_READY;
			resetDat();
		}

		if (irpts & SD_CARD_INSERTION) {
#ifdef EMMC_DEBUG2
			Log::info("card insertion detected");
#endif
			reset_mask |= SD_CARD_INSERTION;
		}

		if (irpts & SD_CARD_REMOVAL) {
#ifdef EMMC_DEBUG2
			Log::info("card removal detected");
#endif
			reset_mask |= SD_CARD_REMOVAL;
			cardRemoval = 1;
		}

		if (irpts & SD_CARD_INTERRUPT) {
#ifdef EMMC_DEBUG2
			Log::info("card interrupt detected");
#endif
			handleCardInterrupt();
			reset_mask |= SD_CARD_INTERRUPT;
		}

		if (irpts & 0x8000) {
#ifdef EMMC_DEBUG2
			Log::info("spurious error interrupt: %08x", irpts);
#endif
			reset_mask |= 0xffff0000;
		}

		write32(EMMC_INTERRUPT, reset_mask);
	}

#else	// #ifndef USE_SDHOST

	void EMMCDevice::issueCommandInt(uint32_t cmd_reg, uint32_t argument, int timeout) {
		lastCmdReg = cmd_reg;
		lastCmdSuccess = 0;

		// Set block size and block count
		// For now, block size = 512 bytes, block count = 1,
		if (blocksToTransfer > 0xffff) {
			Log::info("blocks_to_transfer too great (%d)", blocksToTransfer);
			return;
		}

		mmc_command Cmd;
		memset(&Cmd, 0, sizeof Cmd);
		Cmd.opcode = cmd_reg >> 24;
		Cmd.arg = argument;

		switch (cmd_reg & SD_CMD_RSPNS_TYPE_MASK) {
			case SD_CMD_RSPNS_TYPE_48:
				Cmd.flags |= MMC_RSP_PRESENT;
				break;

			case SD_CMD_RSPNS_TYPE_48B:
				Cmd.flags |= MMC_RSP_PRESENT | MMC_RSP_BUSY;
				break;

			case SD_CMD_RSPNS_TYPE_136:
				Cmd.flags |= MMC_RSP_PRESENT | MMC_RSP_136;
				break;
		}

		if (cmd_reg & SD_CMD_CRCCHK_EN)
			Cmd.flags |= MMC_RSP_CRC;

		mmc_data Data;
		if (cmd_reg & SD_CMD_ISDATA) {
			memset(&Data, 0, sizeof Data);
			Data.flags |= cmd_reg & SD_CMD_DAT_DIR_CH ? MMC_DATA_READ : MMC_DATA_WRITE;
			Data.blksz = blockSize;
			Data.blocks = blocksToTransfer;
			Data.sg = buf;
			Data.sg_len = blockSize * blocksToTransfer;
			Cmd.data = &Data;
		}

		int nError = host.Command(&Cmd, 0);
		if (nError != 0) {
			assert(nError < 0);
			lastError = -nError;
			return;
		}

		// Get response data
		switch (cmd_reg & SD_CMD_RSPNS_TYPE_MASK) {
			case SD_CMD_RSPNS_TYPE_48:
			case SD_CMD_RSPNS_TYPE_48B:
				lastR0 = Cmd.resp[0];
				break;

			case SD_CMD_RSPNS_TYPE_136:
				lastR0 = Cmd.resp[3];
				lastR1 = Cmd.resp[2];
				lastR2 = Cmd.resp[1];
				lastR3 = Cmd.resp[0];
				break;
		}

			// Return success
			lastCmdSuccess = 1;
	}

#endif	// #ifndef USE_SDHOST


	bool EMMCDevice::issueCommand(uint32_t command, uint32_t argument, int timeout) {
#ifndef USE_SDHOST
		// First, handle any pending interrupts
		handleInterrupts();

		// Stop the command issue if it was the card remove interrupt that was handled
		if (cardRemoval) {
			lastCmdSuccess = 0;
			return false;
		}
#endif

		// Now run the appropriate commands by calling issueCommandInt()
		if (command & IS_APP_CMD) {
			command &= 0xff;
#ifdef EMMC_DEBUG2
			Log::info("Issuing command ACMD%d", command);
#endif

			if (sdACommands[command] == SD_CMD_RESERVED(0)) {
				Log::error("Invalid command ACMD%d", command);
				lastCmdSuccess = 0;
				return false;
			}
			lastCmd = APP_CMD;

			uint32_t rca = 0;
			if (cardRCA != CARD_RCA_INVALID)
				rca = cardRCA << 16;
			issueCommandInt(sdCommands[APP_CMD], rca, timeout);
			if (lastCmdSuccess) {
				lastCmd = command | IS_APP_CMD;
				issueCommandInt(sdACommands[command], argument, timeout);
			}
		} else {
#ifdef EMMC_DEBUG2
			Log::info("Issuing command CMD%d", command);
#endif

			if (sdCommands[command] == SD_CMD_RESERVED(0)) {
				Log::error("Invalid command CMD%d", command);
				lastCmdSuccess = 0;
				return false;
			}

			lastCmd = command;
			issueCommandInt(sdCommands[command], argument, timeout);
		}

#ifdef EMMC_DEBUG2
		if (FAIL) {
#ifndef USE_SDHOST
			Log::warn("Error issuing %s%u (intr %08x)", lastCmd & IS_APP_CMD ? "ACMD" : "CMD", lastCmd & 0xff, lastInterrupt);

			if (lastError == 0)
				Log::info("TIMEOUT");
			else
				for (int i = 0; i < SD_ERR_RSVD; i++)
					if (lastError & (1 << (i + 16)))
						Log::info(errIrpts[i]);
#else
			Log::warn("Error issuing %s%u", (lastCmd & IS_APP_CMD)? "ACMD" : "CMD", lastCmd & 0xff);

			const char *err_msg;
			switch (lastError) {
				case EINVAL:    err_msg = "INVAL";   break;
				case ETIMEDOUT: err_msg = "TIMEOUT"; break;
				case EILSEQ:    err_msg = "ILSEQ";   break;
				case ENOTSUP:   err_msg = "NOTSUP";  break;
				default:        err_msg = "UNKNOWN"; break;
			}

			Log::info("%s", err_msg);
#endif
		} else
			Log::info("command completed successfully");
#endif

		return lastCmdSuccess;
	}

	int EMMCDevice::cardReset() {
#ifndef USE_SDHOST

#ifdef EMMC_DEBUG2
		Log::info("Resetting controller");
#endif

		uint32_t control1 = read32(EMMC_CONTROL1);
		control1 |= (1 << 24);
		// Disable clock
		control1 &= ~(1 << 2);
		control1 &= ~(1 << 0);
		write32(EMMC_CONTROL1, control1);
		if (timeoutWait(EMMC_CONTROL1, 7 << 24, 0, 1000000) < 0) {
			Log::error("Controller did not reset properly");
			return -1;
		}
#ifdef EMMC_DEBUG2
		Log::info("control0: %08x, control1: %08x, control2: %08x",
			read32(EMMC_CONTROL0), read32(EMMC_CONTROL1), read32(EMMC_CONTROL2));
#endif

#if RASPPI >= 4
		// Enable SD Bus Power VDD1 at 3.3V
		uint32_t control0 = read32(EMMC_CONTROL0);
		control0 |= 0x0F << 8;
		write32(EMMC_CONTROL0, control0);
		usDelay(2000);
#endif

		// Check for a valid card
#ifdef EMMC_DEBUG2
		Log::info("checking for an inserted card");
#endif
		timeoutWait(EMMC_STATUS, 1 << 16, 1, 500000);
		uint32_t status_reg = read32(EMMC_STATUS);
		if ((status_reg & (1 << 16)) == 0) {
			Log::warn("no card inserted");
			return -1;
		}
#ifdef EMMC_DEBUG2
		Log::info("status: %08x", status_reg);
#endif

		// Clear control2
		write32(EMMC_CONTROL2, 0);

		// Get the base clock rate
		uint32_t base_clock = getBaseClock();
		if (base_clock == 0) {
			Log::warn("assuming clock rate to be 100MHz");
			base_clock = 100000000;
		}

		// Set clock rate to something slow
#ifdef EMMC_DEBUG2
		Log::info("setting clock rate");
#endif
		control1 = read32(EMMC_CONTROL1);
		control1 |= 1;			// enable clock

		// Set to identification frequency (400 kHz)
		uint32_t f_id = getClockDivider(base_clock, SD_CLOCK_ID);
		if (f_id == SD_GET_CLOCK_DIVIDER_FAIL) {
			Log::info("unable to get a valid clock divider for ID frequency");
			return -1;
		}
		control1 |= f_id;

		// was not masked out and or'd with (7 << 16) in original driver
		control1 &= ~(0xF << 16);
		control1 |= (11 << 16);		// data timeout = TMCLK * 2^24

		write32(EMMC_CONTROL1, control1);

		if (timeoutWait(EMMC_CONTROL1, 2, 1, 1000000) < 0) {
			Log::error("Clock did not stabilise within 1 second");
			return -1;
		}
#ifdef EMMC_DEBUG2
		Log::info("control0: %08x, control1: %08x", 
					read32(EMMC_CONTROL0), read32(EMMC_CONTROL1));
#endif

		// Enable the SD clock
#ifdef EMMC_DEBUG2
		Log::info("enabling SD clock");
#endif
		usDelay(2000);
		control1 = read32(EMMC_CONTROL1);
		control1 |= 4;
		write32(EMMC_CONTROL1, control1);
		usDelay(2000);

		// Mask off sending interrupts to the ARM
		write32(EMMC_IRPT_EN, 0);
		// Reset interrupts
		write32(EMMC_INTERRUPT, 0xffffffff);
		// Have all interrupts sent to the INTERRUPT register
		uint32_t irpt_mask = 0xffffffff & (~SD_CARD_INTERRUPT);
#ifdef SD_CARD_INTERRUPTS
		irpt_mask |= SD_CARD_INTERRUPT;
#endif
		write32(EMMC_IRPT_MASK, irpt_mask);

		usDelay(2000);

#else	// #ifndef USE_SDHOST

		// Set clock rate to something slow
#ifdef EMMC_DEBUG2
		Log::info("setting clock rate");
#endif
		host.setClock(SD_CLOCK_ID);

#endif	// #ifndef USE_SDHOST

		// >> Prepare the device structure
		deviceID[0] = 0;
		deviceID[1] = 0;
		deviceID[2] = 0;
		deviceID[3] = 0;

		cardSupportsSDHC = 0;
		cardSupportsHS = 0;
		cardSupports18V = 0;
		cardOCR = 0;
		cardRCA = CARD_RCA_INVALID;
#ifndef USE_SDHOST
		lastInterrupt = 0;
#endif
		lastError = 0;

		failedVoltageSwitch = 0;

		lastCmdReg = 0;
		lastCmd = 0;
		lastCmdSuccess = 0;
		lastR0 = 0;
		lastR1 = 0;
		lastR2 = 0;
		lastR3 = 0;

		buf = 0;
		blocksToTransfer = 0;
		blockSize = 0;
#ifndef USE_SDHOST
		cardRemoval = 0;
		baseClock = 0;
#endif
		// << Prepare the device structure
		
#ifndef USE_SDHOST
		baseClock = base_clock;
#endif

		// Send CMD0 to the card (reset to idle state)
		if (!issueCommand(GO_IDLE_STATE, 0)) {
			Log::error("no CMD0 response");
			return -1;
		}

#ifndef USE_EMBEDDED_MMC_CM4

		// Send CMD8 to the card
		// Voltage supplied = 0x1 = 2.7-3.6V (standard)
		// Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA
#ifdef EMMC_DEBUG2
		Log::info("Note a timeout error on the following command (CMD8) is normal and expected if the SD card version is less than 2.0");
#endif
		issueCommand(SEND_IF_COND, 0x1aa);
		int v2_later = 0;
		if (TIMEOUT) {
			v2_later = 0;
#ifndef USE_SDHOST
		} else if (CMD_TIMEOUT) {
			if (!resetCmd())
				return -1;
			write32(EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
			v2_later = 0;
#endif
		} else if (FAIL) {
			Log::error("failure sending CMD8");
			return -1;
		} else {
			if ((lastR0 & 0xfff) != 0x1aa) {
				Log::error("unusable card");
#ifdef EMMC_DEBUG
				Log::info("CMD8 response %08x", lastR0);
#endif
				return -1;
			} else
				v2_later = 1;
		}

		// Here we are supposed to check the response to CMD5 (HCSS 3.6)
		// It only returns if the card is a SDIO card
#ifdef EMMC_DEBUG2
		Log::info("Note that a timeout error on the following command (CMD5) is normal and expected if the card is not a SDIO card.");
#endif
		issueCommand(IO_SET_OP_COND, 0, 10000);
		if (!TIMEOUT) {
#ifndef USE_SDHOST
			if (CMD_TIMEOUT) {
				if (!resetCmd())
					return -1;
				write32(EMMC_INTERRUPT, SD_ERR_MASK_CMD_TIMEOUT);
			} else
#endif
			{
				Log::error("SDIO card detected - not currently supported");
#ifdef EMMC_DEBUG2
				Log::info("CMD5 returned %08x", lastR0);
#endif
				return -1;
			}
		}

#else
		int v2_later = 1;
#endif	// #ifndef USE_EMBEDDED_MMC_CM4

		// Call an inquiry ACMD41 (voltage window = 0) to get the OCR
#ifdef EMMC_DEBUG2
		Log::info("sending inquiry ACMD41");
#endif
#ifdef USE_EMBEDDED_MMC_CM4
		if (!issueCommand(SEND_OP_COND, 0)) {
#else
		if (!issueCommand(ACMD(41), 0)) {
#endif
			Log::error("Inquiry ACMD41 failed");
			return -1;
		}
#ifdef EMMC_DEBUG2
		Log::info("inquiry ACMD41 returned %08x", lastR0);
#endif

		// Call initialization ACMD41
		int card_is_busy = 1;
		while (card_is_busy) {
			uint32_t v2_flags = 0;
			if (v2_later) {
				// Set SDHC support
				v2_flags |= (1 << 30);

				// Set 1.8v support
#ifdef SD_1_8V_SUPPORT
				if (!failedVoltageSwitch)
					v2_flags |= (1 << 24);
#endif
#ifdef SDXC_MAXIMUM_PERFORMANCE
				// Enable SDXC maximum performance
				v2_flags |= (1 << 28);
#endif
			}

#ifdef USE_EMBEDDED_MMC_CM4
			if (!issueCommand(SEND_OP_COND, 0x00ff8000 | v2_flags)) {
#else
			if (!issueCommand(ACMD(41), 0x00ff8000 | v2_flags)) {
#endif
				Log::error("Error issuing ACMD41");
				return -1;
			}

			if ((lastR0 >> 31) & 1) {
				// Initialization is complete
				cardOCR = (lastR0 >> 8) & 0xffff;
				cardSupportsSDHC = (lastR0 >> 30) & 0x1;
#ifdef SD_1_8V_SUPPORT
				if (!failedVoltageSwitch)
					cardSupports18V = (lastR0 >> 24) & 0x1;
#endif

				card_is_busy = 0;
			} else {
				// Card is still busy
#ifdef EMMC_DEBUG2
				Log::info("Card is busy, retrying");
#endif
				usDelay(500000);
			}
		}

#ifdef EMMC_DEBUG2
		Log::info("card identified: OCR: %04x, 1.8v support: %d, SDHC support: %d", cardOCR, cardSupports18V, cardSupportsSDHC);
#endif

#ifndef USE_EMBEDDED_MMC_CM4
		// At this point, we know the card is definitely an SD card, so will definitely
		//  support SDR12 mode which runs at 25 MHz
#ifndef USE_SDHOST
		switchClockRate(base_clock, SD_CLOCK_NORMAL);
#else
		host.setClock(SD_CLOCK_NORMAL);
#endif

		// A small wait before the voltage switch
		usDelay(5000);
#endif

#if !defined (USE_SDHOST) && !defined (USE_EMBEDDED_MMC_CM4)

		// Switch to 1.8V mode if possible
		if (cardSupports18V) {
#ifdef EMMC_DEBUG2
			Log::info("switching to 1.8V mode");
#endif
			// As per HCSS 3.6.1

			// Send VOLTAGE_SWITCH
			if (!issueCommand(VOLTAGE_SWITCH, 0)) {
#ifdef EMMC_DEBUG
				Log::info("error issuing VOLTAGE_SWITCH");
#endif
				failedVoltageSwitch = 1;
				powerOff();
				return cardReset();
			}

			// Disable SD clock
			control1 = read32(EMMC_CONTROL1);
			control1 &= ~(1 << 2);
			write32(EMMC_CONTROL1, control1);

			// Check DAT[3:0]
			status_reg = read32(EMMC_STATUS);
			uint32_t dat30 = (status_reg >> 20) & 0xf;
			if (dat30 != 0) {
#ifdef EMMC_DEBUG
				Log::info("DAT[3:0] did not settle to 0");
#endif
				failedVoltageSwitch = 1;
				powerOff();
				return cardReset();
			}

			// Set 1.8V signal enable to 1
			uint32_t control0 = read32(EMMC_CONTROL0);
			control0 |= (1 << 8);
			write32(EMMC_CONTROL0, control0);

			// Wait 5 ms
			usDelay(5000);

			// Check the 1.8V signal enable is set
			control0 = read32(EMMC_CONTROL0);
			if (((control0 >> 8) & 1) == 0) {
#ifdef EMMC_DEBUG
				Log::info("controller did not keep 1.8V signal enable high");
#endif
				failedVoltageSwitch = 1;
				powerOff();
				return cardReset();
			}

			// Re-enable the SD clock
			control1 = read32(EMMC_CONTROL1);
			control1 |= (1 << 2);
			write32(EMMC_CONTROL1, control1);

			usDelay(10000);

			// Check DAT[3:0]
			status_reg = read32(EMMC_STATUS);
			dat30 = (status_reg >> 20) & 0xf;
			if (dat30 != 0xf) {
#ifdef EMMC_DEBUG
				Log::info("DAT[3:0] did not settle to 1111b (%01x)", dat30);
#endif
				failedVoltageSwitch = 1;
				powerOff();
				return cardReset();
			}

#ifdef EMMC_DEBUG2
			Log::info("voltage switch complete");
#endif
		}

#endif	// #if !defined (USE_SDHOST) && !defined (USE_EMBEDDED_MMC_CM4)

		// Send CMD2 to get the cards CID
		if (!issueCommand(ALL_SEND_CID, 0)) {
			Log::error("error sending ALL_SEND_CID");
			return -1;
		}
		deviceID[0] = lastR0;
		deviceID[1] = lastR1;
		deviceID[2] = lastR2;
		deviceID[3] = lastR3;
#ifdef EMMC_DEBUG2
		Log::info("Card CID: %08x%08x%08x%08x", deviceID[3], deviceID[2], deviceID[1], deviceID[0]);
#endif

		// Send CMD3 to enter the data state
		if (!issueCommand(SEND_RELATIVE_ADDR, 0)) {
			Log::error("error sending SEND_RELATIVE_ADDR");
			return -1;
		}

		uint32_t cmd3_resp = lastR0;
#ifdef EMMC_DEBUG2
		Log::info("CMD3 response: %08x", cmd3_resp);
#endif

		cardRCA = (cmd3_resp >> 16) & 0xffff;
		uint32_t error = (cmd3_resp >> 13) & 0x1;
		uint32_t status = (cmd3_resp >> 9) & 0xf;
		uint32_t ready = (cmd3_resp >> 8) & 0x1;

		if ((cmd3_resp >> 15) & 0x1) {
			Log::error("CRC error");
			return -1;
		}

		if ((cmd3_resp >> 14) & 0x1) {
			Log::error("illegal command");
			return -1;
		}

		if (error) {
			Log::error("generic error");
			return -1;
		}

		if (!ready) {
			Log::error("not ready for data");
			return -1;
		}

#ifdef EMMC_DEBUG2
		Log::info("RCA: %04x", cardRCA);
#endif

		// Now select the card (toggles it to transfer state)
		if (!issueCommand(SELECT_CARD, cardRCA << 16)) {
			Log::info("error sending CMD7");
			return -1;
		}

		uint32_t cmd7_resp = lastR0;
		status = (cmd7_resp >> 9) & 0xf;

		if ((status != 3) && (status != 4)) {
			Log::error("Invalid status (%d)", status);
			return -1;
		}

		// If not an SDHC card, ensure BLOCKLEN is 512 bytes
		if (!cardSupportsSDHC && !issueCommand(SET_BLOCKLEN, SD_BLOCK_SIZE)) {
			Log::error("Error sending SET_BLOCKLEN");
			return -1;
		}

#ifndef USE_SDHOST
		uint32_t controller_block_size = read32(EMMC_BLKSIZECNT);
		controller_block_size &= (~0xfff);
		controller_block_size |= 0x200;
		write32(EMMC_BLKSIZECNT, controller_block_size);
#endif

#ifndef USE_EMBEDDED_MMC_CM4
		// Get the card's SCR register
		buf = &sdConfig->scr[0];
		blockSize = 8;
		blocksToTransfer = 1;
		issueCommand(SEND_SCR, 0, 1000000);
		blockSize = SD_BLOCK_SIZE;
		if (FAIL) {
#ifdef EMMC_DEBUG2
			Log::error("Error sending SEND_SCR");
#endif
			return -2;
		}

		// Determine card version
		// Note that the SCR is big-endian
		uint32_t scr0 = __builtin_bswap32(sdConfig->scr[0]);
		sdConfig->sdVersion = SD_VER_UNKNOWN;
		uint32_t sd_spec  = (scr0 >> (56 - 32)) & 0xf;
		uint32_t sd_spec3 = (scr0 >> (47 - 32)) & 0x1;
		uint32_t sd_spec4 = (scr0 >> (42 - 32)) & 0x1;
		sdConfig->sdBusWidths = (scr0 >> (48 - 32)) & 0xf;
		if (sd_spec == 0) {
			sdConfig->sdVersion = SD_VER_1;
		} else if (sd_spec == 1) {
			sdConfig->sdVersion = SD_VER_1_1;
		} else if (sd_spec == 2) {
			if (sd_spec3 == 0) {
				sdConfig->sdVersion = SD_VER_2;
			} else if (sd_spec3 == 1) {
				if (sd_spec4 == 0)
					sdConfig->sdVersion = SD_VER_3;
				else if (sd_spec4 == 1)
					sdConfig->sdVersion = SD_VER_4;
			}
		}
#ifdef EMMC_DEBUG2
		Log::info("SCR[0]: %08x, SCR[1]: %08x", sdConfig->scr[0], sdConfig->scr[1]);;
		Log::info("SCR: %08x%08x", be2le32(sdConfig->scr[0]), be2le32(sdConfig->scr[1]));
		Log::info("SCR: version %s, bus_widths %01x", sdVersions[sdConfig->sdVersion], sdConfig->sdBusWidths);
#endif

#ifdef SD_HIGH_SPEED
		// If card supports CMD6, read switch information from card
		if (sdConfig->sdVersion >= SD_VER_1_1) {
			// 512 bit response
			uint8_t cmd6_resp[64];
			buf = &cmd6_resp[0];
			blockSize = 64;

			// CMD6 Mode 0: Check Function (Group 1, Access Mode)
			if (!issueCommand(SWITCH_FUNC, 0x00fffff0, 100000)) {
				Log::error("Error sending SWITCH_FUNC (Mode 0)");
			} else {
				// Check Group 1, Function 1 (High Speed/SDR25)
				cardSupportsHS = (cmd6_resp[13] >> 1) & 0x1;

				// Attempt switch if supported
				if (cardSupportsHS) {
#ifdef EMMC_DEBUG2
					Log::info("Switching to %s mode", cardSupports18V? "SDR25" : "High Speed");
#endif

					// CMD6 Mode 1: Set Function (Group 1, Access Mode = High Speed/SDR25)
					if (!issueCommand(SWITCH_FUNC, 0x80fffff1, 100000)) {
						Log::error("Switch to %s mode failed", cardSupports18V? "SDR25" : "High Speed");
					} else {
						// Success; switch clock to 50MHz
#ifndef USE_SDHOST
						switchClockRate(base_clock, SD_CLOCK_HIGH);
#else
						host.setClock(SD_CLOCK_HIGH);
#endif
#ifdef EMMC_DEBUG2
						Log::info("Switch to 50MHz clock complete");
#endif
					}
				}
			}

			// Restore block size
			blockSize = SD_BLOCK_SIZE;
		}
#endif

#else
		sdConfig->sdVersion = SD_VER_4;
		sdConfig->sdBusWidths = 8;
		blockSize = SD_BLOCK_SIZE;
#endif	// #ifndef USE_EMBEDDED_MMC_CM4

#ifndef USE_EMBEDDED_MMC_CM4

		if (sdConfig->sdBusWidths & 4) {
			// Set 4-bit transfer mode (ACMD6)
			// See HCSS 3.4 for the algorithm
#ifdef SD_4BIT_DATA
#ifdef EMMC_DEBUG2
			Log::info("Switching to 4-bit data mode");
#endif

#ifndef USE_SDHOST
			// Disable card interrupt in host
			uint32_t old_irpt_mask = read32(EMMC_IRPT_MASK);
			uint32_t new_iprt_mask = old_irpt_mask & ~(1 << 8);
			write32(EMMC_IRPT_MASK, new_iprt_mask);
#endif

			// Send ACMD6 to change the card's bit mode
			if (!issueCommand(SET_BUS_WIDTH, 2)) {
				Log::error("Switch to 4-bit data mode failed");
			} else {
#ifndef USE_SDHOST
				// Change bit mode for Host
				uint32_t control0 = read32(EMMC_CONTROL0);
				control0 |= 0x2;
				write32(EMMC_CONTROL0, control0);

				// Re-enable card interrupt in host
				write32(EMMC_IRPT_MASK, old_irpt_mask);
#else
				// Change bit mode for Host
				host.setBusWidth(4);
#endif

#ifdef EMMC_DEBUG2
				Log::info("switch to 4-bit complete");
#endif
			}
#endif
		}

		Log::info("Found a valid version %s SD card", sdVersions[sdConfig->sdVersion]);

#else	// #ifndef USE_EMBEDDED_MMC_CM4

		if (sdConfig->sdBusWidths & 8) {
			// Set 8-bit transfer mode (CMD6)
#ifdef EMMC_DEBUG2
			Log::info("Switching to 8-bit data mode");
#endif

			// Disable card interrupt in host
			const uint32_t old_irpt_mask = read32(EMMC_IRPT_MASK);
			const uint32_t new_iprt_mask = old_irpt_mask & ~(1 << 8);
			write32(EMMC_IRPT_MASK, new_iprt_mask);

			// Send CMD6 to change the card's bit mode
			if (!issueCommand(SWITCH_FUNC, 0x3B70200)) {
				Log::error("Switch to 8-bit data mode failed");
			} else {
				// Change bit mode for Host
				uint32_t control0 = read32(EMMC_CONTROL0);
				control0 |= 1 << 5;
				write32(EMMC_CONTROL0, control0);

				// Re-enable card interrupt in host
				write32(EMMC_IRPT_MASK, old_irpt_mask);

#ifdef EMMC_DEBUG2
				Log::info("switch to 8-bit complete");
#endif
			}
		}

		switchClockRate(base_clock, SD_CLOCK_NORMAL);
		usDelay(5000);

		Log::info("Found a valid eMMC chip");

#endif	// #ifndef USE_EMBEDDED_MMC_CM4

#ifdef EMMC_DEBUG2
		Log::info("setup successful (status %d)", status);
#endif

#ifndef USE_SDHOST
		// Reset interrupt register
		write32(EMMC_INTERRUPT, 0xffffffff);
#endif

		return 0;
	}

	int EMMCDevice::cardInit() {
#ifndef USE_SDHOST
		if (!powerOn()) {
			Log::error("BCM2708 controller did not power on successfully");
			return -1;
		}
#endif

		// Check the sanity of the sdCommands and sdACommands structures
		static_assert(sizeof(sdCommands)  == (64 * sizeof(uint32_t)));
		static_assert(sizeof(sdACommands) == (64 * sizeof(uint32_t)));

#ifndef USE_SDHOST

		// Read the controller version
		const uint32_t ver = read32(EMMC_SLOTISR_VER);
		const uint32_t sd_version = (ver >> 16) & 0xff;
#ifdef EMMC_DEBUG2
		const uint32_t vendor = ver >> 24;
		const uint32_t slot_status = ver & 0xff;
		Log::info("Vendor %x, SD version %x, slot status %x", vendor, sd_version, slot_status);
#endif
		hciVersion = sd_version;
		if (hciVersion < 2) {
#ifdef EMMC_ALLOW_OLD_SDHCI
			Log::warn("Old SDHCI version detected");
#else
			Log::error("Only SDHCI versions >= 3.0 are supported");
			return -1;
#endif
		}

#endif	// #ifndef USE_SDHOST

		// The SEND_SCR command may fail with a DATA_TIMEOUT on the Raspberry Pi 4
		// for unknown reason. As a workaround the whole card reset is retried.
		int ret;
		for (unsigned nTries = 3; nTries > 0; nTries--) {
			ret = cardReset();
			if (ret != -2)
				break;
			Log::warn("Card reset failed. Retrying.");
		}

		return ret;
	}

	int EMMCDevice::ensureDataMode() {
		if (cardRCA == CARD_RCA_INVALID) {
			// Try again to initialise the card
			int ret = cardReset();
			if (ret != 0)
				return ret;
		}

#ifdef EMMC_DEBUG2
		Log::info("ensureDataMode() obtaining status register for card_rca %08x: ", cardRCA);
#endif

		if (!issueCommand(SEND_STATUS, cardRCA << 16)) {
			Log::warn("ensureDataMode() error sending CMD13");
			cardRCA = CARD_RCA_INVALID;
			return -1;
		}

		uint32_t status = lastR0;
		uint32_t cur_state = (status >> 9) & 0xf;
#ifdef EMMC_DEBUG2
		Log::info("status %d", cur_state);
#endif
		if (cur_state == 3) {
			// Currently in the stand-by state - select it
			if (!issueCommand(SELECT_CARD, cardRCA << 16)) {
				Log::warn("ensureDataMode() no response from CMD17");
				cardRCA = CARD_RCA_INVALID;
				return -1;
			}
		} else if (cur_state == 5) {
			// In the data transfer state - cancel the transmission
			if (!issueCommand(STOP_TRANSMISSION, 0)) {
				Log::warn("ensureDataMode() no response from CMD12");
				cardRCA = CARD_RCA_INVALID;
				return -1;
			}

#ifndef USE_SDHOST
			// Reset the data circuit
			resetDat();
#endif
		} else if (cur_state != 4) {
			// Not in the transfer state - re-initialise
			int ret = cardReset();
			if (ret != 0)
				return ret;
		}

		// Check again that we're now in the correct mode
		if (cur_state != 4) {
#ifdef EMMC_DEBUG2
			Log::info("ensureDataMode() rechecking status: ");
#endif
			if (!issueCommand(SEND_STATUS, cardRCA << 16)) {
				Log::warn("ensureDataMode() no response from CMD13");
				cardRCA = CARD_RCA_INVALID;

				return -1;
			}
			status = lastR0;
			cur_state = (status >> 9) & 0xf;
#ifdef EMMC_DEBUG2
			Log::info("status %d", cur_state);
#endif

			if (cur_state != 4) {
				Log::warn("unable to initialise SD card to data mode (state %d)", cur_state);
				cardRCA = CARD_RCA_INVALID;
				return -1;
			}
		}

		return 0;
	}

	int EMMCDevice::doDataCommand(int is_write, uint8_t *buffer_, size_t buf_size, uint32_t block) {
		// PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
		if (!cardSupportsSDHC)
			block *= SD_BLOCK_SIZE;

		// This is as per HCSS 3.7.2.1
		if (buf_size < blockSize) {
			Log::warn("doDataCommand() called with buffer size (%d) less than block size (%d)", buf_size, blockSize);
			return -1;
		}

		blocksToTransfer = buf_size / blockSize;
		if (buf_size % blockSize) {
			Log::warn("doDataCommand() called with buffer size (%d) not an exact multiple of block size (%d)", buf_size, blockSize);
			return -1;
		}

		buf = buffer_;

		// Decide on the command to use
		int command;
		if (is_write) {
			if (blocksToTransfer > 1)
				command = WRITE_MULTIPLE_BLOCK;
			else
				command = WRITE_BLOCK;
		} else if (blocksToTransfer > 1)
			command = READ_MULTIPLE_BLOCK;
		else
			command = READ_SINGLE_BLOCK;

		int retry_count = 0;
		constexpr int max_retries = 3;
		while (retry_count < max_retries) {
			if (issueCommand(command, block, 5000000)) {
				break;
			} else {
				Log::warn("error sending CMD%d", command);
				Log::info("error = %08x", lastError);

				if (++retry_count < max_retries)
					Log::info("Retrying");
				else
					Log::info("Giving up");
			}
		}

		if (retry_count == max_retries) {
			cardRCA = CARD_RCA_INVALID;
			return -1;
		}

		return 0;
	}

	size_t EMMCDevice::doRead(uint8_t *buffer_, size_t buf_size, uint32_t block) {
		// Check the status of the card
		if (ensureDataMode() != 0)
			return -1;

#ifdef EMMC_DEBUG2
		Log::info("Reading from block %u", block);
#endif

		if (doDataCommand(0, buffer_, buf_size, block) < 0)
			return -1;

#ifdef EMMC_DEBUG2
		Log::info("Data read successful");
#endif

		return buf_size;
	}

	size_t EMMCDevice::doWrite(uint8_t *buffer_, size_t buf_size, uint32_t block) {
		// Check the status of the card
		if (ensureDataMode() != 0)
			return -1;

#ifdef EMMC_DEBUG2
		Log::info("Writing to block %u", block);
#endif

		if (doDataCommand(1, buffer_, buf_size, block) < 0)
			return -1;

#ifdef EMMC_DEBUG2
		Log::info("Data write successful");
#endif

		return buf_size;
	}

#ifndef USE_SDHOST
	int EMMCDevice::timeoutWait(unsigned reg, unsigned mask, int value, unsigned usec) {
		unsigned start_ticks = Timers::getClockTicks();
		unsigned timeout_ticks = usec * (Timers::CLOCKHZ / 1'000'000);

		while ((read32(reg) & mask)? !value : value) {
			if (Timers::getClockTicks() - start_ticks >= timeout_ticks)
				return -1;
#ifdef NO_BUSY_WAIT
			Scheduler::get()->yield();
#endif
		}

		return 0;
	}
#endif

	void EMMCDevice::usDelay(unsigned usec) {
		Timers::waitMicroseconds(usec);
	}

	const uint32_t *EMMCDevice::getID() {
		return deviceID;
	}
}
