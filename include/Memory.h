#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "MemoryMap.h"

// Credit: https://github.com/rsta2/circle/blob/master/include/circle/memory.h

namespace Armaz::Memory {

	constexpr unsigned SLOT_PROP_MAILBOX = 0;
	constexpr unsigned SLOT_GPIO_VIRTBUF = 1;
	constexpr unsigned SLOT_TOUCHBUF     = 2;

	constexpr unsigned SLOT_VCHIQ_START = MEGABYTE / PAGE_SIZE / 2;
	constexpr unsigned SLOT_VCHIQ_END   = MEGABYTE / PAGE_SIZE - 1;

#if RASPPI >= 4
	constexpr unsigned SLOT_XHCI_START = MEGABYTE / PAGE_SIZE;
	constexpr unsigned SLOT_XHCI_END   = 4 * MEGABYTE / PAGE_SIZE - 1;
#endif

	constexpr size_t MEMORY_ALIGN = 32;

	uintptr_t getCoherentPage(unsigned slot);

	class Allocator {
		public:
			struct BlockMeta {
				size_t size;
				BlockMeta *next;
				bool free;
			};

		private:
			static constexpr size_t PAGE_LENGTH = 4096;

			// size_t align;
			size_t allocated = 0;
			char *start, *high, *end;
			BlockMeta *base = nullptr;
			uintptr_t highestAllocated = 0;

			uintptr_t realign(uintptr_t);
			BlockMeta * findFreeBlock(BlockMeta * &last, size_t);
			BlockMeta * requestSpace(BlockMeta *last, size_t);
			void split(BlockMeta &, size_t);
			int merge();

		public:
			Allocator(const Allocator &) = delete;
			Allocator(Allocator &&) = delete;

			Allocator(char *start_, char *high_);
			Allocator();

			Allocator & operator=(const Allocator &) = delete;
			Allocator & operator=(Allocator &&) = delete;

			void * allocate(size_t size, size_t alignment = 0);
			void free(void *);
			void setBounds(char *new_start, char *new_high);
			BlockMeta * getBlock(void *);
			size_t getAllocated() const;
			size_t getUnallocated() const;
	};
}

extern "C" {
	void * malloc(size_t);
	void * calloc(size_t, size_t);
	void free(void *);
	// int posix_memalign(void **memptr, size_t alignment, size_t size);
}

extern Armaz::Memory::Allocator *global_memory;

#define MEMORY_OPERATORS_SET
#ifdef __clang__
void * operator new(size_t size);
void * operator new[](size_t size);
void * operator new(size_t, void *ptr);
void * operator new[](size_t, void *ptr);
void operator delete(void *ptr)   noexcept;
void operator delete[](void *ptr) noexcept;
void operator delete(void *, void *)   noexcept;
void operator delete[](void *, void *) noexcept;
void operator delete(void *, unsigned long)   noexcept;
void operator delete[](void *, unsigned long) noexcept;
#else
#ifndef __cpp_exceptions
inline void * operator new(size_t size)   throw() { return malloc(size); }
inline void * operator new[](size_t size) throw() { return malloc(size); }
inline void * operator new(size_t, void *ptr)   throw() { return ptr; }
inline void * operator new[](size_t, void *ptr) throw() { return ptr; }
inline void operator delete(void *ptr)   throw() { free(ptr); }
inline void operator delete[](void *ptr) throw() { free(ptr); }
inline void operator delete(void *, void *)   throw() {}
inline void operator delete[](void *, void *) throw() {}
inline void operator delete(void *, unsigned long)   throw() {}
inline void operator delete[](void *, unsigned long) throw() {}
#else
inline void * operator new(size_t size)   { return malloc(size); }
inline void * operator new[](size_t size) { return malloc(size); }
inline void * operator new(size_t, void *ptr)   { return ptr; }
inline void * operator new[](size_t, void *ptr) { return ptr; }
inline void operator delete(void *ptr)   noexcept { free(ptr); }
inline void operator delete[](void *ptr) noexcept { free(ptr); }
inline void operator delete(void *, void *)   noexcept {}
inline void operator delete[](void *, void *) noexcept {}
inline void operator delete(void *, unsigned long)   noexcept {}
inline void operator delete[](void *, unsigned long) noexcept {}
#endif
#endif