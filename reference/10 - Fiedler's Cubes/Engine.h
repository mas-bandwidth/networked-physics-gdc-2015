/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef ENGINE_H
#define ENGINE_H

#include "Config.h"
#include "Mathematics.h"
#include "Activation.h"
#include "Simulation.h"

#include <list>
#include <algorithm>
#include <vector>

namespace engine
{
	using activation::ObjectId;
	using activation::ActiveId;
	using activation::ActivationSystem;
	
	struct AuthorityEntry
	{
		ObjectId id;
		int authority;
		bool forced;
		float time;
	};

	class AuthorityManager
	{
	public:

		AuthorityManager()
		{
		}
	
		void Clear()
		{
			entries.clear();
		}

	 	bool SetAuthority( ObjectId id, int authority, bool force = false )
		{
			assert( authority >= 0 );
			assert( authority < MaxPlayers );
			const int count = entries.size();
			for ( int i = 0; i < count; ++i )
			{
				if ( entries[i].id == id )
				{
					if ( authority <= entries[i].authority && !entries[i].forced || force )
					{
						entries[i].authority = authority;
						entries[i].forced = force;
						entries[i].time = 0.0f;
						return true;
					}
					else
						return false;
				}
			}
			// add new entry
			entries.resize( count + 1 );
			entries[count].id = id;
			entries[count].time = 0.0f;
			entries[count].forced = force;
			entries[count].authority = authority;
			return true;
		}
	
		int GetAuthority( ObjectId id )
		{
			const int count = entries.size();
			for ( int i = 0; i < count; ++i )
			{
				if ( entries[i].id == id )
				{
					assert( entries[i].authority >= 0 );
					assert( entries[i].authority < MaxPlayers );
					return entries[i].authority;
				}
			}
			return MaxPlayers;		// note: this represents "default" authority, any other player can take authority in this case
		}
	
		void RemoveAuthority( ObjectId id )
		{
			const int count = entries.size();
			for ( int i = 0; i < count; ++i )
			{
				if ( entries[i].id == id )
				{
					if ( i != count - 1 )
						entries[i] = entries[count-1];
					entries.resize( count - 1 );
					return;
				}
			}
		}

		void Update( float deltaTime, float authorityTimeout )
		{
			int count = entries.size();
			for ( int i = 0; i < count; )
			{
				entries[i].time += deltaTime;
				if ( entries[i].time >= authorityTimeout || entries[i].authority == MaxPlayers )
				{
					if ( i != count - 1 )
						entries[i] = entries[count-1];
					entries.resize( count - 1 );
					count--;
				}
				else
					++i;
			}
		}
	
		int GetEntryCount() const
		{
			return entries.size();
		}

	private:
	
		std::vector<AuthorityEntry> entries;
	};

	// --------------------------------------------------

	class InteractionManager
	{
	public:

		InteractionManager()
		{
			ClearInteractions();
		}

		void ClearInteractions()
		{
			interacting.clear();
		}
		
		void PrepInteractions( int maxActiveId )
		{
			interacting.resize( maxActiveId + 1 );
			for ( int i = 0; i <= maxActiveId; ++i )
				interacting[i] = false;
		}
		
		void WalkInteractions( int activeId, const InteractionPair * interactionPairs, int numInteractionPairs, std::vector<bool> & ignore )
		{
			assert( activeId >= 0 );
			assert( activeId < (int) interacting.size() );
			if ( interacting[activeId] )
				return;
			if ( ignore[activeId] )
				return;
			SetInteracting( activeId );
			for ( int i = 0; i < numInteractionPairs; ++i )
			{
				if ( interactionPairs[i].a == activeId )
					WalkInteractions( interactionPairs[i].b, interactionPairs, numInteractionPairs, ignore );
				if ( interactionPairs[i].b == activeId )
					WalkInteractions( interactionPairs[i].a, interactionPairs, numInteractionPairs, ignore );
			}
		}

		void SetInteracting( int activeId )
		{
			assert( activeId >= 0 );
			assert( activeId < (int) interacting.size() );
			interacting[activeId] = true;
		}

		bool IsInteracting( int activeId )
		{
			assert( activeId >= 0 );
			assert( activeId < (int) interacting.size() );
			return interacting[activeId];
		}
		
