// Core Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef CORE_ARRAY_H
#define CORE_ARRAY_H

#include "Allocator.h"
#include "Types.h"

namespace core
{
	namespace array
	{
		template<typename T> inline uint32_t size( const Array<T> & a ) 	{ return a.m_size; }
		template<typename T> inline bool any( const Array<T> & a ) 			{ return a.m_size != 0; }
		template<typename T> inline bool empty( const Array<T> & a ) 		{ return a.m_size == 0; }
		
		template<typename T> inline T * begin( Array<T> & a ) 				{ return a.m_data; }
		template<typename T> inline const T * begin( const Array<T> & a ) 	{ return a.m_data; }
		template<typename T> inline T * end( Array<T> & a ) 				{ return a.m_data + a.m_size; }
		template<typename T> inline const T * end( const Array<T> & a ) 	{ return a.m_data + a.m_size; }
		
		template<typename T> inline T & front( Array<T> & a ) 				{ return a.m_data[0]; }
		template<typename T> inline const T & front( const Array<T> & a ) 	{ return a.m_data[0]; }
		template<typename T> inline T & back( Array<T> & a ) 				{ return a.m_data[a.m_size-1]; }
		template<typename T> inline const T & back(const Array<T> & a ) 	{ return a.m_data[a.m_size-1]; }

		template<typename T> void resize( Array<T> & a, uint32_t new_size );

		template<typename T> inline void clear( Array<T> & a ) { resize( a, 0 ); }
		template<typename T> inline void trim( Array<T> & a ) { set_capacity( a, a.m_size); }

		template<typename T> void set_capacity( Array<T> & a, uint32_t new_capacity );

		template<typename T> inline void reserve( Array<T> & a, uint32_t new_capacity )
		{
			if ( new_capacity > a.m_capacity )
				set_capacity( a, new_capacity );
		}

		template<typename T> void grow( Array<T> & a, uint32_t min_capacity )
		{
			uint32_t new_capacity = a.m_capacity * 2 + 8;
			if ( new_capacity < min_capacity )
				 new_capacity = min_capacity ;
			set_capacity( a, new_capacity );
		}

		template<typename T> void resize( Array<T> & a, uint32_t new_size )
		{
			if ( new_size > a.m_capacity )
				grow( a, new_size );
			a.m_size = new_size;
		}

		template<typename T> void set_capacity( Array<T> & a, uint32_t new_capacity )
		{
			if ( new_capacity == a.m_capacity )
				return;

			if ( new_capacity < a.m_size )
				resize( a, new_capacity );

			T * new_data = nullptr;

			if ( new_capacity > 0 )
			{
				new_data = (T*) a.m_allocator->Allocate( sizeof(T)*new_capacity, alignof(T) );
				memcpy( new_data, a.m_data, sizeof(T) * a.m_size );
			}

			a.m_allocator->Free( a.m_data );
			a.m_data = new_data;
			a.m_capacity = new_capacity;
		}

		template<typename T> inline void push_back( Array<T> & a, const T &item )
		{
			if ( a.m_size + 1 > a.m_capacity )
				grow( a, a.m_size + 1 );
			a.m_data[a.m_size++] = item;
		}

		template<typename T> inline void pop_back( Array<T> & a )
		{
			a.m_size--;
		}
	}

	template<typename T> inline Array<T>::Array( Allocator & allocator ) 
		: m_allocator( &allocator ), m_size(0), m_capacity(0), m_data(0) 
	{
		// ...
	}

	template<typename T> inline Array<T>::~Array()
	{
		m_allocator->Free( m_data );
	}

	template<typename T> Array<T>::Array( const Array<T> & other ) 
		: m_allocator( other.m_allocator ), m_size(0), m_capacity(0), m_data(0)
	{
		const uint32_t n = other.m_size;
		array::set_capacity( *this, n );
		memcpy( m_data, other.m_data, sizeof(T) * n );
		m_size = n;
	}

	template<typename T> Array<T> & Array<T>::operator = ( const Array<T> & other )
	{
		const uint32_t n = other.m_size;
		array::resize( *this, n );
		memcpy( m_data, other.m_data, sizeof(T) * n );
		return *this;
	}

	template<typename T> inline T & Array<T>::operator [] ( uint32_t i )
	{
		return m_data[i];
	}

	template<typename T> inline const T & Array<T>::operator [] ( uint32_t i ) const
	{
		return m_data[i];
	}
}

#endif
