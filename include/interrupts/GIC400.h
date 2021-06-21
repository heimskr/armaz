#pragma once

/* Copyright 2013–2018 Brian Sidebotham <brian.sidebotham@gmail.com>
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

namespace Armaz::GIC400 {
	void init(void *interrupt_controller_base);

	constexpr int CTL_DISABLE = 0;
	constexpr int CTL_ENABLE  = 1;

	template <typename T>
	T TYPE_ITLINESNUMBER_GET(T x) {
		return x & 0xf;
	}

	template <typename T>
	T TYPE_LSPI_GET(T x) {
		return (x >> 11) & 0x1f;
	}

	template <typename T>
	T TYPE_SECURITY_EXTENSIONS_GET(T x) {
		return (x >> 10) & 1;
	}

	template <typename T>
	T TYPE_CPU_NUMBER_GET(T x) {
		return (x >> 5) & 7;
	}

	constexpr int TARGET_CPU0 = 1 << 0;
	constexpr int TARGET_CPU1 = 1 << 1;
	constexpr int TARGET_CPU2 = 1 << 2;
	constexpr int TARGET_CPU3 = 1 << 3;
	constexpr int TARGET_CPU4 = 1 << 4;
	constexpr int TARGET_CPU5 = 1 << 5;
	constexpr int TARGET_CPU6 = 1 << 6;
	constexpr int TARGET_CPU7 = 1 << 7;

	constexpr int ICFG_LEVEL_SENSITIVE = 0 << 1;
	constexpr int ICFG_EDGE_TRIGGERED  = 1 << 1;

	struct GICD {
		volatile unsigned int ctl;
		volatile const unsigned int type;
		volatile const unsigned int iid;
		volatile unsigned int _res0[(0x80 - 0xc) / sizeof(unsigned int)];
		volatile unsigned int igroup[(0x100 - 0x80) / sizeof(unsigned int)];
		volatile unsigned int isenable[(0x180 - 0x100) / sizeof(unsigned int)];
		volatile unsigned int icenable[(0x200 - 0x180) / sizeof(unsigned int)];
		volatile unsigned int ispend[(0x280 - 0x200) / sizeof(unsigned int)];
		volatile unsigned int icpend[(0x300 - 0x280) / sizeof(unsigned int)];
		volatile unsigned int isactive[(0x380 - 0x300) / sizeof(unsigned int)];
		volatile unsigned int icactive[(0x400 - 0x380) / sizeof(unsigned int)];
		volatile unsigned char ipriority[(0x800 - 0x400) / sizeof(unsigned char)];
		volatile unsigned char istargets[(0xc00 - 0x800) / sizeof(unsigned char)];
		volatile unsigned int icfg[(0xd00 - 0xc08) / sizeof(unsigned int)];
		volatile const unsigned int ppis;
		volatile unsigned int spis[(0xf00 - 0xd04) / sizeof(unsigned int)];
		volatile unsigned int sgi;
		volatile unsigned int _res1[(0xf10 - 0xf04) / sizeof(unsigned int)];
		volatile unsigned int cpendsgi[(0xf20 - 0xf10) / sizeof(unsigned int)];
		volatile unsigned int spendsgi[(0xf30 - 0xf20) / sizeof(unsigned int)];
		volatile unsigned int _res2[(0xfd0 - 0xf30) / sizeof(unsigned int)];
		volatile const unsigned int pid[(0xff0 - 0xfd0) / sizeof(unsigned int)];
		volatile const unsigned int cid[(0x1000 - 0xff0) / sizeof(unsigned int)];
	};

	struct GICC {
		volatile unsigned int ctl;
		volatile unsigned int pm;
		volatile unsigned int bp;
		volatile const unsigned int ia;
		volatile unsigned int eoi;
		volatile const unsigned int rp;
		volatile const unsigned int hppi;
		volatile unsigned int abp;
		volatile const unsigned int aia;
		volatile unsigned int aeoi;
		volatile const unsigned int ahppi;
		volatile unsigned int _res0[(0xd0 - 0x2c) / sizeof(unsigned int)];
		volatile unsigned int ap;
		volatile unsigned int _res1[(0xe0 - 0xd4) / sizeof(unsigned int)];
		volatile unsigned int nasp;
		volatile unsigned int _res2[(0xfc - 0xe4) / sizeof(unsigned int)];
		volatile const unsigned int iid;
		volatile unsigned int _res3[(0x1000 - 0x100) / sizeof(unsigned int)];
		volatile unsigned int dir;
	};
}
