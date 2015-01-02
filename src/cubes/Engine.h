/*
	Networked Physics Demo
	Copyright © 2008-2015 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef CUBES_ENGINE_H
#define CUBES_ENGINE_H

#include "Config.h"
#include "Mathematics.h"
#include "Activation.h"
#include "Simulation.h"
#include <list>

namespace cubes
{
	using activation::ObjectId;
	using activation::ActiveId;
	using activation::ActivationSystem;
	
	/*
		Priority set. 
		Used to track n most important active objects to send,
		so we know which objects to include in each packet while
		distributing fairly according to priority and last time sent.
	*/

	class PrioritySet
	{
	public:
		
		void Clear()
		{
			entries.clear();
			sorted_entries.clear();
		}
		
		void AddObject( int activeIndex )
		{
			assert( activeIndex == (int) entries.size() );
			ObjectEntry entry;
			entry.activeIndex = activeIndex;
			entry.accumulator = 0.0f;
			entries.push_back( entry );
		}
		
		void RemoveObject( int index )
		{
			assert( index >= 0 );
			assert( index < (int) entries.size() );
			const int last = (int) entries.size() - 1;
			if ( index != last )
			{
				entries[index] = entries[last];
				entries[index].activeIndex = index;
			}
			entries.resize( entries.size() - 1 );
		}

		int GetNumObjects() const
		{
			return entries.size();
		}

		void SetAccumulator( int index, float accumulator )
		{
			assert( index >= 0 );
			assert( index < (int) entries.size() );
			entries[index].accumulator = accumulator;
		}
		
		float GetAccumulator( int index ) const
		{
			assert( index >= 0 );
			assert( index < (int) entries.size() );
			return entries[index].accumulator;
		}
		
		void SortObjects()
		{
			sorted_entries.resize( entries.size() );
			ObjectEntry * src = &entries[0];
			ObjectEntry * dst = &sorted_entries[0];
			memcpy( dst, src, sizeof(ObjectEntry) * entries.size() );
			std::sort( sorted_entries.begin(), sorted_entries.end() );
		}

 		int GetSortedObject( int index, float & priority ) const
		{
			assert( index >= 0 );
			assert( index < (int) sorted_entries.size() );
			priority = sorted_entries[index].accumulator;
			return sorted_entries[index].activeIndex;
		}
		
	private:

		struct ObjectEntry
		{
			int activeIndex;
			float accumulator;
			bool operator < ( const ObjectEntry & other ) const
			{
				return accumulator > other.accumulator;
			}
		};

		std::vector<ObjectEntry> entries;
		std::vector<ObjectEntry> sorted_entries;		// IMPORTANT: we just copy then sort, entries are small.
	};
	
	// helper functions for compression
	
	void CompressPosition( const math::Vector & position, uint64_t & compressed_position );
	void DecompressPosition( uint64_t compressed_position, math::Vector & position );

	void CompressOrientation( const math::Quaternion & orientation, uint32_t & compressed_orientation );
	void DecompressOrientation( uint32_t compressed_orientation, math::Quaternion & orientation );

	void QuantizeVector( const math::Vector & vector, int32_t & x, int32_t & y, int32_t & z, float res );
	void DequantizeVector( const int32_t & x, const int32_t & y, const int32_t & z, math::Vector & vector, float res );
}

#endif
