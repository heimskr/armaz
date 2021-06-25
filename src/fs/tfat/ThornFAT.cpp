#include <cerrno>
#include <string>
#include <string.h>

#include "fs/tfat/ThornFAT.h"
#include "fs/tfat/Util.h"
#include "lib/printf.h"
#include "Memory.h"
#include "util.h"

namespace Armaz::ThornFAT {
	char ThornFATDriver::nothing[sizeof(DirEntry)] = {0};

	ThornFATDriver::ThornFATDriver(Partition *partition_): Driver(partition_) {
		root.startBlock = UNUSABLE;
		readSuperblock(superblock);
	}

	int ThornFATDriver::readSuperblock(Superblock &out) {
		HELLO("");
		int status = partition->read(&out, sizeof(Superblock), 0);
		CHECKS(RSUPERBLOCKH, "Couldn't read superblock");
		// if (status != 0) {
		// 	DEBUG("[ThornFATDriver::readSuperblock] Reading failed: %s\n", strerror(status));
		// }
		return -status;
	}

	int ThornFATDriver::find(fd_t fd, const char *path, DirEntry *out, off_t *offset, bool get_parent,
	                         std::string *last_name) {
		ENTER;
		REPORT;

		if (!FD_VALID(fd) && !path) {
			REPORT;
			WARNS(FATFINDH, "Both search types are unspecified.");
			EXIT;
			return -EINVAL;
		}

		REPORT;
#ifndef DEBUG_FATFIND
	METHOD_OFF(INDEX_FATFIND);
#endif

		if (strcmp(path, "/") == 0 || strlen(path) == 0) {
			REPORT;
			// For the special case of "/" or "" (maybe that last one should be invalid...), return the root directory.
			DirEntry &rootdir = getRoot(offset);
			if (out)
				*out = rootdir;

			REPORT;
			DBGF(FATFINDH, "Returning the root: %s", std::string(rootdir).c_str());
			FF_EXIT;
			REPORT;
			return 0;
		}

		REPORT;
		size_t count;
		size_t i;
		bool at_end;
		REPORT;

		std::vector<DirEntry> entries;
		std::vector<off_t> offsets;
		std::string newpath, remaining = path;

		REPORT;
		// Start at the root.
		DirEntry dir = getRoot();
		off_t dir_offset = superblock.startBlock * superblock.blockSize;
		REPORT;

		bool done = false;

		REPORT;
		do {
			REPORT;
			std::optional<std::string> search = Util::pathFirst(remaining, &newpath);
			remaining = newpath;
			at_end = newpath.empty() || !search.has_value();
			REPORT;

			if (at_end && get_parent) {
				// We just want the containing directory, thank you very much.

				if (out)
					*out = dir;

				if (offset)
					*offset = dir_offset;
				WARNS(FATFINDH, "Returning path component.");
				if (last_name)
					*last_name = search.value_or("");
				FF_EXIT;
				return 0;
			}
			REPORT;

			if (!search.has_value()) {
				FF_EXIT;
				return -ENOENT;
			}
			REPORT;

			int status = readDir(dir, entries, &offsets);
			REPORT;
			count = entries.size();
			if (status < 0) {
				WARN(FATFINDH, "Couldn't read directory. Status: " BDR " (%s)", status, STRERR(status));
				FF_EXIT;
				return status;
			}
			REPORT;

			// If we find any matches in the following for loop, this is set to 1.
			// If it stays 0, there were no (non-free) matches in the directory and the search failed.
			int found = 0;

			for (i = 0; i < count; i++) {
				REPORT;
				DirEntry &entry = entries[i];
				char *fname = entry.name.str;
				if (search == fname && !isFree(entry)) {
					REPORT;
					if (entry.isFile()) {
						if (at_end) {
							// We're at the end of the path and it's a file!
							DBG(FATFINDH, "Returning file at the end.");
							DBGF(FATFINDH, "  Name" DL " " BSTR DM " offset" DL " " BLR, entry.name.str, offsets[i]);

							if (out)
								*out = entry;
							if (offset)
								*offset = offsets[i];

							FF_EXIT;
							return 0;
						}

						// At this point, we've found a file, but we're still trying to search it like a directory.
						// That's not valid because it's a directory, so an ENOTDIR error occurs.
						WARN(FATFINDH, "Not a directory: " BSR, fname);
						FF_EXIT;
						return -ENOTDIR;
					} else if (at_end) {
						REPORT;
						// This is a directory at the end of the path. Success.
						DBG(FATFINDH, "Returning directory at the end.");
						DBG2(FATFINDH, "  Name:", entry.name.str);

						if (out)
							*out = entry;
						if (offset)
							*offset = offsets[i];

						FF_EXIT;
						return 0;
					}
					REPORT;

					// This is a directory, but we're not at the end yet. Search within this directory next.
					dir = entry;
					dir_offset = offsets[i];
					found = 1;
					break;
				}
				REPORT;
			}
			REPORT;

			done = remaining.empty();

			if (!found) {
				// None of the (non-free) entries in the directory matched.
				DBG(FATFINDH, "Returning nothing.");
				FF_EXIT;
				return -ENOENT;
			}
			REPORT;
		} while (!done);
		REPORT;
		// It shouldn't be possible to get here.
		WARNS(FATFINDH, "Reached the end of the function " UDARR " " IDS("EIO"));
		FF_EXIT;
		return -EIO;
	}

	size_t ThornFATDriver::forget(block_t start) {
		ENTER;

		DBGNE(FORGETH, "Forgetting", start);

		int64_t next;
		block_t block = start;
		size_t removed = 0;
		for (;;) {
			next = readFAT(block);
			if (next == FINAL) {
				DBGFE(FORGETH, "Freeing " BLR " (next == FINAL)", block);
				writeFAT(0, block);
				removed++;
				break;
			} else if (0 < next) {
				DBGFE(FORGETH, "Freeing " BLR " (0 < next)", block);
				writeFAT(0, block);
				removed++;
				block = next;
			} else
				break;
		}

		blocksFree += removed;
		EXIT;
		return removed;
	}

	size_t ThornFATDriver::chainLength(block_t start) {
		HELLO("");
		ENTER;
		size_t length = 0;
		if (readFAT(start) == 0) {
			WARNS("chain_length", "Start block is free.");
		} else {
			block_t block = start;
			for (;;) {
				length++;
				block = readFAT(block);
				if (block == -2) {
					break;
				} else if (block == 0) {
					WARNS("chain_length", "Chain ended on free block.");
					break;
				} else if (block == -1) {
					WARNS("chain_length", "Chain ended on invalid block.");
					break;
				} else if (block < -2) {
					DIE("chain_length", "Chain contains an invalid block: " BDR, block);
				}
			}
		}

		EXIT;
		return length;
	}

	int ThornFATDriver::writeEntry(const DirEntry &dir, off_t offset) {
		HELLO(dir.name.str);
		ENTER;
		DBGF(WRENTRYH, "Writing directory entry at offset " BLR " (" BLR DMS BLR DL BLR ") with filename " BSTR DM
			" length " BUR DM " start block " BDR " (next: " BDR ")",
			offset, offset / superblock.blockSize, (offset % superblock.blockSize) / sizeof(DirEntry),
			(offset % superblock.blockSize) % sizeof(DirEntry),  dir.name.str, dir.length, dir.startBlock,
			readFAT(dir.startBlock));

		// CHECKSEEK(WRENTRYH, "Invalid offset:", offset);
		// lseek(imgfd, offset, SEEK_SET);
		// IFERRNOXC(WARN(WRENTRYH, "lseek() failed " UDARR " " DSR, strerror(errno)));

		int status = partition->write(&dir, sizeof(DirEntry), offset);
		if (status != 0) {
			DBGF(WRENTRYH, "Writing failed: %s", strerror(status));
			return -status;
		}
		// IFERRNOXC(WARN(WRENTRYH, "write() failed " UDARR " " DSR, strerror(errno)));

		if (offset == superblock.startBlock * superblock.blockSize) {
			// If we're writing to "." in the root, update ".." as well.
			// We need to make a copy of the entry so we can change its filename to "..".
			DirEntry dir_cpy = dir;
			memset(dir_cpy.name.str, 0, sizeof(dir_cpy.name.str));
			dir_cpy.name.str[0] = dir_cpy.name.str[1] = '.';
			// write(imgfd, &dir_cpy, sizeof(DirEntry));
			status = partition->write(&dir_cpy, sizeof(DirEntry), offset + sizeof(DirEntry));
			SCHECKX(WRENTRYH, "Writing failed");
		}

		if (dir.startBlock == superblock.startBlock) {
			DBGF(WRENTRYH, "Writing root (" BDR SUDARR BDR "); dir %c= rootptr.", root.length, dir.length, &dir ==
				&root? '=' : '!');
			if (&dir != &root)
				root.length = dir.length;
		}

		EXIT;
		return 0;
	}

