#include "GPT.h"
#include "printf.h"
#include "UART.h"

namespace Armaz::GPT {
	void GUID::print(bool newline) {
		printf("%x-%x-%x-%x%x-%x%x%x%x%x%x%s", timeLow, timeMid, timeHigh | (version << 12), clock[0], clock[1],
			node[0], node[1], node[2], node[3], node[4], node[5], newline? "\n" : "");
	}

	GUID::operator std::string() const {
		char str[37];
		snprintf(str, sizeof(str), "%x-%x-%x-%x%x-%x%x%x%x%x%x", timeLow, timeMid, timeHigh | (version << 12), clock[0],
			clock[1], node[0], node[1], node[2], node[3], node[4], node[5]);
		return str;
	}

	GUID::operator bool() const {
		if (timeLow == 0xffffffff && timeMid == 0xffff && timeHigh == 0xfff && version == 0xf && clock[0] == 0xff &&
			clock[1] == 0xff && node[0] == 0xff && node[1] == 0xff && node[2] == 0xff && node[3] == 0xff &&
			node[4] == 0xff && node[5] == 0xff)
			return false;
		return timeLow || timeMid || timeHigh || version || clock[0] || clock[1] || node[0] || node[1] || node[2]
			|| node[3] || node[4] || node[5];
	}

	void PartitionEntry::printName(bool newline) {
		for (unsigned i = 0; i < 36; ++i) {
			if (!name[i])
				break;
			UART::write(static_cast<char>(name[i] & 0xff));
		}
		if (newline)
			UART::write('\n');
	}

	PartitionEntry::operator std::string() const {
		char str[37];
		for (unsigned i = 0; i < 36; ++i)
			str[i] = name[i] & 0xff;
		str[36] = '\0';
		return str;
	}
}
