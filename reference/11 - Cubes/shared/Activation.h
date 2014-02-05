/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef ACTIVATION_H
#define ACTIVATION_H

#include "Config.h"
#include "Mathematics.h"

namespace activation
{
	typedef uint32_t ObjectId;
	typedef uint32_t ActiveId;

	/*
		The activation system divides the world up into grid cells.
		This is the per-object entry for an object inside a cell.
	*/
	
	struct CellObject
	{
		uint32_t id : 20;
		uint32_t active : 1;
		uint32_t disabled : 1;
		uint32_t activeObjectIndex : 10;
		float x,y;
		#ifdef VALIDATION
 		int cellIndex;
		void Clear()
		{
			id = 0;
			active = 0;
			disabled = 0;
			activeObjectIndex = 0;
			cellIndex = -1;
			x = 0.0f;
			y = 0.0f;
		}
		#endif
	};

	/*
		Objects inside the player activation circle are activated.
		This is the activation system data per active object.
	*/
	
	struct ActiveObject
	{
 		uint32_t id : 20;
		uint32_t cellIndex : 20;
 		uint32_t cellObjectIndex : 12;
		uint32_t pendingDeactivation : 1;
		float pendingDeactivationTime;					// TODO - convert to n bits frame counter

		#ifdef VALIDATION
		void Clear()
		{
			id = 0;
			pendingDeactivation = 0;
			pendingDeactivationTime = 0;
			cellIndex = 0;
			cellObjectIndex = 0;
		}
		#endif
	};

	/*
		The set template is used by game code to maintain
		sets of objects. Objects are unordered and deletion
		is implemented by replacing the deleted item with the last.
	*/
	
	template <typename T> class Set
	{
	public:

		Set()
		{
			count = 0;
			size = 0;
			ids = NULL;
			objects = NULL;
		}

		~Set()
		{
			Free();
		}

		void Allocate( int initialSize )
		{
			assert( objects == NULL );
			assert( initialSize > 0 );
			ids = new uint32_t[initialSize];
			objects = new T[initialSize];
			size = initialSize;
			count = 0;
		}
		
		void Free()
		{
			delete [] objects;
			delete [] ids;
			objects = NULL;
			count = 0;
			size = 0;
		}

		void Clear()
		{
			count = 0;
		}

 		T & InsertObject( ObjectId id )
		{
			if ( count >= size )
				Grow();
			ids[count] = id;
			return objects[count++];
		}

		void DeleteObject( ObjectId id )
		{
			assert( count >= 1 );
			for ( int i = 0; i < count; ++i )
			{
				if ( objects[i].id == id )
				{
					DeleteObject( i );
					return;
				}
			}
			assert( false );
		}
		
		void DeleteObject( T * object )
		{
			int index = object - &objects[0];
			DeleteObject( index );
		}

		void DeleteObject( int index )
		{
			assert( count >= 1 );
			
			// delete object
			assert( index >= 0 );
			assert( index < count );
			int last = count - 1;
			if ( index != last )
			{
				ids[index] = ids[last];
				objects[index] = objects[last];
			}
			count--;
			if ( count < size/3 )
				Shrink();
		}

 		T & GetObject( int index )
		{
			if ( index >= count )
			{
				printf( "index = %d, count = %d\n", index, count );
			}
			assert( index >= 0 );
			assert( index < count );
			return objects[index];
		}

 		T * FindObject( ObjectId id )
		{
			for ( int i = 0; i < count; ++i )
				if ( ids[i] == id )
					return &objects[i];
			return NULL;
		}

	 	const T * FindObject( ObjectId id ) const
		{
			for ( int i = 0; i < count; ++i )
				if ( ids[i] == id )
					return &objects[i];
			return NULL;
		}

		int GetCount() const
		{
			return count;
		}
		
		int GetSize() const
		{
			return size;
		}
		
		int GetBytes() const
		{
			return sizeof(T) * size;
		}
		
	protected:

		void Grow()
		{
//			printf( "grow %d -> %d\n", size, size*2 );
			size *= 2;
			uint32_t * oldIds = ids;
			T * oldObjects = objects;
			ids = new uint32_t[size];
			objects = new T[size];
			memcpy( &ids[0], &oldIds[0], sizeof(int)*count );
			memcpy( &objects[0], &oldObjects[0], sizeof(T)*count );
			delete[] oldIds;
			delete[] oldObjects;
		}

		void Shrink()
		{
//			printf( "shrink %d -> %d\n", size, size / 2 );
			size /= 2;
			assert( count <= size );
			assert( size >= 1 );
			uint32_t * oldIds = ids;
			T * oldObjects = objects;
 			ids = new uint32_t[size];
			objects = new T[size];
			memcpy( &ids[0], &oldIds[0], sizeof(int)*count );
			memcpy( &objects[0], &oldObjects[0], sizeof(T)*count );
			delete[] oldIds;
			delete[] oldObjects;
		}

		int count;
		int size;
		uint32_t * ids;
		T * objects;
	};