	DirEntry & ThornFATDriver::getRoot(off_t *offset) {
		HELLO("");
		ENTER;

		off_t start = superblock.startBlock * superblock.blockSize;
		printf("%llu * %llu = %llu\n", superblock.startBlock, superblock.blockSize, superblock.startBlock * superblock.blockSize);
		if (offset) {
			DBGFE("getRoot", "Setting offset to " BLR, start);
			*offset = start;
		}

		// If the root directory is already cached, we can simply return a reference to the cached entry.
		if (root.startBlock != UNUSABLE) {
			EXIT;
			return root;
		}

		int status = partition->read(&root, sizeof(DirEntry), start);
		if (status)
			DBGF(GETROOTH, "Reading failed: %s", STRERR(status));

		EXIT;
		return root;
	}


	int ThornFATDriver::readDir(const DirEntry &dir, std::vector<DirEntry> &entries, std::vector<off_t> *offsets,
	                         int *first_index) {
		HELLO(dir.name.str);
#ifndef DEBUG_DIRREAD
		METHOD_OFF(INDEX_DIRREAD);
#endif

		ENTER;
		DBGFE("readDir", IDS("Reading directory ") BSTR, dir.name.str);

		if (dir.length == 0 || !dir.isDirectory()) {
			// If the directory is free or actually a file, it's not valid.
			DR_EXIT; WARN("readDir", "Not a directory.", "");
			return -ENOTDIR;
		}

		if (dir.length % sizeof(DirEntry) != 0)
			WARN("readDir", "Directory length " BDR " isn't a multiple of DirEntry size " BLR ".", dir.length,
				sizeof(DirEntry));

		DBGF("readDir", "%s", std::string(dir).c_str());

		const size_t count = dir.length / sizeof(DirEntry);
		entries.clear();
		entries.resize(count);
		DBGFE("readDir", "count = " BULR, count);

		if (offsets) {
			offsets->clear();
			offsets->resize(count);
		}

		std::vector<uint8_t> raw;
		size_t byte_c;

		DBGFE("readDir", "About to read from " BSR, std::string(dir).c_str());

		int status = readFile(dir, raw, &byte_c);
		if (status < 0) {
			DR_EXIT;
			DBGF("readDir", "readFile failed: " BSR, strerror(-status));
			return status;
		}

		// for (uint8_t ch: raw)
		// 	DEBUG("%x ", ch & 0xff);
		// DEBUG("\n");

		char first_name[THORNFAT_PATH_MAX + 1];
		memcpy(first_name, raw.data() + offsetof(DirEntry, name), THORNFAT_PATH_MAX);
		if (strcmp(first_name, ".") == 0) {
			// The directory contains a "." entry, presumably in addition to a ".." entry.
			// This means there are two meta-entries before the actual entries.
			if (first_index)
				*first_index = 2;
		} else if (strcmp(first_name, "..") == 0) {
			// The directory appears to contain just ".." and not "." (unless the order is reversed,
			// but that should never happen). This means there's just one meta-entry.
			if (first_index)
				*first_index = 1;
		}

		DBGFE("readDir", "first_name = \"" BSR "\", byte_c = " BULR ", count = " BULR, first_name, byte_c, count);

		// for (size_t i = 0; i < raw.size(); ++i)
		// 	DEBUG("[%lu] '%c' (%d)\n", i, raw[i], raw[i]);

		block_t block = dir.startBlock;
		int rem;

		for (size_t i = 0; i < count; i++) {
			memcpy(entries.data() + i, raw.data() + sizeof(DirEntry) * i, sizeof(DirEntry));

			// for (size_t j = 0; j < sizeof(DirEntry); ++j) {
			// 	const char ch = *(raw.data() + sizeof(DirEntry) * i + j);
			// 	if (ch == '\0')
			// 		DEBUG("[%lu] \\0\n", j);
			// 	else
			// 		DEBUG("[%lu] %c\n", j, ch);
			// }

			DirEntry entry;
			memcpy(&entry, raw.data() + sizeof(DirEntry) * i, sizeof(DirEntry));
			DBGFE("readDir", "[i=%d, offset=%5lu] %s", i, sizeof(DirEntry) * i, std::string(entry).c_str());

			const int entries_per_block = superblock.blockSize / sizeof(DirEntry);
			rem = i % entries_per_block;

			if (offsets)
				(*offsets)[i] = block * superblock.blockSize + rem * sizeof(DirEntry);

			if (rem == entries_per_block - 1)
				block = readFAT(block);
		}

		for (const DirEntry &entry: entries) {
			(void) entry;
			DBGFE("readDir", "Entry: " BSR, std::string(entry).c_str());
		}

		DR_EXIT;
		return 0;
	}

	int ThornFATDriver::readFile(const DirEntry &file, std::vector<uint8_t> &out, size_t *count) {
		ENTER;
		DBGF(FILEREADH, "Reading file \"" BSR "\" of length " BULR " @ " BDR, file.name.str, file.length,
			file.startBlock * superblock.blockSize);
		// DBGF("readFile", "file.startBlock = " BDR ", superblock.blockSize = " BUR, file.startBlock,
		// 	superblock.blockSize);
		if (file.length == 0) {
			if (count)
				*count = 0;
			EXIT;
			return 0;
		}

		const auto bs = superblock.blockSize;
		// DEBUG("[ThornFATDriver::readFile] bs = %u\n", bs);

		if (count)
			*count = file.length;

		out.clear();
		out.resize(file.length, '*');
		uint8_t *ptr = out.data();
		size_t remaining = file.length;
		block_t block = file.startBlock;
		while (0 < remaining) {
			if (remaining <= bs) {
				checkBlock(block);
				int status = partition->read(ptr, remaining, block * bs);
				SCHECK(FILEREADH, "Couldn't read from partition");
				ptr += remaining;
				remaining = 0;

				block_t nextblock = readFAT(block);
				if (nextblock != FINAL) {
					// The file should end here, but the file allocation table says there are still more blocks.
#ifdef SHRINK_DIRS
					const bool shrink = true;
#else
					const bool shrink = false;
#endif

					if (!shrink || !file.isDirectory()) {
						WARN(FILEREADH, "%s " BSR " has no bytes left, but the file allocation table says more blocks "
							"are allocated to it.", file.isFile()? "File" : "Directory", file.name.str);
						WARN(FILEREADH, SUB "remaining = " BDR DM " readFAT(" BDR ") = " BDR DM " bs = " BDR, remaining,
							block, readFAT(block), bs);
					} else {
						WARN(FILEREADH, "%s " BSR " has extra FAT blocks; trimming.",
							file.isFile()? "File" : "Directory", file.name.str);
						writeFAT(FINAL, block);
						DBGF(FILEREADH, BDR " ← FINAL", block);
						block_t shrinkblock = nextblock;
						for (;;) {
							nextblock = readFAT(shrinkblock);
							if (nextblock == FINAL) {
								DBG(FILEREADH, "Finished shrinking (FINAL).");
								break;
							} else if (nextblock == 0) {
								DBG(FILEREADH, "Finished shrinking (0).");
								break;
							} else if ((block_t) superblock.fatBlocks <= nextblock) {
								WARN(FILEREADH, "FAT[" BDR "] = " BDR ", outside of FAT (" BDR " block%s)", shrinkblock,
									nextblock, superblock.fatBlocks, superblock.fatBlocks == 1? "" : "s");
								break;
							}

							writeFAT(0, shrinkblock);
							++blocksFree;
							shrinkblock = nextblock;
						}
					}
				}
			} else {
				if (readFAT(block) == FINAL) {
					// There's still more data that should be remaining after this block, but
					// the file allocation table says the file doesn't continue past this block.

					WARN(FILEREADH, "File still has " BDR " byte%s left, but the file allocation table doesn't have a "
						"next block after " BDR ".", PLURALS(remaining), block);
					EXIT;
					return -EINVAL;
				} else {
					checkBlock(block);
					DBGF(FILEREADH, "block * bs = " BUR, block * bs);
					int status = partition->read(ptr, bs, block * bs);
					SCHECK(FILEREADH, "Couldn't read from partition");
					ptr += bs;
					remaining -= bs;
					block = readFAT(block);
				}
			}
		}

		EXIT;
		return 0;
	}

