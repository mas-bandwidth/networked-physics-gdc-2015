// Core Library - Copyright (c) 2008-2015, Glenn Fiedler

#include "core/Memory.h"
#include <new>

namespace core
{
	struct MemoryGlobals 
	{
		static const int ALLOCATOR_MEMORY = sizeof( MallocAllocator ) + sizeof( ScratchAllocator );

		uint8_t buffer[ALLOCATOR_MEMORY];

		MallocAllocator * default_allocator;

#if USE_SCRATCH_ALLOCATOR
		ScratchAllocator * scratch_allocator;
#else
		MallocAllocator * scratch_allocator;
#endif

		MemoryGlobals() : default_allocator( nullptr ), scratch_allocator( nullptr ) {}
	};

	MemoryGlobals memory_globals;

	namespace memory
	{
		void initialize( uint32_t temporary_memory ) 
		{
			uint8_t * p = memory_globals.buffer;
			memory_globals.default_allocator = new (p) MallocAllocator();
			p += sizeof( MallocAllocator );
#if USE_SCRATCH_ALLOCATOR
			memory_globals.scratch_allocator = new (p) ScratchAllocator( *memory_globals.default_allocator, temporary_memory );
#else
			memory_globals.scratch_allocator = new (p) MallocAllocator();
#endif
		}

		Allocator & default_allocator() 
		{
			CORE_ASSERT( memory_globals.default_allocator );
			return *memory_globals.default_allocator;
		}

		Allocator & scratch_allocator() 
		{
			CORE_ASSERT( memory_globals.scratch_allocator );
			return *memory_globals.scratch_allocator;
		}

		void shutdown() 
		{
#if USE_SCRATCH_ALLOCATOR
			memory_globals.scratch_allocator->~ScratchAllocator();
#else
			memory_globals.scratch_allocator->~MallocAllocator();
#endif
			memory_globals.default_allocator->~MallocAllocator();
			memory_globals = MemoryGlobals();
		}
	}
}