		int GetCount() const
		{
			return interacting.size();
		}
	
	private:
	
		std::vector<bool> interacting;
	};

	// --------------------------------------------------

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

	// --------------------------------------------------------------

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
	
		PacketQueue()
		{
			delay = 0.0f;
		}

		~PacketQueue()
		{
			Clear();
		}
	
		void Clear()
		{
			for ( int i = 0; i < (int) queue.size(); ++i )
				delete queue[i];
			queue.clear();
		}
	
		void QueuePacket( int sourceNodeId, int destinationNodeId, unsigned char * data, int bytes )
		{
			assert( bytes >= 0 );
			Packet * packet = new Packet();
			packet->timeInQueue = 0.0f;
			packet->sourceNodeId = sourceNodeId;
			packet->destinationNodeId = destinationNodeId;
			packet->data.resize( bytes );
			memcpy( &packet->data[0], data, bytes );
			queue.push_back( packet );
		}
	
		void SetDelay( float delay )
		{
			this->delay = delay;
		}

		void Update( float deltaTime )
		{
			for ( int i = 0; i < (int) queue.size(); ++i )
				queue[i]->timeInQueue += deltaTime;
		}
	
		Packet * PacketReadyToSend()
		{
			while ( queue.size() > 0 )
			{
				Packet * packet = queue[0];
				if ( packet->timeInQueue >= delay )
				{
					queue.erase( queue.begin() );
					return packet;			// important! it is your responsibility to delete the packet(!!!)
				}
				else
					break;
			}
			return NULL;
		}
	
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
		
		PrioritySet()
		{
		}
		
		void Clear()
		{
			entries.clear();
		}
		
		bool ObjectExists( ObjectId objectId ) const
		{
			for ( int i = 0; i < (int) entries.size(); ++i )
			{
				if ( entries[i].objectId == objectId )
					return true;
			}
			return false;
		}

		void AddObject( ObjectId objectId )
		{
			assert( !ObjectExists( objectId ) );
			const int size = entries.size();
			entries.resize( size + 1 );
			entries[size].objectId = objectId;
			entries[size].priority = 0.0f;
		}
		
		void RemoveObject( ObjectId objectId )
		{
			const int count = entries.size();
			assert( count > 0 );
			for ( int i = 0; i < count; ++i )
			{
				if ( entries[i].objectId == objectId )
				{
					entries[i].objectId = entries[count-1].objectId;
					entries[i].priority = entries[count-1].priority;
					entries.resize( count - 1 );
					return;
				}
			}
		}

		float GetPriorityAtIndex( int index ) const
		{
			assert( index >= 0 );
			assert( index < (int) entries.size() );
			return entries[index].priority;
		}
		
		void SetPriorityAtIndex( int index, float priority )
		{
			assert( index >= 0 );
			assert( index < (int) entries.size() );
			entries[index].priority = priority;
		}

		void SortObjects()
		{
			std::sort( entries.begin(), entries.end() );
		}
		
		ObjectId GetPriorityObject( int index ) const
		{
			return entries[index].objectId;
		}
		
		int GetObjectCount() const
		{
			return entries.size();
		}
		
	private:
		
		struct ObjectEntry
		{
			// todo: we could probably crunch these guys down into 32 bits...
			ActiveId objectId;
			float priority;
			bool operator < ( const ObjectEntry & other ) const
			{
				return priority > other.priority;
			}
		};

