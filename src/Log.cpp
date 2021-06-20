#include "Log.h"
#include "printf.h"
#include "UART.h"

namespace Armaz::Log {
	void error(const char *format, ...) {
		UART::write("\e[2m[\e[22;31mx\e[39;2m]\e[22m ");
		va_list var;
		va_start(var, format);
		vprintf(format, var);
		va_end(var);
		UART::write('\n');
	}

	void info(const char *format, ...) {
		UART::write("\e[2m[\e[22;36mi\e[39;2m]\e[22m ");
		va_list var;
		va_start(var, format);
		vprintf(format, var);
		va_end(var);
		UART::write('\n');
	}

	void warn(const char *format, ...) {
		UART::write("\e[2m[\e[22;33m!\e[39;2m]\e[22m ");
		va_list var;
		va_start(var, format);
		vprintf(format, var);
		va_end(var);
		UART::write('\n');
	}

	void success(const char *format, ...) {
		UART::write("\e[2m[\e[22;32m🗸\e[39;2m]\e[22m ");
		va_list var;
		va_start(var, format);
		vprintf(format, var);
		va_end(var);
		UART::write('\n');
	}
}
