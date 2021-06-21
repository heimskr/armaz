// Credit: https://github.com/rsta2/circle/blob/master/lib/memory64.cpp

#include "Memory.h"
#include "util.h"
#include "lib/printf.h"

// #define DEBUG_ALLOCATION

Armaz::Memory::Allocator *global_memory = nullptr;

namespace Armaz::Memory {
	uintptr_t getCoherentPage(unsigned slot) {
		return MEM_COHERENT_REGION + slot * PAGE_SIZE;
	}

	Allocator::Allocator(char *start_, char *high_): start(start_), high(high_), end(start_) {
		start = (char *) realign((uintptr_t) start);
		global_memory = this;
		highestAllocated = reinterpret_cast<uintptr_t>(start_);
	}

	Allocator::Allocator(): Allocator((char *) 0, (char *) 0) {}

	uintptr_t Allocator::realign(uintptr_t val) {
#ifdef DEBUG_ALLOCATION
		printf("realign(0x%lx)\n", val);
#endif
		size_t offset = (val + sizeof(BlockMeta)) % MEMORY_ALIGN;
		if (offset)
			val += MEMORY_ALIGN - offset;
		return val;
	}

	Allocator::BlockMeta * Allocator::findFreeBlock(BlockMeta * &last, size_t size) {
#ifdef DEBUG_ALLOCATION
		printf("findFreeBlock(0x%lx, %lu)\n", last, size);
#endif
		BlockMeta *current = base;
		while (current && !(current->free && current->size >= size)) {
			last = current;
			current = current->next;
		}
		return current;
	}

	Allocator::BlockMeta * Allocator::requestSpace(BlockMeta *last, size_t size) {
#ifdef DEBUG_ALLOCATION
		printf("requestSpace(0x%lx, %lu)\n", last, size);
#endif
		BlockMeta *block = (BlockMeta *) realign((uintptr_t) end);

		if (last)
			last->next = block;

#ifdef PROACTIVE_PAGING
		auto &pager = Kernel::getPager();
		while (highestAllocated <= (uintptr_t) block + size) {
			pager.assignAddress(reinterpret_cast<void *>(highestAllocated));
			highestAllocated += PAGE_LENGTH;
		}
#endif

		block->size = size;
		block->next = nullptr;
		block->free = 0;

		end = reinterpret_cast<char *>(block) + block->size + sizeof(BlockMeta) + 1;
		return block;
	}

	void * Allocator::allocate(size_t size, size_t /* alignment */) {
#ifdef DEBUG_ALLOCATION
		printf("allocate(%lu)\n", size);
#endif
		BlockMeta *block = nullptr;

		if (size <= 0)
			return nullptr;

		if (!base) {
			block = requestSpace(nullptr, size);
			if (!block)
				return nullptr;
			base = block;
		} else {
			BlockMeta *last = base;
			block = findFreeBlock(last, size);
			if (!block) {
				block = requestSpace(last, size);
				if (!block)
					return nullptr;
			} else {
				split(*block, size);
				block->free = 0;
			}
		}

		allocated += block->size + sizeof(BlockMeta);
		return block + 1;
	}

	void Allocator::split(BlockMeta &block, size_t size) {
#ifdef DEBUG_ALLOCATION
		printf("split(0x%lx, %lu)\n", &block, size);
#endif
		if (block.size > size + sizeof(BlockMeta)) {
			// We have enough space to split the block, unless alignment takes up too much.
			BlockMeta *new_block = (BlockMeta *) realign((uintptr_t) &block + size + sizeof(BlockMeta) + 1);

			// After we realign, we need to make sure that the new block's new size isn't negative.

			if (block.next) {
				const int new_size = (char *) block.next - (char *) new_block - sizeof(BlockMeta);

				// Realigning the new block can make it too small, so we need to make sure the new block is big enough.
				if (new_size > 0) {
					new_block->size = new_size;
					new_block->next = block.next;
					new_block->free = 1;
					block.next = new_block;
					block.size = size;
				}
			} else {
				const int new_size = (char *) &block + block.size - (char *) new_block;

				if (new_size > 0) {
					new_block->size = new_size;
					new_block->free = 1;
					new_block->next = nullptr;
					block.size = size;
					block.next = new_block;
				}
			}
		}
	}

	Allocator::BlockMeta * Allocator::getBlock(void *ptr) {
#ifdef DEBUG_ALLOCATION
		printf("getBlock(0x%lx)\n", ptr);
#endif
		return (BlockMeta *) ptr - 1;
	}

	void Allocator::free(void *ptr) {
#ifdef DEBUG_ALLOCATION
		printf("free(0x%lx)\n", ptr);
#endif
		if (!ptr)
			return;

		BlockMeta *block_ptr = getBlock(ptr);
		block_ptr->free = 1;
		allocated -= block_ptr->size + sizeof(BlockMeta);
		merge();
	}

	int Allocator::merge() {
#ifdef DEBUG_ALLOCATION
		printf("merge()\n");
#endif
		int count = 0;
		BlockMeta *current = base;
		while (current && current->next) {
			if (current->free && current->next->free) {
				current->size += sizeof(BlockMeta) + current->next->size;
				current->next = current->next->next;
				count++;
			} else
				current = current->next;
		}

		return count;
	}

	void Allocator::setBounds(char *new_start, char *new_high) {
#ifdef DEBUG_ALLOCATION
		printf("setBounds(0x%lx, 0x%lx)\n", new_start, new_high);
#endif
		start = (char *) realign((uintptr_t) new_start);
		highestAllocated = reinterpret_cast<uintptr_t>(start);
		high = new_high;
		end = new_start;
	}

	size_t Allocator::getAllocated() const {
		return allocated;
	}

	size_t Allocator::getUnallocated() const {
		return high - start - allocated;
	}
}

extern "C" void * malloc(size_t size) {
#ifdef DEBUG_ALLOCATION
	printf("malloc(0x%lx)\n", size);
#endif
	if (global_memory == nullptr)
		return nullptr;
	auto result = global_memory->allocate(size);
#ifdef DEBUG_ALLOCATION
	printf("malloc complete: 0x%llx\n", result);
#endif
	return result;
}

extern "C" void * calloc(size_t count, size_t size) {
	printf("calloc(0x%llx x 0x%llx)\n", count, size);
	void *chunk = malloc(count * size);
	if (chunk)
		memset(chunk, 0, count * size);
	return chunk;
}

extern "C" void free(void *ptr) {
	if (global_memory)
		global_memory->free(ptr);
}

#ifdef __clang__
void * operator new(size_t size)   {
	auto result = malloc(size);
	return result;
}
void * operator new[](size_t size) {
	auto result = malloc(size);
	return result;
}
void * operator new(size_t, void *ptr)   {
	return ptr;
}
void * operator new[](size_t, void *ptr) {
	return ptr;
}
void operator delete(void *ptr)   noexcept {
	free(ptr);
}
void operator delete[](void *ptr) noexcept {
	free(ptr);
}
void operator delete(void *, void *)   noexcept {}
void operator delete[](void *, void *) noexcept {}
void operator delete(void *, unsigned long)   noexcept {}
void operator delete[](void *, unsigned long) noexcept {}
#endif
