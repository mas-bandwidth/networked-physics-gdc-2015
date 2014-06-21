#include "Memory.h"

namespace protocol
{
	struct MemoryGlobals 
	{
		static const int ALLOCATOR_MEMORY = sizeof( MallocAllocator ) + sizeof( ScratchAllocator );

		uint8_t buffer[ALLOCATOR_MEMORY];

		MallocAllocator * default_allocator;
		MallocAllocator * default_scratch_allocator;//ScratchAllocator * default_scratch_allocator;

		MemoryGlobals() : default_allocator(0), default_scratch_allocator(0) {}
	};

	MemoryGlobals memory_globals;

	namespace memory
	{
		void initialize( uint32_t temporary_memory ) 
		{
			uint8_t * p = memory_globals.buffer;
			memory_globals.default_allocator = new (p) MallocAllocator();
			p += sizeof( MallocAllocator );
			memory_globals.default_scratch_allocator = new (p) MallocAllocator();// ScratchAllocator( *memory_globals.default_allocator, temporary_memory );
		}

		Allocator & default_allocator() 
		{
			assert( memory_globals.default_allocator );
			return * memory_globals.default_allocator;
		}

		Allocator & default_scratch_allocator() 
		{
			assert( memory_globals.default_scratch_allocator );
			return * memory_globals.default_scratch_allocator;
		}

		void shutdown() 
		{
			//memory_globals.default_scratch_allocator->~ScratchAllocator();
			memory_globals.default_scratch_allocator->~MallocAllocator();
			memory_globals.default_allocator->~MallocAllocator();
			memory_globals = MemoryGlobals();
		}
	}
}
