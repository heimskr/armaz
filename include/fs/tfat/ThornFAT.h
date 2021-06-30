#pragma once

// ThornFAT: a primitive filesystem that shares some concepts with the FAT family of filesystems.

#include <cstdint>
#include <ctime>
#include <string>
#include <string.h>
#include <vector>

#include "fs/FS.h"

namespace Armaz::ThornFAT {
	using block_t = int32_t;
	using Armaz::FS::fd_t;

	constexpr size_t THORNFAT_PATH_MAX = 255;
	constexpr size_t FD_MAX = 128; // ???

	constexpr size_t PATHC_MAX = 1024;
	constexpr size_t FDC_MAX   = 1024;

	constexpr block_t UNUSABLE = -1;
	constexpr block_t FINAL    = -2;

	struct Superblock {
		uint32_t magic;
		size_t blockCount;
		uint32_t fatBlocks;
		uint32_t blockSize;
		/** The block containing the root directory. */
		block_t startBlock;
		operator std::string() const;
	} __attribute__((packed));

	struct Times {
		long created  = 0;
		long modified = 0;
		long accessed = 0;
		Times() = default;
		Times(long created_, long modified_, long accessed_):
			created(created_), modified(modified_), accessed(accessed_) {}
	};

	union Filename {
		char str[THORNFAT_PATH_MAX + 1] = {0};
		uint64_t longs[sizeof(str) / sizeof(uint64_t)];
	};

	enum class FileType: int {File, Directory};

	struct DirEntry {
		Filename name = {{0}};
		Times times;
		/** Length of the directory entry in bytes. For directories, 0 -> free. */
		size_t length = 0;
		/** 0 if free, -1 if invalid. */
		block_t startBlock = -1;
		FileType type = FileType::File;
		mode_t modes = 0;
		uid_t uid;
		gid_t gid;
		char padding[8] = {0}; // update if THORNFAT_PATH_MAX changes so that sizeof(DirEntry) is a multiple of 64

		DirEntry() = default;
		DirEntry(const Times &, size_t, FileType);
		DirEntry(const DirEntry &);
		DirEntry & operator=(const DirEntry &);
		bool isFile() const { return type == FileType::File; }
		bool isDirectory() const { return type == FileType::Directory; }
		void reset();
		operator std::string() const;
		void print() const;
	};

	static_assert(sizeof(DirEntry) % 64 == 0);

	constexpr uint32_t MAGIC = 0xfa91283e;
	constexpr int NEWFILE_SKIP_MAX = 4;
	constexpr int OVERFLOW_MAX = 32;
	constexpr size_t MINBLOCKS = 3;

	class ThornFATDriver: public FS::Driver {
		public:
			Superblock superblock;
			ssize_t blocksFree = -1;
			DirEntry root;
			size_t writeOffset = 0;
			/** Ugly hack to avoid allocating memory on the heap because I'm too lazy to deal with freeing it. */
			DirEntry overflow[OVERFLOW_MAX];
			size_t overflowIndex = 0;

			int writeSuperblock(const Superblock &);
			int readSuperblock(Superblock &);
			// void error(const std::string &);

			/** Attempts to find a file within the filesystem. 
			 *  Note: can allocate new memory in *out or in **outptr.
			 *  @param fd         An optional file descriptor. If negative, it won't be used to find a file.
			 *  @param path       An optional string representing a pathname. If nullptr, it won't be used to find a
			 *                    file.
			 *  @param out        A pointer that will be set to a copy of a matching file or directory. Unused if outptr
			 *                    isn't nullptr.
			 *  @param offset     A pointer that will be set to the directory entry's offset if a match was found.
			 *  @param get_parent Whether to return the directory containing the match instead of the match itself.
			 *  @param last_name  A pointer to a string that will be set to the last path component if get_parent is
			 *                    true.
			 *  @return Returns 0 if the operation was successful or a negative error code otherwise. */
			int find(fd_t, const char *, DirEntry *out = nullptr, off_t * = nullptr, bool get_parent = false,
			         std::string *last_name = nullptr);

