#include "util.h"

namespace Armaz::Util {
	std::list<std::string> splitToList(const std::string &str, const std::string &delimiter, bool condense) {
		if (str.empty())
			return std::list<std::string>();

		size_t next = str.find(delimiter);
		if (next == std::string::npos)
			return std::list<std::string> {str};

		std::list<std::string> out {};
		const size_t delimiter_length = delimiter.size();
		size_t last = 0;

		out.push_back(str.substr(0, next));

		while (next != std::string::npos) {
			last = next;
			next = str.find(delimiter, last + delimiter_length);
			std::string sub = str.substr(last + delimiter_length, next - last - delimiter_length);
			if (!sub.empty() || !condense)
				out.push_back(std::move(sub));
		}

		return out;
	}

	std::vector<std::string> splitToVector(const std::string &str, const std::string &delimiter, bool condense) {
		if (str.empty())
			return std::vector<std::string>();

		size_t next = str.find(delimiter);
		if (next == std::string::npos)
			return std::vector<std::string> {str};

		std::vector<std::string> out {};
		const size_t delimiter_length = delimiter.size();
		size_t last = 0;

		out.push_back(str.substr(0, next));

		while (next != std::string::npos) {
			last = next;
			next = str.find(delimiter, last + delimiter_length);
			std::string sub = str.substr(last + delimiter_length, next - last - delimiter_length);
			if (!sub.empty() || !condense)
				out.push_back(std::move(sub));
		}

		return out;
	}
}
