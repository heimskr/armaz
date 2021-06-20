#pragma once

namespace Armaz::Log {
	void error(const char *format, ...);
	void info(const char *format, ...);
	void warn(const char *format, ...);
	void success(const char *format, ...);
}
