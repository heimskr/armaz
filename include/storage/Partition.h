#pragma once

#include "storage/StorageDevice.h"

namespace Armaz {
	struct MBREntry;

	struct Partition {
		StorageDevice *parent;
		/** Number of bytes after the start of the disk. */
		size_t offset;
		/** Length of the partition in bytes. */
		size_t length;

		Partition(StorageDevice &parent_, size_t offset_, size_t length_):
			parent(&parent_), offset(offset_), length(length_) {}

		Partition(StorageDevice &, const MBREntry &);

		int read(void *buffer, size_t size, size_t byte_offset);
		int write(const void *buffer, size_t size, size_t byte_offset);
		// int clear();
	};
}
