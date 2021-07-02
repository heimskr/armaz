#pragma once

//
// emmc.h
//
// Provides an interface to the EMMC controller and commands for interacting
// with an sd card
//
// Copyright(C) 2013 by John Cronin <jncronin@tysos.org>
//
// Modified for Circle by R. Stange
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
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

#include <stddef.h>
#include <stdint.h>

#include "storage/StorageDevice.h"

namespace Armaz {
	struct SDConfiguration {
		uint32_t scr[2];
		uint32_t sdBusWidths;
		int	sdVersion;
	};

	class EMMCDevice: public StorageDevice {
		public:
			EMMCDevice();
			virtual ~EMMCDevice();

			bool init();

			ssize_t read(void *buffer, size_t count);
			ssize_t write(const void *buffer, size_t count);

			uint64_t seek(uint64_t new_offset);

			virtual ssize_t read(void *buffer, size_t size, size_t byte_offset) override;
			virtual ssize_t write(const void *buffer, size_t bytes, size_t byte_offset) override;

			const uint32_t * getID();

			bool isReady() const { return initialized; }

		private:
			bool initialized;

#ifndef USE_SDHOST
			bool powerOn();
			void powerOff();

			uint32_t getBaseClock();
			uint32_t getClockDivider(uint32_t baseClock, uint32_t target_rate);
			bool switchClockRate(uint32_t baseClock, uint32_t target_rate);

			bool resetCmd();
			bool resetDat();
#endif

			void issueCommandInt(uint32_t cmd_reg, uint64_t argument, int timeout);
#ifndef USE_SDHOST
			void handleCardInterrupt();
			void handleInterrupts();
#endif
			bool issueCommand(uint32_t command, uint64_t argument, int timeout = 500000);

			int cardReset();
			int cardInit();

			int ensureDataMode();
			bool doDataCommand(int is_write, uint8_t *buf, size_t buf_size, uint64_t block);
			size_t doRead(uint8_t *buf, size_t buf_size, uint64_t block);
			size_t doWrite(uint8_t *buf, size_t buf_size, uint64_t block);

#ifndef USE_SDHOST
			int timeoutWait(ptrdiff_t reg, unsigned mask, int value, unsigned usec);
#endif

			void usDelay(unsigned usec);

#ifndef USE_SDHOST
#if RASPPI == 3
			GPIOPin gpio34_39[6]; // WiFi
			GPIOPin gpio48_53[6]; // SD card
#endif
#endif

			uint64_t offset;

#ifdef USE_SDHOST
			SDHOSTDevice host;
#else
			uint32_t hciVersion;
#endif

			// was: struct emmc_block_dev
			uint32_t deviceID[4];

			uint32_t cardSupportsSDHC;
			uint32_t cardSupportsHS;
			uint32_t cardSupports18V;
			uint32_t cardOCR;
			uint32_t cardRCA;
			static constexpr uint32_t CARD_RCA_INVALID = 0xffff0000;
#ifndef USE_SDHOST
			uint32_t lastInterrupt;
#endif
			uint32_t lastError;

			SDConfiguration *sdConfig;

			int failedVoltageSwitch;

			uint32_t lastCmdReg;
			uint32_t lastCmd;
			uint32_t lastCmdSuccess;
			uint32_t lastR0;
			uint32_t lastR1;
			uint32_t lastR2;
			uint32_t lastR3;

			void *buf;
			int blocksToTransfer;
			size_t blockSize;
#ifndef USE_SDHOST
			int cardRemoval;
			uint32_t baseClock;
#endif

			static const char *sdVersions[];
#ifndef USE_SDHOST
			static const char *errIrpts[];
#endif
			static const uint32_t sdCommands[];
			static const uint32_t sdACommands[];

			ssize_t readBytes(void *buffer, size_t bytes, size_t byte_offset);
			ssize_t writeBytes(const void *buffer, size_t bytes, size_t byte_offset);
	};
}