	int ThornFATDriver::newFile(const char *path, size_t length, FileType type, const Times *times,
	                            DirEntry *dir_out, off_t *offset_out, DirEntry *parent_dir_out,
	                            off_t *parent_offset_out, bool noalloc) {
		HELLO(path);
#ifndef DEBUG_NEWFILE
		METHOD_OFF(INDEX_NEWFILE);
#endif
		ENTER;
		DBG2(NEWFILEH, "Trying to create new file at", path);

		DirEntry parent;
		off_t parent_offset = 0;
		std::string last_name;
		int status = find(-1, path, &parent, &parent_offset, true, &last_name);
		bool parent_is_new = status == 1;

		if (status < 0 && status != -ENOENT) {
			// It's fine if we get ENOENT. The existence of a file shouldn't be a prerequisite for its creation.
			// DEBUG("[ThornFATDriver::newFile] find failed: %s\n", strerror(-status));
			// NF_EXIT;
			// return status;
			SCHECK(NEWFILEH, "fat_find failed");
		}

		// First, check the filename length.
		const size_t ln_length = last_name.size();
		if (THORNFAT_PATH_MAX < ln_length) {
			// If it's too long, we can give up early.
			// DEBUG("[ThornFATDriver::newFile] Name too long (%lu chars)\n", ln_length);
			WARN(NEWFILEH, "Name too long: " BSR " " UDARR " " IDS("ENAMETOOLONG"), last_name.c_str());
			NF_EXIT;
			return -ENAMETOOLONG;
		}

		// TODO: implement time.
		DirEntry newfile(times == NULL? Times(0, 0, 0) : *times, length, type);
		block_t free_block = findFreeBlock();
		if (noalloc) {
			// We provide an option not to allocate space for the file. This is helpful for fat_rename, when we just
			// need to find an offset where an existing directory entry can be moved to. It's possibly okay if no
			// free block is available if the parent directory has enough space for another entry.
			newfile.startBlock = 0;
		} else {
			// We'll need at least one free block to store the new file in so we can
			// store the starting block in the directory entry we'll create soon.
			if (free_block == UNUSABLE) {
				// If we don't have one, we'll complain about having no space left and give up.
				WARNS(NEWFILEH, "No free block " UDARR " " IDS("ENOSPC"));
				NF_EXIT;
				return -ENOSPC;
			}

			SUCC(NEWFILEH, "Allocated " BSR " at block " BDR ".", path, free_block);

			// Allocate the first block and decrement the free blocks count.
			writeFAT(FINAL, free_block);
			--blocksFree;
			newfile.startBlock = free_block;

			// There's not a point in copying the name to the entry if this function
			// is being called just to get an offset to move an existing entry to.
			memcpy(newfile.name.str, last_name.c_str(), ln_length);
		}

		block_t old_free_block = free_block;

		// Read the directory to check whether there's a freed entry we can recycle. This can save us a lot of pain.
		std::vector<DirEntry> entries;
		std::vector<off_t> offsets;
		int first_index;
		status = readDir(parent, entries, &offsets, &first_index);
		SCHECK(NEWFILEH, "readDir failed");
		size_t count = entries.size();

		// Check all the directory entries except the initial meta-entries.
		off_t offset = -1;
		size_t offset_index = -1;
		for (size_t i = (size_t) first_index; i < count; ++i) {
			// DEBUG("[%lu] free(%d): %s\n", i, isFree(entries[i])? 1 : 0, std::string(entries[i]).c_str());
			if (isFree(entries[i])) {
				// We found one! Store its offset in `offset` to replace its previous value of -1.
				offset = offsets[i];
				offset_index = i;
				SUCC(NEWFILEH, ICS("Found") " freed entry at offset " BLR ".", offset);
				break;
			}
		}

		const size_t bs = superblock.blockSize;
		const uint64_t block_c = Armaz::Util::updiv(length, bs);

		// There are four different scenarios that we have to accommodate when we want to add a new directory entry.
		// The first and easiest is when the parent directory has a slot that used to contain an entry but was later
		// freed. The second is when the parent directory is less than a block long, so we take its first free slot.
		// The third is when the parent directory spans multiple blocks and is fully block-aligned, so we have to go
		// through the effort of adding a new block to it. The fourth is like the third, except the parent directory
		// isn't block-aligned so we don't have to expend a lot of effort, much like the second scenario.

		bool increase_parent_length = true;

		if (offset != -1) {
			// Scenario one: we found a free entry earlier. Way too easy.
			// DEBUG("[ThornFATDriver::newFile] Scenario one.\n");
			CDBGS(A_CYAN, NEWFILEH, "Scenario one.");

			status = writeEntry(newfile, offset);
			// if (status < 0) {
			// 	DEBUG("[ThornFATDriver::newFile] Couldn't add entry to parent directory: %s\n", strerror(-status));
			// 	NF_EXIT;
			// 	return status;
			// }
			SCHECK(NEWFILEH, "Couldn't add entry to parent directory");


			DBGF(NEWFILEH, "[S1] parent->length" SDEQ BUR DMS "offset_index" SDEQ BULR DMS "parent->length / "
				"sizeof(DirEntry)" SDEQ BULR, parent.length, offset_index, parent.length / sizeof(DirEntry));
			if (offset_index < parent.length / sizeof(DirEntry)) {
				// Don't increase the length if it already extends past this entry.
				increase_parent_length = false;
			}
		} else if (parent.length <= bs - sizeof(DirEntry)) {
			// Scenario two: the parent directory has free space in its first block, which is also pretty easy to deal
			// with.
			CDBGS(A_CYAN, NEWFILEH, "Scenario two.");

			if (!noalloc && !hasFree(block_c)) {
				// If we're allocating space for the new file, we need enough blocks to hold it.
				// If we don't have enough blocks, we should stop now before we make any changes to the filesystem.
				WARN(NEWFILEH, "No free block " UDARR " " DSR, "ENOSPC");
				writeFAT(0, old_free_block);
				NF_EXIT;
				return -ENOSPC;
			}

			// The offset of the free space is the sum of the parent's starting offset and its length.
			// Try to write the entry to it.
			offset = parent.startBlock * bs + parent.length;
			status = writeEntry(newfile, offset);
			SCHECK(NEWFILEH, "Couldn't add entry to parent directory");
		} else {
			// Scenarios three and four: the parent directory spans multiple blocks.
			// Four is mercifully simple, three less so.

			// Skip to the last block; we don't need to read or change anything in the earlier blocks.
			size_t remaining = parent.length;
			block_t block = parent.startBlock;
			DBGN(NEWFILEH, "Parent start block:", block);
			int skipped = 0;
			while (bs < remaining) {
				block = readFAT(block);
				remaining -= bs;
				if (++skipped <= NEWFILE_SKIP_MAX)
					DBGN(NEWFILEH, "  Skipping to", block);
			}

			if (NEWFILE_SKIP_MAX < skipped)
				DBGF(NEWFILEH, "  " IDS("... %d more"), skipped);

			DBGF(NEWFILEH, "bs - sizeof(DirEntry) < remaining  " UDBARR "  " BLR " - " BLR " < " BLR "  " UDBARR "  " 
				BLR " < " BLR, bs, sizeof(DirEntry), remaining, bs - sizeof(DirEntry), remaining);

			if (bs - sizeof(DirEntry) < remaining) {
				// Scenario three, the worst one: there isn't enough free space left in the
				// parent directory to fit in another entry, so we have to add another block.
				CDBGS(A_CYAN, NEWFILEH, "Scenario three.");

				bool nospc = false;

				if (!noalloc && !hasFree(2 + block_c)) {
					// We need to make sure we have at least two free blocks: one for the expansion of the parent
					// directory and another for the new directory entry. We also need more free blocks to contain the
					// file, but that's mostly handled by the for loop below.
					// TODO: is it actually 1 + block_c?
					writeFAT(0, old_free_block);
					nospc = true;
				} else if (noalloc && !hasFree(1)) {
					// If we don't need to allocate space for the new file,
					// we need only one extra block for the parent directory.
					nospc = true;
				}

				if (nospc) {
					// If we don't have enough free blocks, the operation fails due to lack of space.
					WARN(NEWFILEH, "No free block (need " BLR ") " UDARR " " DSR, 2 + block_c, "ENOSPC");
					// NF_EXIT;
					return -ENOSPC;
				}

				// We'll take the value from the free block we found earlier to use as the next block in the parent
				// directory.
				writeFAT(block, old_free_block);
				block = old_free_block;

				if (!noalloc) {
					// If we need to allocate space for the new file, we now try to find
					// another free block to use as the new file's start block.
					free_block = findFreeBlock();
					if (free_block == UNUSABLE) {
						WARN(NEWFILEH, "No free block " UDARR " " DSR, "ENOSPC");
						writeFAT(0, old_free_block);
						NF_EXIT;
						return -ENOSPC;
					}

					// Decrease the free block count and assign the free block as the new file's starting block.
					--blocksFree;
					newfile.startBlock = free_block;
				}

				// Write the directory entry to the block we allocated for expanding the parent directory.
				offset = block * bs;
				status = writeEntry(newfile, offset);
				SCHECKX(NEWFILEH, "Couldn't add entry to parent directory");
			} else {
				// Scenario four: there's enough space in the parent directory to add the new entry. Nice.
				CDBGS(A_CYAN, NEWFILEH, "Scenario four.");

				offset = block * bs + remaining;
				status = writeEntry(newfile, offset);
				SCHECKX(NEWFILEH, "Couldn't add entry to parent directory");
			}
		}

		if (increase_parent_length) {
			// Increase the size of the parent directory and write it back to its original offset.
			DBG(NEWFILEH, IMS("Increasing parent length."));
			DBGFE(NEWFILEH, "Parent length" DLS BDR SUDARR BDR, parent.length, (uint32_t) (parent.length +
				sizeof(DirEntry)));
			parent.length += sizeof(DirEntry);
			status = writeEntry(parent, parent_offset);
			SCHECKX(NEWFILEH, "Couldn't write the parent directory to disk");
		}

		if (!noalloc) {
			// If the file is more than one block in length, we need to allocate more entries in the file allocation
			// table.
			block_t block = newfile.startBlock;
			for (auto size_left = length; bs < size_left; size_left -= bs) {
				block_t another_free_block = findFreeBlock();
				if (another_free_block == UNUSABLE) {
					writeFAT(FINAL, block);
					blocksFree = -1;
					writeFAT(0, old_free_block);
					WARN(NEWFILEH, "No free block " UDARR " " DSR, "ENOSPC");
					countFree();
					NF_EXIT;
					return -ENOSPC;
				}

				--blocksFree;
				writeFAT(another_free_block, block);
				block = another_free_block;
			}

			writeFAT(FINAL, block);
		}

		if (dir_out)
			*dir_out = newfile;

		if (offset_out)
			*offset_out = offset;

		if (parent_dir_out)
			*parent_dir_out = parent;

		if (parent_offset_out)
			*parent_offset_out = parent_offset;

		if (offset_out)
			*offset_out = offset;

		if (parent_dir_out)
			*parent_dir_out = parent;

		if (parent_offset_out)
			*parent_offset_out = parent_offset;

		SUCCSE(NEWFILEH, "Created new file.");
		NF_EXIT;
		return parent_is_new;
	}

