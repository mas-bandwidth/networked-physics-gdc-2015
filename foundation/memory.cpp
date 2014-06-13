#include "memory.h"

#include <stdlib.h>
#include <assert.h>
#include <new>

namespace {
	using namespace foundation;

}

namespace foundation
{
	namespace memory_globals
	{
		void init(uint32_t temporary_memory) {
			char *p = _memory_globals.buffer;
			_memory_globals.default_allocator = new (p) MallocAllocator();
			p += sizeof(MallocAllocator);
			_memory_globals.default_scratch_allocator = new (p) ScratchAllocator(*_memory_globals.default_allocator, temporary_memory);
		}

		Allocator &default_allocator() {
			return *_memory_globals.default_allocator;
		}

		Allocator &default_scratch_allocator() {
			return *_memory_globals.default_scratch_allocator;
		}

		void shutdown() {
			_memory_globals.default_scratch_allocator->~ScratchAllocator();
			_memory_globals.default_allocator->~MallocAllocator();
			_memory_globals = MemoryGlobals();
		}
	}
}