		std::vector<ObjectEntry> entries;
	};
	
	// helper functions for compression
	
	void CompressPosition( const math::Vector & position, uint64_t & compressed_position )
	{
		float x = position.x + 512.0f;
		float y = position.y + 512.0f;
		float z = position.z + 512.0f;

		const float delta_x = 1024.0f;
		const float delta_y = 1024.0f;
		const float delta_z = 1024.0f;

		const float normal_x = x / delta_x;
		const float normal_y = y / delta_y;
		const float normal_z = z / delta_z;

		uint64_t integer_x = math::floor( normal_x * 1024 * 1024 + 0.5f );
		uint64_t integer_y = math::floor( normal_y * 1024 * 1024 + 0.5f );
		uint64_t integer_z = math::floor( normal_z * 1024 * 1024 + 0.5f );

		integer_x &= ( 1 << 20 ) - 1;
		integer_y &= ( 1 << 20 ) - 1;
		integer_z &= ( 1 << 20 ) - 1;

		compressed_position = ( integer_x << 40 ) | ( integer_y << 20 ) | integer_z;
	}

	void DecompressPosition( uint64_t compressed_position, math::Vector & position )
	{
		uint64_t integer_x = ( compressed_position >> 40 ) & ( (1<<20) - 1 );
		uint64_t integer_y = ( compressed_position >> 20 ) & ( (1<<20) - 1 ); 
		uint64_t integer_z = ( compressed_position       ) & ( (1<<20) - 1 );

		float normal_x = integer_x / ( 1024.0f * 1024.0f );
		float normal_y = integer_y / ( 1024.0f * 1024.0f );
		float normal_z = integer_z / ( 1024.0f * 1024.0f );

		position.x = normal_x * 1024.0f - 512.0f;
		position.y = normal_y * 1024.0f - 512.0f;
		position.z = normal_z * 1024.0f - 512.0f;
	}

	void CompressOrientation( const math::Quaternion & orientation, uint32_t & compressed_orientation )
	{
 		uint32_t largest = 0;
		float a,b,c;
		a = 0;
		b = 0;
		c = 0;

		const float w = orientation.w;
		const float x = orientation.x;
		const float y = orientation.y;
		const float z = orientation.z;

		#ifdef DEBUG
		const float epsilon = 0.0001f;
		const float length_squared = w*w + x*x + y*y + z*z;
		assert( length_squared >= 1.0f - epsilon && length_squared <= 1.0f + epsilon );
		#endif

		const float abs_w = math::abs( w );
		const float abs_x = math::abs( x );
		const float abs_y = math::abs( y );
		const float abs_z = math::abs( z );

		float largest_value = abs_x;

		if ( abs_y > largest_value )
		{
			largest = 1;
			largest_value = abs_y;
		}

		if ( abs_z > largest_value )
		{
			largest = 2;
			largest_value = abs_z;
		}

		if ( abs_w > largest_value )
		{
			largest = 3;
			largest_value = abs_w;
		}

		switch ( largest )
		{
			case 0:
				if ( x >= 0 )
				{
					a = y;
					b = z;
					c = w;
				}
				else
				{
					a = -y;
					b = -z;
					c = -w;
				}
				break;

			case 1:
				if ( y >= 0 )
				{
					a = x;
					b = z;
					c = w;
				}
				else
				{
					a = -x;
					b = -z;
					c = -w;
				}
				break;

			case 2:
				if ( z >= 0 )
				{
					a = x;
					b = y;
					c = w;
				}
				else
				{
					a = -x;
					b = -y;
					c = -w;
				}
				break;

			case 3:
				if ( w >= 0 )
				{
					a = x;
					b = y;
					c = z;
				}
				else
				{
					a = -x;
					b = -y;
					c = -z;
				}
				break;

			default:
				assert( false );
		}

//		printf( "float: a = %f, b = %f, c = %f\n", a, b, c );

		const float minimum = - 1.0f / 1.414214f;		// note: 1.0f / sqrt(2)
		const float maximum = + 1.0f / 1.414214f;

		const float normal_a = ( a - minimum ) / ( maximum - minimum ); 
		const float normal_b = ( b - minimum ) / ( maximum - minimum );
		const float normal_c = ( c - minimum ) / ( maximum - minimum );

 		uint32_t integer_a = math::floor( normal_a * 1024.0f + 0.5f );
 		uint32_t integer_b = math::floor( normal_b * 1024.0f + 0.5f );
 		uint32_t integer_c = math::floor( normal_c * 1024.0f + 0.5f );

//		printf( "integer: a = %d, b = %d, c = %d, largest = %d\n", 
//			integer_a, integer_b, integer_c, largest );

		compressed_orientation = ( largest << 30 ) | ( integer_a << 20 ) | ( integer_b << 10 ) | integer_c;
	}

	void DecompressOrientation( uint32_t compressed_orientation, math::Quaternion & orientation )
	{
		uint32_t largest = compressed_orientation >> 30;
		uint32_t integer_a = ( compressed_orientation >> 20 ) & ( (1<<10) - 1 );
		uint32_t integer_b = ( compressed_orientation >> 10 ) & ( (1<<10) - 1 );
		uint32_t integer_c = ( compressed_orientation ) & ( (1<<10) - 1 );

//		printf( "---------\n" );

//		printf( "integer: a = %d, b = %d, c = %d, largest = %d\n", 
//			integer_a, integer_b, integer_c, largest );

		const float minimum = - 1.0f / 1.414214f;		// note: 1.0f / sqrt(2)
		const float maximum = + 1.0f / 1.414214f;

		const float a = integer_a / 1024.0f * ( maximum - minimum ) + minimum;
		const float b = integer_b / 1024.0f * ( maximum - minimum ) + minimum;
		const float c = integer_c / 1024.0f * ( maximum - minimum ) + minimum;

//		printf( "float: a = %f, b = %f, c = %f\n", a, b, c );

		switch ( largest )
		{
			case 0:
			{
				// (?) y z w

				orientation.y = a;
				orientation.z = b;
				orientation.w = c;
				orientation.x = math::sqrt( 1 - orientation.y*orientation.y 
				                              - orientation.z*orientation.z 
				                              - orientation.w*orientation.w );
			}
			break;

			case 1:
			{
				// x (?) z w

				orientation.x = a;
				orientation.z = b;
				orientation.w = c;
				orientation.y = math::sqrt( 1 - orientation.x*orientation.x 
				                              - orientation.z*orientation.z 
				                              - orientation.w*orientation.w );
			}
			break;

			case 2:
			{
				// x y (?) w

				orientation.x = a;
				orientation.y = b;
				orientation.w = c;
				orientation.z = math::sqrt( 1 - orientation.x*orientation.x 
				                              - orientation.y*orientation.y 
				                              - orientation.w*orientation.w );
			}
			break;

			case 3:
			{
				// x y z (?)

				orientation.x = a;
				orientation.y = b;
				orientation.z = c;
				orientation.w = math::sqrt( 1 - orientation.x*orientation.x 
				                              - orientation.y*orientation.y 
				                              - orientation.z*orientation.z );
			}
			break;

			default:
			{
				assert( false );
				orientation.w = 1.0f;
				orientation.x = 0.0f;
				orientation.y = 0.0f;
				orientation.z = 0.0f;
			}
		}

		orientation.normalize();
	}