	int ThornFATDriver::remove(const char *path) {
		DBGF(REMOVEH, "Removing " BSR " from the filesystem.", path);

		DirEntry found;
		off_t offset;

		DBG_OFFE();
		int status = find(-1, path, &found, &offset, false, nullptr);
		DBG_ONE();
		SCHECK(REMOVEH, "fat_find failed");

		if (found.isDirectory())
			return -EISDIR;

		DBGF(REMOVEH, "Offset" DL " " BLR DM " start" DL " " BDR DM " next block" DL " " BDR, offset, found.startBlock,
			readFAT(found.startBlock));

		forget(found.startBlock);
		found.startBlock = 0;
		writeEntry(found, offset);

		return 0;
	}

	int ThornFATDriver::resize(DirEntry &file, off_t file_offset, size_t new_size) {
		HELLO(file.name.str);
		ENTER;
		if (file.length == new_size) {
			DBGF(RESIZEH, IYS("Skipping") " resize for path " BSTR " to size " BLR " (same size)",
				file.name.str, new_size);
			EXIT;
			return 0;
		}

		size_t old_calculated;
		const size_t new_c = Armaz::Util::updiv(new_size, static_cast<size_t>(superblock.blockSize));
		const size_t old_apparent = Armaz::Util::updiv(file.length, static_cast<size_t>(superblock.blockSize));

		if (file.startBlock == 0) {
			WARNS(RESIZEH, "Trying to resize a free file. Defaulting to apparent block length.");
			old_calculated = old_apparent;
		} else {
			old_calculated = chainLength(file.startBlock);
		}

		const size_t old_c = old_calculated;
		DBGF(RESIZEH, ICS("Resizing") " for path " BSTR " to new size " BLR "; file" DARR "length" DL " " BDR DM
			" blocks needed for old size" DL " " BLR DM " blocks needed for new size" DL " " BLR,
			file.name.str, new_size, file.length, old_c, new_c);

		if (new_size == 0) {
			DBG(RESIZEH, "Truncating to 0.");
			// Deallocate all blocks belonging to the file and keep just the first block allocated.
			forget(file.startBlock);
			writeFAT(FINAL, file.startBlock);

			// Update the file length and then save the directory entry.
			file.length = 0;
			writeEntry(file, file_offset);
		} else if (old_c == new_c) {
			DBGF(RESIZEH, "No change in block count (" BLR ")", old_c);
			// Easy: no block boundaries need to be crossed.

			int status;
			if (new_size < file.length) {
				// We need to zero out the end.
				status = zeroOutFree(file, new_size);
				SCHECKX(RESIZEH, "fat_zero_out_free status");
			}

			DBGF(RESIZEH, ICS("Trying to change file" DARR "length") " at " BLR " from " BDR " byte(s) to " BLR " byte(s)",
				file_offset, file.length, new_size);
			file.length = new_size;
			status = writeEntry(file, file_offset);
			SCHECKX(RESIZEH, "fat_write_entry status");
		} else if (new_c < old_c) {
			// Less easy: we have to shrink the file.

			DBGF(RESIZEH, "Reducing block count from " BLR " to " BLR ".", old_c, new_c);

			block_t blocks[old_c];
			block_t block = file.startBlock;
			for (size_t i = 0; i < old_c; i++) {
				blocks[i] = block;
				DBGN(RESIZEH, IMS("Noting") " a FAT block:", blocks[i]);
				block = readFAT(block);
			}

			DBGE(RESIZEH, "About to zero out free space.");
			int status = zeroOutFree(file, new_size);
			SCHECKX(RESIZEH, "fat_zero_out_free status");

			for (size_t i = new_c; i < old_c; i++) {
				DBGN(RESIZEH, ILS("Freeing") " a FAT block:", blocks[i]);
				writeFAT(0, blocks[i]);
				++blocksFree;
			}

			if (0 < new_c) {
				DBGN(RESIZEH, ILS("Ending") " a FAT block:", blocks[new_c - 1]);
				writeFAT(FINAL, blocks[new_c - 1]);
			}

			DBGN(RESIZEH, "Setting new byte length to", new_size);
			file.length = new_size;
			status = writeEntry(file, file_offset);
			SCHECKX(RESIZEH, "fat_write_entry failed");
		} else {
			DBGF(RESIZEH, "Increasing block count from " BLR " to " BLR ".", old_c, new_c);

			// The tricky one: we have to absorb more blocks.
			// Let's first make sure we can add enough additional blocks.
			size_t to_add = new_c - (old_c? old_c : 1);
			DBGF(RESIZEH, "Trying to add " BLR " block%s", PLURALS(to_add));
			if (!hasFree(to_add)) {
				WARNS(RESIZEH, "Not enough blocks " UDARR " " IDS("ENOSPC"));
				EXIT;
				return -ENOSPC;
			}

			block_t new_block;
			block_t block = file.startBlock;
			size_t skipped = 0, added = 0;

			for (;;) {
				new_block = readFAT(block);
				if (new_block <= 0)
					break;
				block = readFAT(block);
				++skipped;
			}

			for (size_t i = 0; i < to_add; i++) {
				new_block = findFreeBlock();
				if (new_block == -1) {
					WARNS(RESIZEH, "Out of space " UDARR " " IDS("ENOSPC"));
					EXIT;
					return -ENOSPC;
				}

				DBGN(RESIZEH, IGS("Added") " block", new_block);
				writeFAT(new_block, block);
				writeFAT(FINAL, new_block);
				block = new_block;
				++added;
			}

			blocksFree -= added;

			DBGF(RESIZEH, "Trying to change file" DARR "length at offset " BLR " from " BDR " to " BLR ".",
				file_offset, file.length, new_size);
			file.length = new_size;
			writeEntry(file, file_offset);
		}

		SUCCS(RESIZEH, "Successfully resized.");
		EXIT; return 0;
	}

