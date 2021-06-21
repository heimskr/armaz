#pragma once

#include <list>
#include <string>
#include <vector>
#include <stddef.h>

extern "C" void * memcpy(void *dest, const void *src, size_t n);

namespace Armaz::Util {
	// Due to clang shenanigans (or possibly due to my own failings), I can't templatify this and have to duplicate the
	// split function myself.

	std::list<std::string> splitToList(const std::string &str, const std::string &delimiter, bool condense = true);
	std::vector<std::string> splitToVector(const std::string &str, const std::string &delimiter, bool condense = true);

	template <typename T>
	inline T upalign(T number, int alignment) {
		return number + ((alignment - (number % alignment)) % alignment);
	}

	template <typename T>
	inline T downalign(T number, int alignment) {
		return number - (number % alignment);
	}

	template <typename T>
	inline T updiv(T n, T d) {
		return n / d + (n % d? 1 : 0);
	}
}
