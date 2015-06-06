// Core Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef CORE_ALLOCATOR_H
#define CORE_ALLOCATOR_H

#include <stdint.h>

namespace core
{
	// allocator base class

	class Allocator
	{
	public:

		static const uint32_t DEFAULT_ALIGN = 4;
		static const uint32_t SIZE_NOT_TRACKED = 0xffffffff;

		Allocator() {}
		virtual ~Allocator() {}
		
		virtual void * Allocate( uint32_t size, uint32_t align = DEFAULT_ALIGN ) = 0;

		virtual void Free( void * p ) = 0;

		virtual uint32_t GetAllocatedSize( void * p ) = 0;

		virtual uint32_t GetTotalAllocated() = 0;

	private:

	    Allocator( const Allocator & other );
	    Allocator & operator = ( const Allocator & other );
	};
}

#endif
