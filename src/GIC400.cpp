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

#include "GIC400.h"

namespace Armaz::GIC400 {
	static struct {
		GICD* gicd;
		GICC* gicc;
	} gic400;

	void init(void *interrupt_controller_base) {
		int number_of_interrupts = 0;

		gic400.gicd = (GICD *) ((char *) interrupt_controller_base + 0x1000);
		gic400.gicc = (GICC *) ((char *) interrupt_controller_base + 0x2000);

		// Disable the controller so we can configure it before it passes any interrupts to the CPU.
		gic400.gicd->ctl = CTL_DISABLE;

		// Get the number of interrupt lines implemented in the GIC400 controller.
		number_of_interrupts = TYPE_ITLINESNUMBER_GET(gic400.gicd->type) * 32;

		// The actual number returned by the ITLINESNUMBER is the number of registers implemented.
		// The actual number of interrupt lines available is (ITLINESNUMBER * 32).
		for (int i = 0; i < number_of_interrupts; ++i) {
			// Deal with register sets that have 32-interrupts per register.
			if ((i % 32) == 0) {
				// Disable this block of 32 interrupts, clear the pending and active flags.
				gic400.gicd->icenable[i / 32] = 0xffffffff;
				gic400.gicd->icpend[i / 32]   = 0xffffffff;
				gic400.gicd->icactive[i / 32] = 0xffffffff;
			}

			// Deal with interrupt configuration. The configuration registers are 32 bits wide and have 2-bit
			// configuration for each interrupt.
			gic400.gicd->icfg[i / 16] = gic400.gicd->icfg[i / 16] & (3 << (i % 16));
			gic400.gicd->icfg[i / 16] = gic400.gicd->icfg[i / 16] | (ICFG_LEVEL_SENSITIVE << (i % 16));

			// Deal with register sets that have one interrupt per register.
			gic400.gicd->ipriority[i] = 0xa0;
			gic400.gicd->istargets[i] = TARGET_CPU0;
		}

		gic400.gicd->ctl = CTL_ENABLE;

		gic400.gicc->pm = 0xf0;
		gic400.gicc->ctl = CTL_ENABLE;
	}
}