			/** Removes a chain of blocks from the file allocation table. 
			 *  Returns the number of blocks that were freed. */
			size_t forget(block_t start);

			/** Returns the length of a chain of blocks. Returns the length of the chain (including the first block). */
			size_t chainLength(block_t start);

			/** Writes a directory entry at a given offset. 
			 *  Returns 0 if the operation was successful or a negative error code otherwise. */
			int writeEntry(const DirEntry &, off_t);

			/** Gets the root directory of a disk image. Takes an optional pointer that will be filled with the root
			 *  directory's offset. Returns a pointer to a struct representing the partition's root directory. */
			DirEntry & getRoot(off_t * = nullptr);

			/** Reads all the directory entries in a given directory and stores them in a vector.
			 *  Note: can allocate new memory in *entries and *offsets.
			 *  @param dir         A reference to a directory entry struct.
			 *  @param entries     A vector that will be filled with directory entries.
			 *  @param offsets     A pointer that will be set to an array of raw offsets. Can be nullptr.
			 *  @param first_index An optional pointer that will be set to the index of the first entry other than ".."
			 *                     or ".".
			 *  @return Returns 0 if the operation succeeded or a negative error code otherwise. */
			int readDir(const DirEntry &dir, std::vector<DirEntry> &entries, std::vector<off_t> *offsets = nullptr,
			            int *first_index = nullptr);

			/** Reads the raw bytes for a given directory entry and stores them in an array.
			 *  @param file  A reference to a directory entry struct.
			 *  @param out   A reference to a vector of bytes that will be filled with the read bytes.
			 *  @param count An optional pointer that will be set to the number of bytes read.
			 *  @return Returns 0 if the operation succeeded or a negative error code otherwise. */
			int readFile(const DirEntry &file, std::vector<uint8_t> &out, size_t *count = nullptr);

			/** Creates a new file.
			 *  @param path              The path of the new file to create.
			 *  @param length            The length (in bytes) of the file's content.
			 *  @param type              The type of the new entry (whether it's a file or a directory).
			 *  @param times             An optional pointer to a set of times that will be assigned to the entry. If
			 *                           nullptr, the current time (or a constant if CONSTANT_TIME is defined) will be
			 *                           used.
			 *  @param dir_out           An optional pointer to which a copy of the new entry will be written.
			 *  @param offset_out        An optional pointer to which the offset of the new directory entry will be
			 *                           written.
			 *  @param parent_dir_out    An optional pointer to which a copy of the new entry's parent directory entry
			 *                           will be written.
			 *  @param parent_offset_out An optional pointer to which the new entry's parent directory entry's offset
			 *                           will be written.
			 *  @param noalloc           A parameter indicating whether blocks shouldn't be allocated for the new file.
			 *                           This is useful if you just want to create a dummy entry (e.g., in fat_rename).
			 *  @return Returns 0 if the operation succeeded or a negative number otherwise. */
			int newFile(const char *path, size_t length, FileType type, const Times *times, DirEntry *dir_out,
			            off_t *offset_out, DirEntry *parent_dir_out, off_t *parent_offset_out, bool noalloc);

			/** Attempts to remove a file.
			 *  @param path          A file path.
			 *  @return Returns 0 if the operation succeeded or a negative error code otherwise. */
			int remove(const char *path);

			/** Attempts to resize a file or directory. If the new size is smaller than the old size, the leftover data
			 *  will be zeroed out. This modifies both the directory entry argument and the data on the disk.
			 *  @param file        A reference to a directory entry.
			 *  @param file_offset The starting offset of the file.
			 *  @param new_size    The new size of the file.
			 *  @return Returns 0 if the operation succeeded or a negative error code otherwise. */
			int resize(DirEntry &file, off_t file_offset, size_t new_size);