/*
	void CompressOrientation( const math::Quaternion & orientation, uint32_t & compressed_orientation )
	{
		const float w = orientation.w;
		const float x = orientation.x;
		const float y = orientation.y;
		const float z = orientation.z;

		const float normal_w = ( w + 1 ) * 0.5f; 
		const float normal_x = ( x + 1 ) * 0.5f;
		const float normal_y = ( y + 1 ) * 0.5f;
		const float normal_z = ( z + 1 ) * 0.5f;

		unsigned int integer_w = math::floor( normal_w * 255.0f + 0.5f );
		unsigned int integer_x = math::floor( normal_x * 255.0f + 0.5f );
		unsigned int integer_y = math::floor( normal_y * 255.0f + 0.5f );
		unsigned int integer_z = math::floor( normal_z * 255.0f + 0.5f );

		compressed_orientation = ( integer_w ) << 24 | ( integer_x << 16 ) | ( integer_y << 8 ) | integer_z;
	}

	void DecompressOrientation( uint32_t compressed_orientation, math::Quaternion & orientation )
	{
		const unsigned int integer_w = compressed_orientation >> 24;
		const unsigned int integer_x = ( compressed_orientation >> 16 ) & 0xFF;
		const unsigned int integer_y = ( compressed_orientation >> 8 ) & 0xFF;
		const unsigned int integer_z = compressed_orientation & 0xFF;

		const float w = integer_w * 2.0f / 255.0f - 1.0f;
		const float x = integer_x * 2.0f / 255.0f - 1.0f;
		const float y = integer_y * 2.0f / 255.0f - 1.0f;
		const float z = integer_z * 2.0f / 255.0f - 1.0f;

		orientation = math::Quaternion( w, x, y, z );
		orientation.normalize();
	}
	*/
}

#endif