	/*
		A set of cell objects.
		Special handling is required when deleting an object
		to keep the active object "cellObjectIndex" up to date
		when the last item is moved into the deleted object slot.
	*/
	
	class CellObjectSet : public Set<CellObject>
	{
	public:

		void DeleteObject( ActiveObject * activeObjects, ObjectId id );

		void DeleteObject( ActiveObject * activeObjects, CellObject & cellObject );
		
		const CellObject * GetObjectArray() const
		{
			return &objects[0];
		}
		
	private:
		
		void DeleteObject( ObjectId id );
		void DeleteObject( CellObject * object );
	};
	
	/*
		Each cell contains a number of objects.
		By keeping track of which objects are in a cell,
		we can activate/deactivate objects with cost proportional
		to the number of grid cells overlapping the activation circle.
	*/
	
	struct Cell
	{
		#ifdef DEBUG
		int index;
		#endif
		int ix,iy;
		float x1,y1,x2,y2;
		CellObjectSet objects;

	#ifdef DEBUG

		static void ValidateCellObject( Cell * cells, ActiveObject * activeObjects, const CellObject & cellObject );
		static void ValidateActiveObject( Cell * cells, ActiveObject * activeObjects, const ActiveObject & activeObject );

	#endif
	
		void Initialize( int initialObjectCount )
		{
			objects.Allocate( initialObjectCount );
		}

		CellObject & InsertObject( Cell * cells, ActiveObject * activeObjects, ObjectId id, float x, float y );

		void DeleteObject( ActiveObject * activeObjects, ObjectId id )
		{
			objects.DeleteObject( activeObjects, id );
		}

		void DeleteObject( ActiveObject * activeObjects, CellObject & cellObject )
		{
			objects.DeleteObject( activeObjects, cellObject );
		}

		CellObject & GetObject( int index )
		{
			return objects.GetObject( index );
		}

		CellObject * FindObject( ObjectId id )
		{
			return objects.FindObject( id );
		}

	 	const CellObject * FindObject( ObjectId id ) const
		{
			return objects.FindObject( id );
		}

		int GetObjectCount()
		{
			return objects.GetCount();
		}
		
		int GetCellObjectIndex( const CellObject & cellObject ) const
		{
			int index = (int) ( &cellObject - objects.GetObjectArray() );
			assert( index >= 0 );
			assert( index < objects.GetCount() );
			return index;
		}
	};

	/*
		Set of active objects.
		We use this to store the set of active objects.
		The set of active objects is a subset of all objects
		in the world corresponding to the objects which are
		currently inside the activation circle of the player.
	*/
	
	class ActiveObjectSet : public Set<ActiveObject>
	{
	public:

		void DeleteObject( Cell * cells, ObjectId id );

		void DeleteObject( Cell * cells, ActiveObject & activeObject );
		
		ActiveObject * GetObjectArray()
		{
			return &objects[0];
		}
		
		int GetActiveObjectIndex( ActiveObject & activeObject )
		{
			int index = (int) ( &activeObject - &objects[0] );
			assert( index >= 0 );
			assert( index < count );
			return index;
		}

	private:
		
		void DeleteObject( ObjectId id );
		void DeleteObject( ActiveObject * object );
	};
	
	/*
		Activation events are sent when objects activate or deactivate. 
		They let an external system track object activation and deactivation 
		so it can perform it's own activation functionality.
	*/	
	
	struct Event
	{
		enum Type { Activate, Deactivate };
		uint32_t type : 1;
		uint32_t id : 31;
	};

	/*
		The activation system tracks which objects are in each grid cell,
		and maintains the set of active objects for the local player.
	*/
	
	class ActivationSystem
	{
	public:

		typedef std::vector<Event> Events;

		ActivationSystem( int maxObjects, float radius, int width, int height, float size, int initialObjectsPerCell, int initialActiveObjects, float deactivationTime = 0.0f );
		~ActivationSystem();

		void SetEnabled( bool enabled );
		
		void Update( float deltaTime );

		void MoveActivationPoint( float new_x, float new_y );
		