	int ThornFATDriver::zeroOutFree(const DirEntry &file, size_t new_size) {
		HELLO(file.name.str);
		ENTER;
		DBGF(ZEROOUTFREEH, "Freeing space for file at block " BDR DS " size" DL " " BDR " " UDARR " " BDR,
			file.startBlock, file.length, new_size);
		uint32_t bs = superblock.blockSize;

		if (new_size % bs == 0) {
			// There's nothing to do here.
			EXIT;
			return 0;
		}

		size_t diff = file.length - new_size;
		if (file.length <= new_size) {
			WARN(ZEROOUTFREEH, "New size isn't smaller than old size (diff = " BDR "); skipping.", diff);
			EXIT;
			return 0;
		}

		uint32_t remaining = new_size;
		block_t block = file.startBlock;

		CHECKBLOCK(ZEROOUTFREEH, "Invalid block");
		// lseek(imgfd, block * bs, SEEK_SET);
		// ECHECKXC(ZEROOUTFREEH, "lseek() failed");

		size_t position = block * bs;

		DBGF(ZEROOUTFREEH, "bs = " BDR DM " remaining = " BUR, bs, remaining);
		while (bs < remaining) {
			DBGF(ZEROOUTFREEH, BDR " " UDARR " " BDR, block, readFAT(block));
			block = readFAT(block);
			CHECKBLOCK(ZEROOUTFREEH, "Invalid block in loop");
			position = block * bs;
			// lseek(imgfd, block * bs, SEEK_SET);
			// ECHECKXC(ZEROOUTFREEH, "lseek() failed");
			remaining -= bs;
		}

		position += remaining;
		static char empty[1024] = {0};
		int status;
		while (sizeof(empty) <= diff) {
			status = partition->write(empty, sizeof(empty), position);
			SCHECK(ZEROOUTFREEH, "Writing to partition failed");
			position += sizeof(empty);
			diff -= sizeof(empty);
		}

		if (0 < diff) {
			status = partition->write(empty, diff, position);
			SCHECK(ZEROOUTFREEH, "Writing to partition failed");
		}

		DBG(ZEROOUTFREEH, "Done.");
		EXIT;
		return 0;
	}

	block_t ThornFATDriver::findFreeBlock() {
		auto block_c = superblock.blockCount;
		for (decltype(block_c) i = 0; i < block_c; i++)
			if (readFAT(i) == 0)
				return i;
		return UNUSABLE;
	}

	int ThornFATDriver::directoryEmpty(const DirEntry &dir) {
		HELLO(dir.name.str);
		if (!dir.isDirectory())
			return 0;

		if (dir.length == 0)
			return 1;

		std::vector<DirEntry> entries;
		int status = readDir(dir, entries);
		if (status < 0)
			return status;

		const size_t count = entries.size();
		if (count <= 1)
			return 1;

		for (size_t i = 0; i < count; i++) {
			if (hasStuff(entries[i]))
				return 0;
		}

		return 1;
	}

	void ThornFATDriver::updateName(DirEntry &entry, const char *new_name) {
		strncpy(entry.name.str, new_name, sizeof(entry.name.str));
	}

	void ThornFATDriver::updateName(DirEntry &entry, const std::string &new_name) {
		updateName(entry, new_name.c_str());
	}

	block_t ThornFATDriver::readFAT(size_t block_offset) {
		block_t out;
		int status = partition->read(&out, sizeof(block_t), superblock.blockSize + block_offset * sizeof(block_t));
		SCHECK("readFAT", "Reading failed");
		DBGFE("readFAT", "Adjusted offset: " BULR " " DARR " " BDR,
			superblock.blockSize + block_offset * sizeof(block_t), out);
		return out;
	}

	int ThornFATDriver::writeFAT(block_t block, size_t block_offset) {
		// DEBUG("writeFAT adjusted offset: %lu <- %d\n", superblock.blockSize + block_offset * sizeof(block_t), block);
		int status = partition->write(&block, sizeof(block_t), superblock.blockSize + block_offset * sizeof(block_t));
		SCHECK("writeFAT", "Writing failed");
		return 0;
	}

	void ThornFATDriver::initFAT(size_t table_size, size_t block_size) {
		size_t written = 0;
		DBGF("initFAT", "writeOffset = %lu, table_size = %lu", writeOffset, table_size);
		DBGF("initFAT", "sizeof: Filename[%lu], Times[%lu], block_t[%lu], FileType[%lu], mode_t[%lu], DirEntry[%lu], "
			"Superblock[%lu]", sizeof(Filename), sizeof(Times), sizeof(block_t), sizeof(FileType), sizeof(mode_t),
			sizeof(DirEntry), sizeof(Superblock));

		DBG("initFAT", "Zeroing out FAT.");
		size_t position = block_size;
		size_t remaining = table_size * block_size;
		static char zeros[512] = {};
		while (512 <= remaining) {
			partition->write(zeros, 512, position);
			position += 512;
			remaining -= 512;
			DBGN("initFAT", "Remaining:", remaining);
		}

		partition->write(zeros, remaining, position);

		// These blocks point to the FAT, so they're not valid regions to write data.
		writeOffset = block_size;
		DBGF("initFAT", "Writing UNUSABLE %lu times to %lu", table_size + 1, writeOffset);
		writeMany(UNUSABLE, table_size + 1);
		written += table_size + 1;
		DBGF("initFAT", "Writing FINAL 1 time to %lu", writeOffset);
		writeMany(FINAL, 1);
		++written;
	}

