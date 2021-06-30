#include "util.h"
#include "aarch64/Timer.h"
#include "fs/tfat/ThornFAT.h"
#include "fs/tfat/Util.h"

int debug_enable = 0;
int debug_disable = 1;
int debug_disable_method = 0;
int debug_disable_external = 0;
char indentation[81];

namespace Armaz::ThornFAT::Util {
	std::optional<std::string> pathFirst(std::string path, std::string *remainder) {
		if (path.empty()) {
			if (remainder)
				*remainder = "";
			return std::nullopt;
		}

		if (path.front() == '/')
			path = path.substr(1);

		size_t count, len = path.size();
		for (count = 0; count < len && path[count] != '/'; ++count);

		std::string out = path.substr(0, count);

		if (remainder) {
			std::string last = path.substr(count);
			*remainder = (count == path.size() || last == "/")? "" : last;
		}

		return {out};
	}

	std::optional<std::string> pathLast(const char *path) {
		if (path == nullptr)
			return std::nullopt;

		const size_t len = strlen(path);

		// If the path is empty, return an empty string.
		if (len == 0)
			return "";

		// Index of the last slash in the path that isn't the last character in the path.
		int last_slash = -1;

		// Whether the path ends with a slash.
		const bool terminal_slash = path[len - 1] == '/';

		// If the only character in the entire path is "/", return an empty string.
		if (len == 1 && terminal_slash)
			return "";

		// Starting at len - 2 instead of len - 1 will cause a slash at the end of the path to be ignored.
		for (int i = len - 2; 0 <= i; --i)
			if (path[i] == '/') {
				last_slash = i;
				break;
			}

		// The absence of any slashes (except possibly a terminal slash) is a special case.
		if (last_slash == -1) {
			// If the path doesn't contain any slashes whatsoever, just return a copy of the whole path.
			if (!terminal_slash)
				return path;

			// If there is only one slash and it's at the end of the path, return a copy of all but the last character.

			return std::string(path, len - 1);
		}

		// len - last_slash is the length of the last slash plus the basename and we want to return just the basename,
		// so we subtract 1 to remove the slash. We need to subtract 1 again if the string ends in a slash because we
		// don't want to include the terminal slash in the result.
		const size_t out_len = len - last_slash - 1 - (terminal_slash? 1 : 0);
		return {std::string(path, last_slash + 1).substr(0, out_len)};
	}

	std::optional<std::string> pathParent(const char *path) {
		if (path == NULL)
			return std::nullopt;

		const size_t len = strlen(path);

		// If the path is empty, return "/".
		if (len == 0)
			return "/";

		// Index of the last slash in the path that isn't the last character in the path.
		int last_slash = -1;
		// Whether the path ends with a slash.
		const bool terminal_slash = path[len - 1] == '/';

		// If the only character in the entire path is "/", return "/".
		if (len == 1 && terminal_slash)
			return "/";

		// Starting at len - 2 instead of len - 1 will cause a slash at the end of the path to be ignored.
		for (int i = len - 2; 0 <= i; --i)
			if (path[i] == '/') {
				last_slash = i;
				break;
			}

		// If there's no slash, return an empty string.
		if (last_slash == -1)
			return "";

		// If the only slash is at the beginning of the string, return "/".
		if (last_slash == 0)
			return "/";

		// Return a copy of the path up to the last slash.
		return std::string(path, last_slash);
	}
}

/**
 * Displays a tag and a string in the log.
 * @param  *s   A tag identifying the source of the debug message.
 * @param  *s1  An info string displayed before the directory entry.
 */
void dbg(const char *source, int line, const char *s, const char *s1) {
	if (DEBUG_ENABLED) {
		printf(LOGPAIR " %s" LOGEND, DBGLOGSET(s), s1);
		Armaz::Timers::waitMicroseconds(100'000);
	}
}

/**
 * Displays a tag and two strings in the log.
 * @param  *s   A tag identifying the source of the debug message.
 * @param  *s1  An info string.
 * @param  *s2  A second info string.
 */
void dbg2(const char *source, int line, const char *s, const char *s1, const char *s2) {
	if (DEBUG_ENABLED) {
		printf(LOGPAIR " %s " BSTR LOGEND, DBGLOGSET(s), s1, s2);
		Armaz::Timers::waitMicroseconds(100'000);
	}
}

/**
 * Displays a tag, a string and a number (in decimal) in the log.
 * @param  *s   A tag identifying the source of the debug message.
 * @param  *s1  An info string displayed before the number.
 * @param   n   A number.
 */
void dbgn(const char *source, int line, const char *s, const char *s1, int64_t n) {
	if (DEBUG_ENABLED) {
		printf(LOGPAIR " %s " BLR LOGEND, DBGLOGSET(s), s1, n);
		Armaz::Timers::waitMicroseconds(100'000);
	}
}

/**
 * Displays a tag, a string and a number (in hexadecimal) in the log.
 * @param  *s   A tag identifying the source of the debug message.
 * @param  *s1  An info string displayed before the number.
 * @param   n   A number.
 */
void dbgh(const char *source, int line, const char *s, const char *s1, int64_t n) {
	if (DEBUG_ENABLED) {
		printf(LOGPAIR " %s " IBS("0x%lx") LOGEND, DBGLOGSET(s), s1, n);
		Armaz::Timers::waitMicroseconds(100'000);
	}
}

void indent(int offset) {
	static int indent_level = -1;

	if (indent_level == -1) {
		// If indent_level is -1 at the start of this function, we need to fill indentation with spaces.
		memset(indentation, ' ', 80);
		indentation[80] = '\0';
		indent_level = 0;
	}

	int old_indent = indent_level;
	indent_level += offset * INDENT_WIDTH;
	if (indent_level < 0) {
		indent_level = 0;
	} else if (80 < indent_level)  {
		indent_level = 80;
	}

	if (-1 < old_indent) {
		indentation[old_indent] = ' ';
	}

	indentation[indent_level] = '\0';
}
