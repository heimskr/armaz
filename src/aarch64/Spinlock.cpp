// Credit: https://github.com/rsta2/circle

#include "assert.h"
#include "aarch64/Spinlock.h"
#include "aarch64/Synchronize.h"

// I'm sorry to make my codebase uglier with this, but VS Code has an issue where it interprets "#" within inline
// assembly strings as the start of a single-line comment that extends past the closing quote. This causes everything
// after it to be colored incorrectly and is incredibly annoying. This is a workaround I found.
#define ASM_HACK_1 "mov w2, #1\n"

namespace Armaz {
	Spinlock::Spinlock(Level level_): level(level_) {}

#ifdef ARM_ALLOW_MULTI_CORE
	bool Spinlock::enabled = false;

	Spinlock::~Spinlock() {
		assert(locked == 0);
	}

	void Spinlock::acquire() {
		if (level == Level::IRQ || level == Level::FIQ)
			enterCritical(level);

		if (enabled)
			asm volatile(
				"mov x1, %0\n"
				ASM_HACK_1
				"prfm pstl1keep, [x1]\n"
				"1: ldaxr w3, [x1]\n"
				"cbnz w3, 1b\n"
				"stxr w3, w2, [x1]\n"
				"cbnz w3, 1b\n"
			:: "r"((uintptr_t) &locked) : "x1", "x2", "x3");
	}

	void Spinlock::release() {
		if (enabled)
			asm volatile ("mov x1, %0 \n stlr wzr, [x1]" :: "r"((uintptr_t) &locked) : "x1");

		if (level == Level::IRQ || level == Level::FIQ)
			leaveCritical();
	}

	void Spinlock::enable() {
		assert(!enabled);
		enabled = true;
	}
#else
	void Spinlock::acquire() {
		if (level == Level::IRQ || level == Level::FIQ)
			enterCritical(level);
	}

	void Spinlock::release() {
		if (level == Level::IRQ || level == Level::FIQ)
			leaveCritical();
	}

	void Spinlock::enable() {}
#endif
}