	void ThornFATDriver::initData(size_t, size_t table_size) {
		root.reset();
		root.name.str[0] = '.';
		root.length = 2 * sizeof(DirEntry);
		root.startBlock = table_size + 1;
		root.type = FileType::Directory;
		write(root);
		root.name.str[1] = '.';
		write(root);
		root.name.str[1] = '\0';
	}

	bool ThornFATDriver::hasFree(const size_t count) {
		size_t scanned = 0;
		size_t block_c = superblock.blockCount;
		for (size_t i = 0; i < block_c; i++)
			if (readFAT(i) == 0 && count <= ++scanned)
				return true;
		return false;
	}

	ssize_t ThornFATDriver::countFree() {
		if (0 <= blocksFree)
			return blocksFree;

		blocksFree = 0;
		for (int i = superblock.blockCount - 1; 0 <= i; --i)
			if (readFAT(i) == 0)
				++blocksFree;
		return blocksFree;
	}

	bool ThornFATDriver::checkBlock(block_t block) {
		if (block < 1) {
			DBGF("checkBlock", "Invalid block: " BDR, block);
			for (;;);
			return false;
		}

		return true;
	}

	bool ThornFATDriver::isFree(const DirEntry &entry) {
		return entry.startBlock == 0 || readFAT(entry.startBlock) == 0;
	}

	bool ThornFATDriver::hasStuff(const DirEntry &entry) {
		if (entry.isFile())
			return !isFree(entry);
		if (entry.isDirectory())
			return strcmp(entry.name.str, "..") != 0 && 0 < entry.length;
		return false;
	}

	bool ThornFATDriver::isRoot(const DirEntry &entry) {
		return superblock.startBlock == entry.startBlock;
	}

	size_t ThornFATDriver::tableSize(size_t block_count, size_t block_size) {
		return block_count < block_size / sizeof(block_t)?
			1 : Armaz::Util::updiv(block_count, block_size / sizeof(block_t));
	}

	int ThornFATDriver::rename(const char *srcpath, const char *destpath) {
		HELLO(srcpath);
		DBGL;
		DBGF(RENAMEH, CMETHOD("rename") BSTR " " UARR " " BSTR, srcpath, destpath);

		DirEntry dest_entry, src_entry;
		off_t dest_offset, src_offset;
		int status;

		// It's okay if the destination doesn't exist, but the source is required to exist.
		status = find(-1, srcpath, &src_entry, &src_offset, false, nullptr);
		SCHECK(RENAMEH, "fat_find failed for old path");

		status = find(-1, destpath, &dest_entry, &dest_offset, false, nullptr);
		DBGN(RENAMEH, "fat_find status:", status);
		if (status < 0 && status != -ENOENT) {
			SCHECK(RENAMEH, "fat_find failed for new path");
		}

		if (0 <= status) {
			DBGF(RENAMEH, IUS("Destination " BSR " already exists."), dest_entry.name.str);
			// Because the destination already exists, we need to unlink it to free up its space.
			// The offset of its directory entry will then be available for us to use.
			// We don't have to worry about not having enough space in this case; in fact,
			// we're actually freeing up at least one block.
			status = remove(destpath);
			SCHECK(RENAMEH, "Couldn't unlink target");
		} else if (status == -ENOENT) {
			DBGFE(RENAMEH, IUS("Destination " BSR " doesn't exist."), destpath);
			// If the destination doesn't exist, we need to try to add another directory entry to the destination
			// directory.

			status = newFile(destpath, 0, src_entry.isDirectory()? FileType::Directory : FileType::File,
			                 &src_entry.times, &dest_entry, &dest_offset, nullptr, nullptr, true);
			SCHECK(RENAMEH, "Couldn't create new directory entry");
			DBGF(RENAMEH, BLR SUDARR BLR, src_offset, dest_offset);

			// We now know the offset of a free directory entry in the destination's parent directory.
			// (We just created one, after all.) Now we copy over the source entry before freeing it.
		}

		// Zero out the source entry's filename, then copy in the basename of the destination.
		std::optional<std::string> destbase = Util::pathLast(destpath);
		if (!destbase.has_value()) {
			DBG(RENAMEH, "destbase has no value!");
			return -EINVAL;
		}

		memset(src_entry.name.str, 0, THORNFAT_PATH_MAX + 1);
		strncpy(src_entry.name.str, destbase->c_str(), THORNFAT_PATH_MAX);

		// Once we've ensured the destination doesn't exist or no longer exists,
		// we move the source's directory entry to the destination's offset.
		status = partition->write(&src_entry, sizeof(DirEntry), dest_offset);
		SCHECK(RENAMEH, "Writing failed");

		// Now we need to remove the source entry's original directory entry from the disk image.
		status = partition->write(nothing, sizeof(DirEntry), src_offset);
		SCHECK(RENAMEH, "Writing failed");

		SUCC(RENAMEH, "Moved " BSR " to " BSR ".", srcpath, destpath);
		return 0;
	}

	int ThornFATDriver::release(const char *) {
		return 0;
	}

	int ThornFATDriver::statfs(const char *, FS::DriverStats &stats) {
		stats.magic = MAGIC;
		stats.nameMax = THORNFAT_PATH_MAX;
		stats.optimalBlockSize = superblock.blockSize;
		stats.totalBlocks = superblock.blockCount;
		// TODO: stats.freeBlocks
		// TODO: stats.availableBlocks
		// TODO: stats.files
		// TODO: stats.freeFiles
		// TODO: stats.flags
		return 0;
	}

	int ThornFATDriver::utimens(const char *path, const timespec &ts) {
		HELLO(path);

		long time_now = ts.tv_sec;

		DBGL;
		DBGF(UTIMENSH, BMETHOD("utimens") BSTR DM " current time " BLR, path, time_now);

		DirEntry dir;
		off_t offset;

		int status = find(-1, path, &dir, &offset);
		SCHECK(UTIMENSH, "find failed");

		dir.times.accessed = time_now;
		dir.times.modified = time_now;

		status = writeEntry(dir, offset);
		SCHECK(UTIMENSH, "writeEntry failed");
		return 0;
	}

	int ThornFATDriver::create(const char *path, mode_t modes) {
		HELLO(path);
		DBGL;
		DBGF(CREATEH, PMETHOD("create") BSTR DM " mode " BDR, path, modes);

		int status = find(-1, path);
		if (0 <= status) {
			// The file already exists, so we don't have to bother creating another entry with the same name.
			// That would be a pretty dumb thing to do...
			DBG2(CREATEH, "File already exists:", path);
			return -EEXIST;
		}

		const std::string simplified = FS::simplifyPath(path);

		DirEntry newfile, parent;
		off_t offset, poffset;
		status = newFile(simplified.c_str(), 0, FileType::File, nullptr, &newfile, &offset, &parent, &poffset, false);
		SCHECK(CREATEH, "newfile failed");

		newfile.modes = modes;
		DBGF(CREATEH, "About to write new file at offset " BLR, offset);
		writeEntry(newfile, offset);

		SUCC(CREATEH, "Done. " IDS("{") "offset" DLS BLR DMS "poffset" DLS BLR IDS("}"), offset, poffset);
		return 0;
	}