		void InsertObject( ObjectId id, float x, float y );

		// IMPORTANT: this is a slow function, use the active or database versions instead if you can
		void MoveObject( ObjectId id, float new_x, float new_y, bool warp = false );
		
		// these are much faster
		void MoveActiveObject( int activeIndex, float new_x, float new_y, bool warp );
 		void MoveDatabaseObject( ObjectId id, float new_x, float new_y );
		
		ActiveObject & ActivateObject( CellObject & cellObject, Cell & cell );

		void DeactivateObject( ActiveObject & activeObject );
		
		void EnableObject( ObjectId objectId );
		void DisableObject( ObjectId objectId );

		void QueueObjectForDeactivation( ActiveObject & activeObject, bool immediate = false );

		int GetEventCount();

		const Event & GetEvent( int index );

		void ClearEvents();

		void Validate();

	public:
	
		float GetBoundX() const
		{
			return bound_x;
		}

		float GetBoundY() const
		{
			return bound_y;
		}

		void Clamp( math::Vector & position )
		{
			position.x = math::clamp( position.x, -bound_x, +bound_x );
			position.y = math::clamp( position.y, -bound_y, +bound_y );
		}
		
		float GetX() const
		{
			return activation_x;
		}

		float GetY() const
		{
			return activation_y;
		}

		int GetActiveCount() const
		{
			return active_objects.GetCount();
		}

		bool IsActive( ObjectId id ) const
		{
			return active_objects.FindObject( id ) != NULL;
		}
		
		bool IsPendingDeactivation( int activeIndex )
		{
			ActiveObject & activeObject = active_objects.GetObject( activeIndex );
			return activeObject.pendingDeactivation;
		}

		Cell * GetCellAtIndex( int ix, int iy )
		{
			assert( ix >= 0 );
			assert( iy >= 0 );
			assert( ix < width );
			assert( iy < height );
			int index = ix + iy * width;
			return &cells[index];
		}

		int GetWidth() const
		{
			return width;
		}

		int GetHeight() const
		{
			return height;
		}

		float GetCellSize() const
		{
			return size;
		}

		bool IsEnabled() const
		{
			return enabled;
		}
		
		int GetBytes() const
		{
			return sizeof( ActivationSystem ) + width * height * ( sizeof( Cell ) + sizeof( CellObject ) * initial_objects_per_cell ) + maxObjects * sizeof( int );
		}

        void DumpInfo()
        {
            printf( "------------------------------------\n" );
            printf( "activation system:\n" );
            printf( "cell = %d bytes\n", (int) sizeof( Cell ) );
            printf( "cell object = %d bytes\n", (int) sizeof( CellObject ) );
            printf( "cell array = %d bytes\n", (int) ( width * height * sizeof( Cell ) ) );
            printf( "cell objects = %d bytes\n", (int) ( width * height * sizeof( CellObject ) * initial_objects_per_cell ) );
            printf( "initial objects per-cell = %d\n", initial_objects_per_cell );
            printf( "id to cell array = %d bytes\n", (int) ( maxObjects * sizeof( int ) ) );
            printf( "------------------------------------\n" );
        }

	protected:

		void ActivateObjectsInsideCircle();

		void DeactivateAllObjects();

		Cell * CellAtPosition( float x, float y )
		{
			assert( x >= -bound_x );
			assert( x <= +bound_x );
			assert( y >= -bound_y );
			assert( y <= +bound_y );
			int ix = math::clamp( (int) math::floor( ( x + bound_x ) * inverse_size ), 0, width - 1 );
			int iy = math::clamp( (int) math::floor( ( y + bound_y ) * inverse_size ), 0, height - 1 );
			return &cells[iy*width+ix];
		}

		void QueueActivationEvent( ObjectId id )
		{
			Event event;
			event.type = Event::Activate;
			event.id = id;
			activation_events.push_back( event );
		}

		void QueueDeactivationEvent( ObjectId id )
		{
			Event event;
			event.type = Event::Deactivate;
			event.id = id;
			activation_events.push_back( event );
		}

		bool active;
		bool enabled;
		bool enabled_last_frame;
		int width;
		int height;
		int maxObjects;
		int initial_objects_per_cell;
		float activation_x;
		float activation_y;
		float activation_radius;
		float activation_radius_squared;
		float size;
		float deactivationTime;
		float inverse_size;
		float bound_x;
		float bound_y;
		Cell * cells;
 		int * idToCellIndex;
		Events activation_events;
		ActiveObjectSet active_objects;
	};
}

#endif
