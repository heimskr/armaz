#pragma once

namespace Armaz::Kernel {
	void __attribute__((noreturn)) panic(const char *fmt, ...);
	void __attribute__((noreturn)) perish();
}