	int ThornFATDriver::write(const char *path, const char *buffer, size_t size, off_t offset) {
		HELLO(path);
		DBGL;
		DBGF(WRITEH, PMETHOD("write") BSTR DMS "offset " BLR DMS "size " BLR, path, offset, size);

		const size_t bs = superblock.blockSize;

		DirEntry file;
		off_t file_offset;

		DBG(WRITEH, "Finding file");
		int status = find(-1, path, &file, &file_offset);
		SCHECK(WRITEH, "fat_find failed");
		DBG2(WRITEH, "Found file:", file.name.str);

		size_t length = file.length;
		size_t new_size = offset + size > length? offset + size : length;
		status = resize(file, file_offset, new_size);
		SCHECK(WRITEH, "resize failed");

		block_t block = file.startBlock;
		DBGF(WRITEH, "Starting write with block offset " BLR ", file offset " BLR ".", block * bs, offset);

		off_t offset_left = offset;
		while (static_cast<off_t>(bs) <= offset_left) {
			const block_t block_fat = readFAT(block);
			if (block_fat != -2 && block_fat < 1) {
				DBGN(WRITEH, "Next block is invalid:", block);
			}

			block = block_fat;
			DBGF(WRITEH, "Skipping to block " BDR " " DARR " offset " BLR, block, block * bs);
			offset_left -= bs;
		}

		CHECKBLOCK(WRITEH, "Invalid block");

		size_t position = block * bs + offset_left;
		ssize_t size_left = size;
		ssize_t to_write;
		ssize_t bytes_written = 0;
		if (offset_left + size <= bs) {
			// If the amount to write doesn't require us to move into other blocks, just finish everything here.
			DBGF(WRITEH, "Performing " IPS("small write") " of size " BLR " to block " BDR " with " BLR
				" remaining offset.", size, block, offset_left);
			status = partition->write(buffer, size, position);
			position += size;
			SCHECK(WRITEH, "Couldn't write from buffer");
			size_left = 0;
			bytes_written += size;
		} else if (0 < offset_left) {
			// We'll need to write multiple blocks and we're currently not block-aligned, so let's fix that.
			to_write = bs - offset_left;
			DBGF(WRITEH, "Writing " BLR " block%s to block-align.", PLURALS(to_write));
			status = partition->write(buffer, to_write, position);
			CHECKS(WRITEH, "Couldn't write to block-align");
			position += to_write;
			DBGH(WRITEH, "Now at offset", block * bs + offset_left + to_write);
			SCHECK(WRITEH, "Couldn't write from buffer");

			block = readFAT(block);
			const block_t block_fat = readFAT(block);
			if (block_fat != FINAL && block_fat < 1)
				DBGN(WRITEH, "Next block is invalid:", block);

			CHECKBLOCK(WRITEH, "Invalid block during block alignment");
			DBGN(WRITEH, "Jumping to block", block);
			position = block * bs;
			DBGH(WRITEH, "Now at offset", block * bs);

			bytes_written += to_write;
			size_left -= to_write;
		}

		while (0 < size_left) {
			// Don't write more than one block at a time.
			to_write = static_cast<ssize_t>(bs) < size_left? bs : size_left;

			DBGF(WRITEH, "Writing to block " A_PINK BDR A_RESET " (to_write = " BLR DMS "position = " BULR ")",
				block, to_write, position);
			status = partition->write(buffer + bytes_written, to_write, position);
			SCHECK(WRITEH, "Couldn't read into buffer");
			position += to_write;

			bytes_written += to_write;
			size_left  -= to_write;
			block = readFAT(block);
			if (block == FINAL && size_left != 0) {
				// We still have more to write, but this block was the last one.
				WARNS(WRITEH, "There is still more to write, but there are no more blocks left!");
				break;
			} else if (block != FINAL) {
				DBGF(WRITEH, "Next block" DL " " BDR DS " seeking to" DL " " BULR, block, block * bs);
				CHECKBLOCK(WRITEH, "Invalid block while writing");
				position = block * bs;
			}
		}

		// TODO: time syscall. Syscalls in general, really.
		// file.times.modified = NOW;

		writeEntry(file, file_offset);
		SUCC(WRITEH, "Wrote " BLR " byte%s", PLURALS(bytes_written));
		return bytes_written;
	}

	int ThornFATDriver::mkdir(const char *path, mode_t mode) {
		HELLO(path);
		UNUSED(mode);
		DBGL;
		DBGF(MKDIRH, PMETHOD("mkdir") BSTR DM " mode " BDR, path, mode);

		const std::string simplified = FS::simplifyPath(path);

		DBGF(MKDIRH, "simplified = \"" BSR "\"", simplified.c_str());

		DirEntry subdir, parent;
		off_t offset;

		DBG(MKDIRH, "Creating new file for directory.");
		int status = newFile(simplified.c_str(), sizeof(DirEntry), FileType::Directory, nullptr, &subdir, &offset,
		                     &parent, nullptr, false);
		SCHECK(MKDIRH, "newFile failed");

		// Set the copy of the parent directory with its named changed to ".." as the first entry in the new directory.
		updateName(parent, "..");
		status = writeEntry(parent, subdir.startBlock * superblock.blockSize);
		SCHECK(MKDIRH, "writeEntry failed");

		DBG(MKDIRH, "Done.");
		return 0;
	}

	int ThornFATDriver::truncate(const char *path, off_t size) {
		HELLO(path);
		DBGL;
		DBGF("truncate", MMETHOD("truncate") BSTR DM " size " BLR, path, size);
		DirEntry found;
		off_t offset;
		int status = find(-1, path, &found, &offset);
		SCHECK("truncate", "fat_find failed");

		return resize(found, offset, size);
	}

	int ThornFATDriver::ftruncate(const char *path, off_t size) {
		return 0;
	}

	int ThornFATDriver::rmdir(const char *path) {
		HELLO(path);
		DBGL;
		DBGF(RMDIRH, RMETHOD("rmdir") BSTR, path);

		if (strcmp(path, "/") == 0 || !path[0]) {
			WARNS(RMDIRH, "Preserving root" SUDARR IDS("EINVAL"));
			return -EINVAL;
		}

		DirEntry found;
		off_t offset;
		int status = find(-1, path, &found, &offset);
		SCHECK(RMDIRH, "find failed");

		if (!found.isDirectory()) {
			WARNS(RMDIRH, "Can't remove non-directory" SUDARR IDS("ENOTDIR"));
			return -ENOTDIR;
		}

		if (directoryEmpty(found) == 0) {
			std::vector<DirEntry> entries;
			status = readDir(found, entries);
			SCHECK(RMDIRH, "readDir failed");

			const size_t count = entries.size();
			for (size_t i = 0; i < count; i++)
				DBGF(RMDIRH, "Existing free? " BSR " (" BSR ")",
					hasStuff(entries[i])? "no" : "ja", entries[i].name.str);

			WARN(RMDIRH, "Can't remove nonempty directory" SUDARR IDS("ENOTEMPTY") " (length: " BDR ")", found.length);
			return -ENOTEMPTY;
		}

		forget(found.startBlock);
		found.startBlock = 0;
		found.length = 0;
		writeEntry(found, offset);

		DBG(RMDIRH, "Done.");
		return 0;
	}

	int ThornFATDriver::unlink(const char *path) {
		HELLO(path);
		DBGL;
		DBGF(UNLINKH, RMETHOD("unlink") BSTR, path);

		int status = remove(path);
		SCHECK(UNLINKH, "remove failed");

		DBG(UNLINKH, "Done.");
		return 0;
	}

	int ThornFATDriver::open(const char *path) {
		return 0;
	}

