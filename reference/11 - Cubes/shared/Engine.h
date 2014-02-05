/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef ENGINE_H
#define ENGINE_H

#include "Config.h"
#include "Mathematics.h"
#include "Activation.h"
#include "Simulation.h"

namespace engine
{
	using activation::ObjectId;
	using activation::ActiveId;
	using activation::ActivationSystem;
	
	/*
		Response queue is used to implement corrections.
		Each object id may be in the queue only once, we pop objects off the front
		of the queue when determining which corrections to include in a packet.
	*/

	template <typename T> class ResponseQueue
	{
	public:
	
		void Clear()
		{
			responses.clear();
		}
	
		bool AlreadyQueued( ObjectId id )
		{
			for ( typename std::list<T>::iterator itor = responses.begin(); itor != responses.end(); ++itor )
				if ( itor->id == id )
					return true;
			return false;
		}
	
		void QueueResponse( const T & response )
		{
			assert( !AlreadyQueued( response.id ) );
			responses.push_back( response );
		}
	
		bool PopResponse( T & response )
		{
			if ( !responses.empty() )
			{
	 			response = *responses.begin();
				responses.pop_front();
				return true;
			}
			else
				return false;
		}
	
	private:
	
		std::list<T> responses;
	};

	/*
		Packet queue is used to simulate a networked P2P mesh with node id [0,maxNodes-1]
		Its job is to hold packets in various queues for some simulated latency time
		before becoming ready to "send".
	*/

	class PacketQueue
	{
	public:
	
		struct Packet
		{
			float timeInQueue;
			int sourceNodeId;
			int destinationNodeId;
			std::vector<unsigned char> data;
		};
	
		PacketQueue();
		~PacketQueue();
	
		void Clear();
		void QueuePacket( int sourceNodeId, int destinationNodeId, unsigned char * data, int bytes );
		void SetDelay( float delay );
		void Update( float deltaTime );
		Packet * PacketReadyToSend();
	
	private:
	
		float delay;						// number of seconds to delay packet sends (hold in queue)
		std::vector<Packet*> queue;
	};

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
	void UnquantizeVector( const int32_t & x, const int32_t & y, const int32_t & z, math::Vector & vector, float res );
}

#endif
