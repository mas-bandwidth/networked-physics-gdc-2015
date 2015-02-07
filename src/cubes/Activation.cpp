/*
	Networked Physics Demo
	Copyright © 2008-2015 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "Activation.h"

namespace activation
{
	void CellObjectSet::DeleteObject( ActiveObject * activeObjects, ObjectId id )
	{
		assert( count >= 1 );
		for ( int i = 0; i < count; ++i )
		{
			if ( ids[i] == id )
			{
				DeleteObject( activeObjects, objects[i] );
				return;
			}
		}
		assert( false );
	}

	void CellObjectSet::DeleteObject( ActiveObject * activeObjects, CellObject & cellObject )
	{			
		assert( count >= 1 );
		CellObject * cellObjects = &objects[0];
		int i = (int) ( &cellObject - cellObjects );
		assert( i >= 0 );
		assert( i < count );
		int last = count - 1;
		if ( i != last )
		{
			ids[i] = ids[last];
			cellObjects[i] = cellObjects[last];
			if ( cellObjects[i].active )
			{
				const int activeObjectIndex = cellObjects[i].activeObjectIndex;
				activeObjects[activeObjectIndex].cellObjectIndex = (int) ( &cellObjects[i] - cellObjects );
			}					
		}
		count--;
		if ( count < size/3 )
			Shrink();
	}

#ifdef VALIDATION

	void Cell::ValidateCellObject( Cell * cells, ActiveObject * activeObjects, const CellObject & cellObject )
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

	void Cell::ValidateActiveObject( Cell * cells, ActiveObject * activeObjects, const ActiveObject & activeObject )
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
	
	CellObject & Cell::InsertObject( Cell * cells, ActiveObject * activeObjects, ObjectId id, float x, float y )
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
		cellObject.disabled = 0;
		cellObject.activeObjectIndex = 0;
		#ifdef VALIDATION
		cellObject.cellIndex = index;
		ValidateCellObject( cells, activeObjects, cellObject );
		#endif
		return cellObject;
	}

	void ActiveObjectSet::DeleteObject( Cell * cells, ObjectId id )
	{
		assert( count >= 1 );
		for ( int i = 0; i < count; ++i )
		{
			if ( ids[i] == id )
			{
				DeleteObject( cells, objects[i] );
				return;
			}
		}
		assert( false );
	}

	void ActiveObjectSet::DeleteObject( Cell * cells, ActiveObject & activeObject )
	{
		assert( count >= 1 );
		ActiveObject * activeObjects = &objects[0];
		int i = (int) ( &activeObject - activeObjects );
		assert( i >= 0 );
		assert( i < count );
		int last = count - 1;
		if ( i != last )
		{
			ids[i] = ids[last];
			activeObjects[i] = activeObjects[last];
			// IMPORTANT: we must patch up the cell object active id to match new index
			Cell & cell = cells[activeObjects[i].cellIndex];
			CellObject & cellObject = cell.GetObject( activeObjects[i].cellObjectIndex );
			assert( cellObject.id == activeObjects[i].id );
			assert( cellObject.activeObjectIndex == last );
			cellObject.activeObjectIndex = i;
		}
		count--;
		if ( count < size/3 )
			Shrink();
	}

	ActivationSystem::ActivationSystem( int maxObjects, float radius, int width, int height, float size, int initialObjectsPerCell, int initialActiveObjects, float deactivationTime )
	{
		/*
		printf( "max objects is %d\n", maxObjects );
		printf( "cell grid is %d x %d\n", width, height );
		printf( "cell size is %.1f meters squared\n", size );
		printf( "%d objects per-cell initially\n", initialObjectsPerCell );
		*/
		
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

	ActivationSystem::~ActivationSystem()
	{
		delete[] idToCellIndex;
		delete[] cells;
	}

	void ActivationSystem::SetEnabled( bool enabled )
	{
		this->enabled = enabled;
	}

	void ActivationSystem::Update( float deltaTime )
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

	void ActivationSystem::ActivateObjectsInsideCircle()
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
					if ( distanceSquared < activation_radius_squared && !cellObject.disabled )
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

	void ActivationSystem::DeactivateAllObjects()
	{
		for ( int i = 0; i < active_objects.GetCount(); ++i )
		{
			ActiveObject & activeObject = active_objects.GetObject( i );
			if ( !activeObject.pendingDeactivation )
				QueueObjectForDeactivation( activeObject );
		}
	}

	void ActivationSystem::MoveActivationPoint( float new_x, float new_y )
	{
		Validate();
		
		// clamp in bounds
		new_x = math::clamp( new_x, -bound_x, +bound_x );
		new_y = math::clamp( new_y, -bound_y, +bound_y );
		
		// if we are not enabled, don't do anything...
		if ( !enabled )
			return;
			
		const float old_x = activation_x;
		const float old_y = activation_y;

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
					if ( distanceSquared < activation_radius_squared && !cellObject.disabled )
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
	
	void ActivationSystem::InsertObject( ObjectId id, float x, float y )
	{
		assert( x >= - bound_x );
		assert( x <= + bound_x );
		assert( y >= - bound_y );
		assert( y <= + bound_y );
		Cell * cell = CellAtPosition( x, y );
		assert( cell );
		cell->InsertObject( cells, active_objects.GetObjectArray(), id, x, y );
		#ifdef DEBUG
		assert( idToCellIndex[id] == -1 );
		#endif
		idToCellIndex[id] = (int) ( cell - &cells[0] );
	}
	
	void ActivationSystem::MoveObject( ObjectId id, float x, float y )
	{
		// IMPORTANT: this is quite a slow function (linear search to find active object...)
		ActiveObject * activeObject = active_objects.FindObject( id );
		if ( activeObject )
		{
			int activeIndex = activeObject - active_objects.GetObjectArray();
			MoveActiveObject( activeIndex, x, y );
		}
		else
		{
			MoveDatabaseObject( id, x, y );
		}
	}

	void ActivationSystem::MoveActiveObject( int activeIndex, float new_x, float new_y )
	{
		// clamp the new position within bounds
		new_x = math::clamp( new_x, -bound_x, +bound_x );
		new_y = math::clamp( new_y, -bound_y, +bound_y );

		// active object
		ActiveObject * activeObject = &active_objects.GetObject( activeIndex );
		ObjectId id = activeObject->id;
		Cell * currentCell = &cells[activeObject->cellIndex];
		CellObject * cellObject = currentCell->FindObject( id );
		assert( cellObject );

		#ifdef VALIDATION
		Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), *cellObject );
		Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), *activeObject );
		#endif
		
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
			cellObject->active = 1;
			cellObject->activeObjectIndex = active_objects.GetActiveObjectIndex( *activeObject );
			activeObject->cellIndex = (int) ( currentCell - cells );
			activeObject->cellObjectIndex = currentCell->GetCellObjectIndex( *cellObject );
		}
		
		#ifdef VALIDATION
		Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), *cellObject );
		Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), activeObject );
		#endif

		// see if the object needs to be deactivated
		const float dx = new_x - activation_x;
		const float dy = new_y - activation_y;
		const float distanceSquared = dx*dx + dy*dy;
		if ( distanceSquared > activation_radius_squared )
		{
			if ( !activeObject->pendingDeactivation )
				QueueObjectForDeactivation( *activeObject );
		}
		else
			activeObject->pendingDeactivation = 0;
	}
	
	void ActivationSystem::MoveDatabaseObject( ObjectId id, float new_x, float new_y )
	{
		// clamp the new position within bounds
		new_x = math::clamp( new_x, -bound_x, +bound_x );
		new_y = math::clamp( new_y, -bound_y, +bound_y );

		// gather all of the data we need about this object
		Cell * currentCell = NULL;
		CellObject * cellObject = NULL;

		// inactive object
		currentCell = &cells[idToCellIndex[id]];
		cellObject = currentCell->FindObject( id );
		assert( cellObject );
		#ifdef VALIDATION
		Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), *cellObject );
		#endif
		
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
		}
		
		#ifdef VALIDATION
		Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), *cellObject );
		#endif
	}
	
	ActiveObject & ActivationSystem::ActivateObject( CellObject & cellObject, Cell & cell )
	{
		assert( !cellObject.disabled );
		assert( !cellObject.active );
		#ifdef VALIDATION
		Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), cellObject );
		#endif
		ActiveObject & activeObject = active_objects.InsertObject( cellObject.id );
		activeObject.id = cellObject.id;
		activeObject.cellIndex = (int) ( &cell - cells );
		activeObject.cellObjectIndex = cell.GetCellObjectIndex( cellObject );
		activeObject.pendingDeactivation = false;
		cellObject.active = 1;
		cellObject.activeObjectIndex = active_objects.GetActiveObjectIndex( activeObject );
		#ifdef VALIDATION
		Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), activeObject );
		#endif
		QueueActivationEvent( cellObject.id );
		return activeObject;
	}

	void ActivationSystem::DeactivateObject( ActiveObject & activeObject )
	{
		#ifdef VALIDATION
		Cell::ValidateActiveObject( cells, active_objects.GetObjectArray(), activeObject );
		#endif
		Cell & cell = cells[activeObject.cellIndex];
		CellObject & cellObject = cell.GetObject( activeObject.cellObjectIndex );
		cellObject.active = 0;
		cellObject.activeObjectIndex = 0;
		active_objects.DeleteObject( cells, activeObject );
		#ifdef VALIDATION
		Cell::ValidateCellObject( cells, active_objects.GetObjectArray(), cellObject );
		#endif
		QueueDeactivationEvent( cellObject.id );
	}

	void ActivationSystem::EnableObject( ObjectId objectId )
	{
		const int cellIndex = idToCellIndex[objectId];
 		Cell & cell = cells[cellIndex];
		CellObject * cellObject = cell.FindObject( objectId );
		assert( cellObject );
		cellObject->disabled = 0;
		if ( !cellObject->active )
		{
			// activate object
			ActivateObject( *cellObject, cell );			
		}
	}
	
	void ActivationSystem::DisableObject( ObjectId objectId )
	{
		const int cellIndex = idToCellIndex[objectId];
 		Cell & cell = cells[cellIndex];
		CellObject * cellObject = cell.FindObject( objectId );
		assert( cellObject );
		cellObject->disabled = 1;
		if ( cellObject->active )
		{
			// deactivate object
			ActiveObject * activeObject = active_objects.FindObject( objectId );
			assert( activeObject );
			if ( !activeObject->pendingDeactivation )
				QueueObjectForDeactivation( *activeObject );
		}
	}

	void ActivationSystem::QueueObjectForDeactivation( ActiveObject & activeObject, bool immediate )
	{
		assert( !activeObject.pendingDeactivation );
		activeObject.pendingDeactivation = true;
		activeObject.pendingDeactivationTime = immediate ? deactivationTime : 0.0f;
	}
	
	int ActivationSystem::GetEventCount()
	{
		return activation_events.size();
	}

	const Event & ActivationSystem::GetEvent( int index )
	{
		assert( index >= 0 );
		assert( index < (int) activation_events.size() );
		return activation_events[index];
	}

	void ActivationSystem::ClearEvents()
	{
		activation_events.clear();
	}

	void ActivationSystem::Validate()
	{
		#ifdef VALIDATION
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
}