	int ThornFATDriver::read(const char *path, void *buffer, size_t size, off_t offset) {
		HELLO(path);
		const size_t bs = superblock.blockSize;

		DBGL;
		DBGF(READH, OMETHOD("read") BSTR DM " offset " BLR DM " size " BLR, path, offset, size);

		DirEntry file;
		off_t file_offset;

		int status = find(-1, path, &file, &file_offset);
		SCHECK(READH, "fat_find failed");

		size_t length = file.length;
		if (length == 0 && file.isDirectory()) {
			// This should already be prevented by fat_find().
			DBG(READH, "ENOENT");
			WARNS(READH, "Can't open empty (i.e., free) directory" SUDARR IDS("ENOENT"));
			return -ENOENT;
		}

		if (length <= static_cast<size_t>(offset)) {
			// The offset extends beyond the length of the file, so there's nothing to read.
			DBGN(READH, "Done (nothing to read).", 0);
			return 0;
		}

		block_t block = file.startBlock;
		size_t bytes_read = 0;
		size_t size_left = size;
		size_t to_read;

		off_t offset_left = offset;
		while (static_cast<off_t>(bs) <= offset_left) {
			const block_t block_fat = readFAT(block);
			if (block_fat != -2 && block_fat < 1) {
				DBGF(READH, "Next block is invalid (" BDR ")" SUDARR IDS("EINVAL"), block);
				return -EINVAL;
			}

			block = block_fat;
			offset_left -= bs;
		}

		// Seek to the block containing the relevant offset plus the remainder of the offset.
		CHECKBLOCK(READH, "Invalid block");
		size_t position = block * bs + offset_left;
		DBGN(READH, "Seek:", block * bs + offset_left);

		if (offset_left + size <= bs) {
			// If the amount to read doesn't require us to move into other blocks, just finish everything here.
			DBGN(READH, "Performing \e[36msmall read\e[36m from block", block);
			DBGN(READH, "Offset left:", offset_left);
			DBGN(READH, "Size:", size);
			status = partition->read(buffer, size, position);
			SCHECK(READH, "Couldn't read into buffer:");
			position += size;
			size_left = 0;
			bytes_read += size;
		} else if (0 < offset_left) {
			// We'll need to read multiple blocks and we're currently not block-aligned, so let's fix that.
			to_read = bs - offset_left;
			status = partition->read(buffer, to_read, position);
			SCHECK(READH, "Couldn't read into buffer:");
			position += to_read;
			bytes_read += to_read;
			size_left -= to_read;
		}

		while (0 < size_left) {
			// Don't read more than one block at a time.
			to_read = bs < size_left? bs : size_left;

			DBGN(READH, "Reading from block:\e[36m", block);
			// status = read(imgfd, buf + bytes_read, to_read);
			status = partition->read(static_cast<char *>(buffer) + bytes_read, to_read, position);
			SCHECK(READH, "Couldn't read into buffer:");

			position += to_read;
			bytes_read += to_read;
			size_left  -= to_read;

			if (size_left == 0)
				break;

			block = readFAT(block);
			if (block == -2 && size_left != 0) {
				// We still have more to read, but this block was the last one. That's not good.
				// Log a warning and break out of the loop. This won't happen unless the code is
				// bad or the disk image is corrupted.
				WARNS(READH, "There is still more to read, but there are no more blocks left!");
				break;
			}

			DBGF(READH, "Next block: " BDR "; size left: " BLR, block, size_left);
			if (block != FINAL)
				position = block * bs;
		}

		// TODO: time syscall.
		// file.times.accessed = NOW;
		DBGN(READH, "Writing new access time to entry:", file.times.accessed);
		status = writeEntry(file, file_offset);
		SCHECK(READH, "fat_write_entry (update accessed) status");

		// Return the number of bytes we read into the buffer.
		DBGN(READH, "Done.", bytes_read);
		return bytes_read;
	}

	int ThornFATDriver::readdir(const char *path, FS::DirFiller filler) {
		HELLO(path);
		DBGL;
		DBGF(READDIRH, OMETHOD("readdir") BSTR, path);
		// DBG_OFFE();

		DirEntry found;
		off_t file_offset;

		REPORT;
		int status = find(-1, path, &found, &file_offset);
		SCHECK(READDIRH, "find failed");

		REPORT;

		if (!found.isDirectory()) {
			// You can't really use readdir with a file.
			DBG(READDIRH, "Can't readdir() a file " DARR " ENOTDIR");
			DBG_ON();
			return -ENOTDIR;
		}
		REPORT;

		if (!isRoot(found))
			// Only the root directory has a "." entry stored in the disk image.
			// For other directories, it has to be added dynamically.
			filler(".", 0);

		REPORT;
		DBGF(READDIRH, "Found directory at offset " BLR ": " BSR, file_offset, std::string(found).c_str());

		std::vector<DirEntry> entries;
		std::vector<off_t> offsets;

		REPORT;
		status = readDir(found, entries, &offsets);
		SCHECK(READDIRH, "readDir failed");
		const size_t count = entries.size();
		DBGF(READDIRH, "Count: " BULR, count);
		REPORT;

		size_t excluded = 0;
#ifdef READDIR_MAX_INCLUDE
		size_t included = 0;
		int last_index = -1;
#endif
		REPORT;

		for (size_t i = 0; i < count; ++i) {
			REPORT;
			const DirEntry &entry = entries[i];
			DBGF(READDIRH, "[] %s: %s", isFree(entry)? "free" : "not free", std::string(entry).c_str());
			if (!isFree(entry)) {
				filler(entry.name.str, offsets[i]);
#ifdef READDIR_MAX_INCLUDE
				last_index = i;
				if (++included < READDIR_MAX_INCLUDE)
					DBGF(READDIRH, "Including entry %s at offset %ld.", entry.name.str, offsets[i]);
#else
				DBGF(READDIRH, "Including entry %s at offset %ld.", entry.name.str, offsets[i]);
#endif
			} else
				excluded++;
		}

		REPORT;

#ifdef READDIR_MAX_INCLUDE
		if (READDIR_MAX_INCLUDE < included)
			DBGF(READDIRH, "... " BULR " more", count - READDIR_MAX_INCLUDE);

		if (READDIR_MAX_INCLUDE <= included)
			DBGF(READDIRH, "Including entry %s at offset %ld.", entries[last_index].name.str, offsets[last_index]);
#endif

		REPORT;
		if (0 < excluded) {
			DBGF(READDIRH, "Excluding " BULR " freed entr%s.", PLURALY(excluded));
		}

		REPORT;
		DBGE(READDIRH, "Done.");
		DBG_ON();
		return 0;
	}

	int ThornFATDriver::getattr(const char *path, FS::FileStats &) {
		return 0;
	}

	int ThornFATDriver::getsize(const char *path, size_t &out) {
		DirEntry found;
		int status = find(-1, path, &found);
		SCHECK("getsize", "find failed");
		out = found.length;
		return 0;
	}

	int ThornFATDriver::isdir(const char *path) {
		DirEntry found;
		int status = find(-1, path, &found);
		SCHECK("isdir", "find failed");
		return found.isDirectory()? 1 : 0;
	}

	int ThornFATDriver::isfile(const char *path) {
		DirEntry found;
		int status = find(-1, path, &found);
		SCHECK("isfile", "find failed");
		return found.isFile()? 1 : 0;
	}

	int ThornFATDriver::exists(const char *path) {
		DirEntry found;
		return find(-1, path, &found);
	}

	bool ThornFATDriver::make(uint32_t block_size) {
		// int status = partition->clear();
		// if (status != 0) {
		// 	DBGF("make", "Clearing partition failed: %s", STRERR(status));
		// 	return false;
		// }

		const size_t block_count = partition->length / block_size;

		if (block_count < MINBLOCKS) {
			DBGF("make", "Number of blocks for partition is too small: %lu", block_count);
			return false;
		}

		if (block_size % sizeof(DirEntry)) {
			// The block size must be a multiple of the size of a directory entry because it must be possible to fill a
			// block with directory entries without any space left over.
			DBGF("make", "Block size isn't a multiple of %lu.", sizeof(DirEntry));
			return false;
		}

		if (block_size < 2 * sizeof(DirEntry)) {
			DBG("make", "Block size must be able to hold at least two directory entries.");
			return false;
		}

		if (block_size < sizeof(Superblock)) {
			DBGF("make", "Block size isn't large enough to contain a superblock (%luB).", sizeof(Superblock));
			return false;
		}

		const size_t table_size = tableSize(block_count, block_size);

		if (UINT32_MAX <= table_size) {
			DBGF("make", "Table size too large: %u", table_size);
			return false;
		}

		superblock = {
			.magic = MAGIC,
			.blockCount = block_count,
			.fatBlocks = static_cast<uint32_t>(table_size),
			.blockSize = block_size,
			.startBlock = static_cast<block_t>(table_size + 1)
		};

		writeOffset = 0;
		write(superblock);
		initFAT(table_size, block_size);
		initData(block_count, table_size);
		return true;
	}

	bool ThornFATDriver::verify() {
		return readSuperblock(superblock)? false : (superblock.magic == MAGIC);
	}
}
