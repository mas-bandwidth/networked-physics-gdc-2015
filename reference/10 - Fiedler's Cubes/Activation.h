/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef ACTIVATION_H
#define ACTIVATION_H

#include "Config.h"
#include "Mathematics.h"
#include <vector>

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
		uint32_t activeObjectIndex : 10;
		float x,y;
		#ifdef DEBUG
 		int cellIndex;
		void Clear()
		{
			id = 0;
			active = 0;
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
 		uint32_t id : 31;
		uint32_t pendingDeactivation : 1;
		float pendingDeactivationTime;					// todo: convert to 4 bits frame counter?
		int cellIndex;									// todo: only need 20bits or so...
		int cellObjectIndex;							// todo: this guy can be only 10 bits or so (1024 max active)
		#ifdef DEBUG
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
			objects = new T[initialSize];
			size = initialSize;
			count = 0;
		}
		
		void Free()
		{
			delete[] objects;
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
			return objects[count++];
		}

		void DeleteObject( ObjectId id )
		{
			assert( count >= 1 );
			for ( int i = 0; i < count; ++i )
			{
				if ( objects[i].id == id )
				{
					DeleteObject( &objects[i] );
					return;
				}
			}
			assert( false );
		}

		void DeleteObject( T & object )
		{
			assert( count >= 1 );
			int i = (int) ( &object - &objects[0] );
			assert( i >= 0 );
			assert( i < count );
			int last = count - 1;
			if ( i != last )
				objects[i] = objects[last];
			count--;
			if ( count < size/3 )
				Shrink();
		}

 		T & GetObject( int index )
		{
			assert( index >= 0 );
			assert( index < count );
			return objects[index];
		}

 		T * FindObject( ObjectId id )
		{
			assert( count >= 0 );
			for ( int i = 0; i < count; ++i )
				if ( objects[i].id == id )
					return &objects[i];
			return NULL;
		}

	 	const T * FindObject( ObjectId id ) const
		{
			assert( count >= 0 );
			for ( int i = 0; i < count; ++i )
				if ( objects[i].id == id )
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
			size *= 2;
			T * oldObjects = objects;
			objects = new T[size];
			memcpy( &objects[0], &oldObjects[0], sizeof(T)*count );
			delete[] oldObjects;
		}

		void Shrink()
		{
			size >>= 1;
			assert( count <= size );
			T * oldObjects = objects;
			objects = new T[size];
			memcpy( &objects[0], &oldObjects[0], sizeof(T)*count );
			delete[] oldObjects;
		}

		int count;
		int size;
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

		void DeleteObject( ActiveObject * activeObjects, ObjectId id )
		{
			assert( count >= 1 );
			for ( int i = 0; i < count; ++i )
			{
				if ( objects[i].id == id )
				{
					DeleteObject( activeObjects, objects[i] );
					return;
				}
			}
			assert( false );
		}

		void DeleteObject( ActiveObject * activeObjects, CellObject & cellObject )
		{			
			assert( count >= 1 );
			CellObject * cellObjects = &objects[0];
			int i = (int) ( &cellObject - cellObjects );
			assert( i >= 0 );
			assert( i < count );
			int last = count - 1;
			if ( i != last )
			{
				cellObjects[i] = cellObjects[last];
				if ( cellObjects[i].active )
				{
					const int activeObjectIndex = cellObjects[i].activeObjectIndex;
					activeObjects[activeObjectIndex].cellObjectIndex = (int) ( &cellObjects[i] - cellObjects );
				}					
			}
			#ifdef DEBUG
			cellObjects[last].Clear();
			#endif
			count--;
			if ( count < size/3 )
				Shrink();
		}
		
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

		static void ValidateCellObject( Cell * cells, ActiveObject * activeObjects, const CellObject & cellObject )
		{
			assert( cellObject.id != 0 );
			assert( cellObject.cellIndex != -1 );
			if ( cellObject.active )
			{
				ActiveObject & activeObject = activeObjects[cellObject.activeObjectIndex];
				assert( activeObject.id == cellObject.id );
				assert( activeObject.cellIndex == cellObject.cellIndex );
			}
		}

		static void ValidateActiveObject( Cell * cells, ActiveObject * activeObjects, const ActiveObject & activeObject )
		{
			assert( activeObject.id != 0 );
			assert( activeObject.cellIndex != -1 );
			Cell & cell = cells[activeObject.cellIndex];
			CellObject & cellObject = cell.GetObject( activeObject.cellObjectIndex );
			assert( cellObject.id == activeObject.id );
			assert( cellObject.active == 1 );
			assert( cellObject.activeObjectIndex == (int) ( &activeObject - activeObjects ) );
			assert( cellObject.cellIndex == activeObject.cellIndex );
		}

	#endif
	
		void Initialize( int initialObjectCount )
		{
			objects.Allocate( initialObjectCount );
		}

		CellObject & InsertObject( Cell * cells, ActiveObject * activeObjects, ObjectId id, float x, float y )
		{
			#ifdef DEBUG
			const float epsilon = 0.0001f;
			assert( x >= x1 - epsilon );
			assert( y >= y1 - epsilon );
			assert( x < x2 + epsilon );
			assert( y < y2 + epsilon );
			#endif
			CellObject & cellObject = objects.InsertObject( id );
			cellObject.id = id;
			cellObject.x = x;
			cellObject.y = y;
			cellObject.active = 0;
			cellObject.activeObjectIndex = 0;
			#ifdef DEBUG
			cellObject.cellIndex = index;
			ValidateCellObject( cells, activeObjects, cellObject );
			#endif
			return cellObject;
		}

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

		void DeleteObject( Cell * cells, ObjectId id )
		{
			assert( count >= 1 );
			for ( int i = 0; i < count; ++i )
			{
				if ( objects[i].id == id )
				{
					DeleteObject( cells, objects[i] );
					return;
				}
			}
			assert( false );
		}

		void DeleteObject( Cell * cells, ActiveObject & activeObject )
		{
			assert( count >= 1 );
			ActiveObject * activeObjects = &objects[0];
			int i = (int) ( &activeObject - activeObjects );
			assert( i >= 0 );
			assert( i < count );
			int last = count - 1;
			if ( i != last )
			{
				activeObjects[i] = activeObjects[last];
				// note: we must patch up the cell object active id to match new index
				Cell & cell = cells[activeObjects[i].cellIndex];
				CellObject & cellObject = cell.GetObject( activeObjects[i].cellObjectIndex );
				assert( cellObject.id == activeObjects[i].id );
				assert( cellObject.activeObjectIndex == last );
				cellObject.activeObjectIndex = i;
			}
			#ifdef DEBUG
			activeObjects[last].Clear();
			#endif
			count--;
			if ( count < size/3 )
				Shrink();
		}
		
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

		ActivationSystem( int maxObjects, float radius, int width, int height, float size, int initialObjectsPerCell, int initialActiveObjects, float deactivationTime = 0.0f )
		{
			assert( maxObjects > 0 );
			assert( width > 0 );
			assert( height >  0 );
			assert( size > 0.0f );
			this->maxObjects = maxObjects;
			this->activation_x = 0.0f;
			this->activation_y = 0.0f;
			this->activation_radius = radius;
			this->activation_radius_squared = radius * radius;
			this->width = width;
			this->height = height;
			this->size = size;
			this->deactivationTime = deactivationTime;
			this->inverse_size = 1.0f / size;
			this->bound_x = width / 2 * size;
			this->bound_y = height / 2 * size;
			cells = new Cell[width*height];
			assert( cells );
			int index = 0;
			float fy = -bound_y;
			for ( int y = 0; y < height; ++y )
			{
				float fx = -bound_x;
				for ( int x = 0; x < width; ++x )
				{
					Cell & cell = cells[index];
					#ifdef DEBUG
					cell.index = index;
					#endif
					cell.ix = x;
					cell.iy = y;
					cell.x1 = fx;
					cell.y1 = fy;
					cell.x2 = fx + size;
					cell.y2 = fy + size;
					cell.Initialize( initialObjectsPerCell );
					fx += size;
					++index;
				}
				fy += size;
			}
			idToCellIndex = new int[maxObjects]; 
			#ifdef DEBUG
			for ( int i = 0; i < maxObjects; ++i )
				idToCellIndex[i] = -1;
			#endif
			enabled = true;
			enabled_last_frame = false;
			active_objects.Allocate( initialActiveObjects );
			initial_objects_per_cell = initialObjectsPerCell;
		}

		~ActivationSystem()
		{
			delete[] idToCellIndex;
			delete[] cells;
		}

		void SetEnabled( bool enabled )
		{
			this->enabled = enabled;
		}

		void Update( float deltaTime )
		{
			if ( !enabled_last_frame && enabled )
				ActivateObjectsInsideCircle();
			else if ( enabled_last_frame && !enabled )
				DeactivateAllObjects();
			enabled_last_frame = enabled;
			int i = 0;
			while ( i < active_objects.GetCount() )
			{
				ActiveObject & activeObject = active_objects.GetObject( i );
				#ifdef DEBUG
				Cell & cell = cells[activeObject.cellIndex];
				CellObject & cellObject = cell.GetObject( activeObject.cellObjectIndex );
				assert( cellObject.active );
				#endif
				if ( activeObject.pendingDeactivation )
				{
					activeObject.pendingDeactivationTime += deltaTime;
					if ( activeObject.pendingDeactivationTime >= deactivationTime )
						DeactivateObject( activeObject );
					else
						++i;
				}
				else
					++i;
			}
		}

	protected:

		void ActivateObjectsInsideCircle()
		{
			// determine grid cells to inspect...
			int ix1 = (int) math::floor( ( activation_x - activation_radius + bound_x ) * inverse_size ) - 1;
			int ix2 = (int) math::floor( ( activation_x + activation_radius + bound_x ) * inverse_size ) + 1;
			int iy1 = (int) math::floor( ( activation_y - activation_radius + bound_y ) * inverse_size ) - 1;
			int iy2 = (int) math::floor( ( activation_y + activation_radius + bound_y ) * inverse_size ) + 1;
			ix1 = math::clamp( ix1, 0, width - 1 );
			ix2 = math::clamp( ix2, 0, width - 1 );
			iy1 = math::clamp( iy1, 0, height - 1 );
			iy2 = math::clamp( iy2, 0, height - 1 );
			// iterate over grid cells and activate objects inside activation circle
			int index = iy1 * width + ix1;
			int stride = width - ( ix2 - ix1 + 1 );
			for ( int iy = iy1; iy <= iy2; ++iy )
			{
				assert( iy >= 0 );
				assert( iy < height );
				for ( int ix = ix1; ix <= ix2; ++ix )
				{
					assert( ix >= 0 );
					assert( ix < width );
					assert( index == iy * width + ix );
					Cell & cell = cells[index++];
					for ( int i = 0; i < cell.objects.GetCount(); ++i )
					{
						CellObject & cellObject = cell.objects.GetObject( i );
						const float dx = cellObject.x - activation_x;
						const float dy = cellObject.y - activation_y;
						const float distanceSquared = dx*dx + dy*dy;
						if ( distanceSquared < activation_radius_squared )
						{
							if ( !cellObject.active )
							{
								ActivateObject( cellObject, cell );
							}
							else
							{
								ActiveObject & activeObject = active_objects.GetObject( cellObject.activeObjectIndex );
								activeObject.pendingDeactivation = false;
							}
						}
					}
				}
				index += stride;
			}
			Validate();
		}

		void DeactivateAllObjects()
		{
			for ( int i = 0; i < active_objects.GetCount(); ++i )
			{
				ActiveObject & activeObject = active_objects.GetObject( i );
				if ( !activeObject.pendingDeactivation )
					QueueObjectForDeactivation( activeObject );
			}
		}

	public:

		void MoveActivationPoint( float new_x, float new_y )
		{
			Validate();
			// clamp in bounds
			new_x = math::clamp( new_x, -bound_x, +bound_x );
			new_y = math::clamp( new_y, -bound_y, +bound_y );
			// if we are not enabled, don't do anything...
			if ( !enabled )
				return;
			// dont do anything if position has not changed (unless we are activating)
			const float old_x = activation_x;
			const float old_y = activation_y;
			if ( new_x == old_x && new_y == old_y )
				return;
			// if there is no overlap between new and old,
			// then we can take a shortcut and just deactivate old circle
			// and activate the new circle...
			if ( math::abs( new_x - old_x ) > activation_radius || math::abs( new_y - old_y ) > activation_radius )
			{
				DeactivateAllObjects();
				activation_x = new_x;
				activation_y = new_y;
				ActivateObjectsInsideCircle();
				Validate();
				return;
			}
			// new and old activation regions overlap
			// first, we determine which grid cells to inspect...
			int ix1,ix2;
			if ( new_x > old_x )
			{
				ix1 = (int) math::floor( ( old_x - activation_radius + bound_x ) * inverse_size ) - 1;
				ix2 = (int) math::floor( ( new_x + activation_radius + bound_x ) * inverse_size ) + 1;
			}
			else
			{
				ix1 = (int) math::floor( ( new_x - activation_radius + bound_x ) * inverse_size ) - 1;
				ix2 = (int) math::floor( ( old_x + activation_radius + bound_x ) * inverse_size ) + 1;
			}
			int iy1,iy2;
			if ( new_y > old_y )
			{
				iy1 = (int) math::floor( ( old_y - activation_radius + bound_y ) * inverse_size ) - 1;
				iy2 = (int) math::floor( ( new_y + activation_radius + bound_y ) * inverse_size ) + 1;
			}
			else
			{
				iy1 = (int) math::floor( ( new_y - activation_radius + bound_y ) * inverse_size ) - 1;
				iy2 = (int) math::floor( ( old_y + activation_radius + bound_y ) * inverse_size ) + 1;
			}
			ix1 = math::clamp( ix1, 0, width - 1 );
			ix2 = math::clamp( ix2, 0, width - 1 );
			iy1 = math::clamp( iy1, 0, height - 1 );
			iy2 = math::clamp( iy2, 0, height - 1 );
			// iterate over grid cells and activate/deactivate objects
			int index = iy1 * width + ix1;
			int stride = width - ( ix2 - ix1 + 1 );
			for ( int iy = iy1; iy <= iy2; ++iy )
			{
				assert( iy >= 0 );
				assert( iy < height );
				for ( int ix = ix1; ix <= ix2; ++ix )
				{
					assert( ix >= 0 );
					assert( ix < width );
					assert( index == iy * width + ix );
					Cell & cell = cells[index++];
					for ( int i = 0; i < cell.objects.GetCount(); ++i )
					{
						CellObject & cellObject = cell.objects.GetObject( i );
						const float dx = cellObject.x - new_x;
						const float dy = cellObject.y - new_y;
						const float distanceSquared = dx*dx + dy*dy;
						if ( distanceSquared < activation_radius_squared )
						{
							if ( !cellObject.active )
							{
								ActivateObject( cellObject, cell );
							}
							else
							{
								ActiveObject & activeObject = active_objects.GetObject( cellObject.activeObjectIndex );
								activeObject.pendingDeactivation = false;						
							}
						}
						else if ( cellObject.active )
						{
							ActiveObject & activeObject = active_objects.GetObject( cellObject.activeObjectIndex );
							if ( !activeObject.pendingDeactivation )
								QueueObjectForDeactivation( activeObject );
						}
					}
				}
				index += stride;
			}
			// update position
			activation_x = new_x;
			activation_y = new_y;
			Validate();
		}
		
		void InsertObject( ObjectId id, float x, float y )
		{
			assert( x >= - bound_x );
			assert( x <= + bound_x );
			assert( y >= - bound_y );
			assert( y <= + bound_y );
			Cell * cell = CellAtPosition( x, y );
			assert( cell );
			cell->InsertObject( cells, active_objects.GetObjectArray(), id, x, y );
			assert( idToCellIndex[id] == -1 );
			idToCellIndex[id] = (int) ( cell - &cells[0] );
		}

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
		
		void MoveObject( ObjectId id, float new_x, float new_y, bool warp = false )
		{
			// clamp the new position within bounds
			new_x = math::clamp( new_x, -bound_x, +bound_x );
			new_y = math::clamp( new_y, -bound_y, +bound_y );

			// gather all of the data we need about this object
			Cell * currentCell = NULL;
			CellObject * cellObject = NULL;
			ActiveObject * activeObject = active_objects.FindObject( id );
			if ( activeObject )
			{
				// active object
				currentCell = &cells[activeObject->cellIndex];
				cellObject = currentCell->FindObject( id );
				assert( cellObject );
				#ifdef DEBUG
				Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), *cellObject );
				Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), *activeObject );
				#endif
			}
			else
			{
				// inactive object
				currentCell = &cells[idToCellIndex[id]];
				cellObject = currentCell->FindObject( id );
				assert( cellObject );
				#ifdef DEBUG
				Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), *cellObject );
				#endif
			}
			
			// move the object, updating the current cell if necessary
			Cell * newCell = CellAtPosition( new_x, new_y );
			assert( newCell );
			if ( currentCell == newCell )
			{
				// common case: same cell
				cellObject->x = new_x;
				cellObject->y = new_y;
			}
			else
			{
				// remove from current cell
				currentCell->DeleteObject( active_objects.GetObjectArray(), *cellObject );

				// add to new cell
				currentCell = newCell;
 				cellObject = &currentCell->InsertObject( cells, active_objects.GetObjectArray(), id, new_x, new_y );
				idToCellIndex[id] = (int) ( currentCell - cells );

				// update active object
				if ( activeObject )
				{
					cellObject->active = 1;
					cellObject->activeObjectIndex = active_objects.GetActiveObjectIndex( *activeObject );
					activeObject->cellIndex = (int) ( currentCell - cells );
					activeObject->cellObjectIndex = currentCell->GetCellObjectIndex( *cellObject );
				}
			}
			
			#ifdef DEBUG
			Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), *cellObject );
			if ( activeObject )
				Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), *activeObject );
			#endif

			// see if the object needs to be activated or deactivated
			const float dx = new_x - activation_x;
			const float dy = new_y - activation_y;
			const float distanceSquared = dx*dx + dy*dy;
			if ( activeObject )
			{
				// active: does it need to be deactivated?
				if ( distanceSquared > activation_radius_squared )
				{
					if ( !activeObject->pendingDeactivation )
						QueueObjectForDeactivation( *activeObject, warp );
				}
				else
					activeObject->pendingDeactivation = 0;
			}
			else
			{
				// inactive: does it need to be activated?
				if ( distanceSquared <= activation_radius_squared )
					activeObject = &ActivateObject( *cellObject, *currentCell );
			}
			
			#ifdef DEBUG
			Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), *cellObject );
			if ( activeObject )
				Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), *activeObject );
			#endif
		}
		
		ActiveObject & ActivateObject( CellObject & cellObject, Cell & cell )
		{
			assert( !cellObject.active );
			#ifdef DEBUG
			Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), cellObject );
			#endif
			ActiveObject & activeObject = active_objects.InsertObject( cellObject.id );
			activeObject.id = cellObject.id;
			activeObject.cellIndex = (int) ( &cell - cells );
			activeObject.cellObjectIndex = cell.GetCellObjectIndex( cellObject );
			activeObject.pendingDeactivation = false;
			cellObject.active = 1;
			cellObject.activeObjectIndex = active_objects.GetActiveObjectIndex( activeObject );
			#ifdef DEBUG
			Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), activeObject );
			#endif
			QueueActivationEvent( cellObject.id );
			return activeObject;
		}

		void DeactivateObject( ActiveObject & activeObject )
		{
			#ifdef DEBUG
			Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), activeObject );
			#endif
			Cell & cell = cells[activeObject.cellIndex];
			CellObject & cellObject = cell.GetObject( activeObject.cellObjectIndex );
			cellObject.active = 0;
			cellObject.activeObjectIndex = 0;
			active_objects.DeleteObject( cells, activeObject );
			#ifdef DEBUG
			Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), cellObject );
			#endif
			QueueDeactivationEvent( cellObject.id );
		}

		void QueueObjectForDeactivation( ActiveObject & activeObject, bool warp = false )
		{
			assert( !activeObject.pendingDeactivation );
			activeObject.pendingDeactivation = true;
			activeObject.pendingDeactivationTime = warp ? deactivationTime : 0.0f;
		}

		void DeleteObject( ObjectId id, float x, float y )
		{
			// not implemented
			assert( false );
		}

		int GetEventCount()
		{
			return activation_events.size();
		}

		const Event & GetEvent( int index )
		{
			assert( index >= 0 );
			assert( index < (int) activation_events.size() );
			return activation_events[index];
		}

		void ClearEvents()
		{
			activation_events.clear();
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

		bool IsPendingDeactivation( ObjectId id ) const
		{
			const ActiveObject * object = active_objects.FindObject( id );
			return object && object->pendingDeactivation;
		}
		
		void Validate()
		{
			#if defined( DEBUG ) && defined( VALIDATE )
			#ifdef SLOW_VALIDATION
			const int size = width*height;
			for ( int i = 0; i < size; ++i )
			{
				Cell & cell = cells[i];
				for ( int j = 0; j < cell.GetObjectCount(); ++j )
				{
					CellObject & cellObject = cell.GetObject(j);
					Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), cellObject );
				}
			}
			#endif
			for ( int i = 0; i < active_objects.GetCount(); ++i )
			{
				ActiveObject & activeObject = active_objects.GetObject(i);
				Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), activeObject );
				Cell & cell = cells[activeObject.cellIndex];
				CellObject & cellObject = cell.GetObject( activeObject.cellObjectIndex );
				const float dx = cellObject.x - activation_x;
				const float dy = cellObject.y - activation_y;
				const float distanceSquared = dx*dx + dy*dy;
				assert( !activeObject.pendingDeactivation && distanceSquared <= activation_radius_squared + 0.001f ||
				         activeObject.pendingDeactivation && distanceSquared >= activation_radius_squared - 0.001f );
			}
			#endif
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

	private:

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