			/** Zeroes out the free space at the end of a file. Useful when truncating a file.
			 *  Returns 0 if the operation was successful or a negative error code otherwise. */
			int zeroOutFree(const DirEntry &file, size_t new_size);

			/** Attempts to find a free block in a file allocation table.
			 *  @return Returns the index of the first free block if any were found; -1 otherwise. */
			block_t findFreeBlock();

			/** Determines whether a directory is empty. Returns 0 if the directory isn't empty, 1 if it is or a
			 *  negative error code if an error occurred. */
			int directoryEmpty(const DirEntry &dir);

			/** Updates a DirEntry's filename by zeroing out its old filename and copying the new filename over it. */
			void updateName(DirEntry &, const char *);
			void updateName(DirEntry &, const std::string &);

			block_t readFAT(size_t block_offset);
			int writeFAT(block_t block, size_t block_offset);

			bool initFAT(size_t table_size, size_t block_size);
			bool initData(size_t table_size, size_t block_size);

			bool hasFree(const size_t);
			ssize_t countFree();

			bool checkBlock(block_t);

			bool isFree(const DirEntry &);
			bool hasStuff(const DirEntry &);
			bool isRoot(const DirEntry &);

			template <typename T>
			ssize_t writeMany(T n, size_t count, off_t offset) {
				ssize_t status;
				for (size_t i = 0; i < count; ++i) {
					status = partition->write(&n, sizeof(T), offset + i * sizeof(T));
					if (status < 0) {
						printf("[ThornFATDriver::writeMany] Writing failed: %ld\n", status);
						return status;
					}
				}
				return status;
			}

			template <typename T>
			ssize_t writeMany(T n, size_t count) {
				ssize_t status;
				for (size_t i = 0; i < count; ++i) {
					status = partition->write(&n, sizeof(T), writeOffset);
					if (status < 0) {
						printf("[ThornFATDriver::writeMany] Writing failed: %ld\n", status);
						return status;
					}
					writeOffset += sizeof(T);
				}
				return status;
			}

			template <typename T>
			ssize_t write(const T &n) {
				ssize_t status = partition->write(&n, sizeof(T), writeOffset);
				if (status < 0) {
					printf("[ThornFATDriver::write] Writing failed: %ld\n", status);
					return status;
				}
				writeOffset += sizeof(T);
				return status;
			}

			static size_t tableSize(size_t block_count, size_t block_size);

			/** Converts a standard file handle (presumably required to be > 0, maybe even > 2) to a FAT file descriptor
			 *  (can be 0). **/
			static inline fd_t transformDescriptor(fd_t descriptor) {
				return descriptor - 1;
			}

			static const char nothing[sizeof(DirEntry)];

		public:
			virtual int rename(const char *path, const char *newpath) override;
			virtual int release(const char *path) override;
			virtual int statfs(const char *, FS::DriverStats &) override;
			virtual int utimens(const char *path, const timespec &) override;
			virtual int create(const char *path, mode_t, uid_t, gid_t) override;
			virtual int write(const char *path, const char *buffer, size_t size, off_t offset) override;
			virtual int mkdir(const char *path, mode_t, uid_t, gid_t) override;
			virtual int truncate(const char *path, off_t size) override;
			virtual int ftruncate(const char *path, off_t size) override;
			virtual int rmdir(const char *path) override;
			virtual int unlink(const char *path) override;
			virtual int open(const char *path) override;
			virtual int read(const char *path, void *buffer, size_t size, off_t offset) override;
			virtual int readdir(const char *path, FS::DirFiller filler) override;
			virtual int getattr(const char *path, FS::FileStats &) override;
			virtual int getsize(const char *path, size_t &out) override;
			virtual int isdir(const char *path) override;
			virtual int isfile(const char *path) override;
			virtual int exists(const char *path) override;
			virtual bool verify() override;
			virtual void cleanup() override {}
			bool make(uint32_t block_size);

			ThornFATDriver(Partition *);
	};
}
