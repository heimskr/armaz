#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <memory>
#include <string>
#include <sys/stat.h>

#include "storage/Partition.h"

namespace Armaz::FS {
	constexpr size_t BLOCKSIZE = 512;

	using fd_t = uint64_t;
	using inode_t = size_t;
	using DirFiller = std::function<void(const char *, off_t)>;

	std::string simplifyPath(std::string cwd, std::string);
	std::string simplifyPath(std::string);

	struct FileInfo {
		fd_t descriptor;
		int flags; // for open and release
		FileInfo() = delete;
		FileInfo(fd_t descriptor_, int flags_): descriptor(descriptor_), flags(flags_) {}
	};

	struct FileStats {
		dev_t device = -1;
		inode_t inode = 0;
		mode_t mode = 0;
		size_t hardlinks = 0;
		uid_t uid = -1;
		gid_t gid = -1;
		dev_t rdevice = -1;
		size_t blockSize = BLOCKSIZE;
		size_t blockCount = 0;
	};

	struct DriverStats {
		/** Identifies the type of the filesystem. */
		unsigned int magic = 0;
		/** Maximum filename length. */
		size_t nameMax = 0;
		size_t optimalBlockSize = 0;
		/** Total blocks in filesystem. */
		size_t totalBlocks = 0;
		/** Total number of free blocks in the filesystem. */
		size_t freeBlocks = 0;
		/** Number of free blocks available to unprivileged users. */
		size_t availableBlocks = 0;
		/** Total number of inodes in the filesystem. */
		size_t files = 0;
		/** Number of free inodes in the filesystem. */
		size_t freeFiles = 0;
		uint64_t flags = 0;
	};

	class Driver {
		public:
			Partition *partition;
			virtual ~Driver() {}
			virtual int rename(const char *path, const char *newpath) = 0;
			virtual int release(const char *path) = 0;
			virtual int statfs(const char *, DriverStats &) = 0;
			virtual int utimens(const char *path, const timespec &) = 0;
			virtual int create(const char *path, mode_t, uid_t, gid_t) = 0;
			virtual int write(const char *path, const char *buffer, size_t size, off_t offset) = 0;
			virtual int mkdir(const char *path, mode_t, uid_t, gid_t) = 0;
			virtual int truncate(const char *path, off_t size) = 0;
			virtual int ftruncate(const char *path, off_t size) = 0;
			virtual int rmdir(const char *path) = 0;
			virtual int unlink(const char *path) = 0;
			virtual int open(const char *path) = 0;
			virtual int read(const char *path, void *buffer, size_t size, off_t offset) = 0;
			virtual int readdir(const char *path, DirFiller filler) = 0;
			virtual int getattr(const char *path, FileStats &) = 0;
			virtual int getsize(const char *path, size_t &out) = 0;
			virtual int isdir(const char *path) = 0;
			virtual int isfile(const char *path) = 0;
			/** Returns 0 if the file exists or a negative error code otherwise. */
			virtual int exists(const char *path) = 0;
			/** Does this partition contain a valid instance of the filesystem? */
			virtual bool verify() = 0;
			virtual void cleanup() = 0;

		protected:
			Driver() = delete;
			Driver(const Driver &) = delete;
			Driver(Driver &&) = delete;
			Driver(Partition *partition_): partition(partition_) {}

			Driver & operator=(const Driver &) = delete;
			Driver & operator=(Driver &&) = delete;
	};
}
