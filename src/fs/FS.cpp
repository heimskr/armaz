#include "fs/FS.h"
#include "util.h"

#include <list>

namespace Armaz::FS {
	std::string simplifyPath(std::string cwd, std::string path) {
		if (cwd.empty())
			cwd = "/";
		
		if (path.empty())
			return cwd;
		
		if (path.front() != '/') {
			if (cwd.back() == '/')
				path = cwd + path;
			else
				path = cwd + '/' + path;
		}

		std::list<std::string> split = Util::splitToList(path, "/", true);

		auto iter = split.begin();
		while (iter != split.end()) {
			const std::string &piece = *iter;
			if (piece.empty() || piece == "." || (piece == ".." && iter == split.begin())) {
				auto next = iter;
				++next;
				if (next == split.end()) {
					split.erase(iter);
					break;
				} else {
					split.erase(iter);
					iter = next;
				}
			} else if (piece == "..") {
				auto prev = iter, next = iter;
				--prev;
				++next;
				split.erase(prev);
				split.erase(iter);
				iter = next;
			} else
				++iter;
		}

		if (split.empty())
			return "/";

		path.clear();

		for (const std::string &piece: split)
			path += "/" + piece;

		return path;
	}

	std::string simplifyPath(std::string path) {
		if (path.empty())
			return "/";

		std::list<std::string> split = Util::splitToList(path, "/", true);

		auto iter = split.begin();
		while (iter != split.end()) {
			const std::string &piece = *iter;
			if (piece.empty() || piece == "." || (piece == ".." && iter == split.begin())) {
				auto next = iter;
				++next;
				if (next == split.end()) {
					split.erase(iter);
					break;
				} else {
					split.erase(iter);
					iter = next;
				}
			} else if (piece == "..") {
				auto prev = iter, next = iter;
				--prev;
				++next;
				split.erase(prev);
				split.erase(iter);
				iter = next;
			} else
				++iter;
		}

		if (split.empty())
			return "/";

		path.clear();

		for (const std::string &piece: split)
			path += "/" + piece;

		return path;
	}
}
