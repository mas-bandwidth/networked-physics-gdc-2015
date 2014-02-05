// Game object framework (pure render client side)

#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "Mathematics.h"
#include "Common.h"
#include <map>
#include <algorithm>

struct GameObjectUpdate
{
	unsigned int id;
	math::Vector position;
	math::Quaternion orientation;
	float scale;
	float r,g,b;
	bool visible;
};

struct GameObject
{
	GameObject()
	{
		id = 0;
		remove = false;
		visible = false;
		blending = false;
	}

	unsigned int id;
	math::Vector position;
	math::Quaternion orientation;
	float scale;
	float r,g,b,a;
	bool remove;
	bool visible;
	bool blending;
	float blend_time;
	float blend_start;
	float blend_finish;
};

class GameObjectManager
{
public:
	
	GameObjectManager()
	{
		// ...
	}
	
	~GameObjectManager()
	{
		// free objects
		
		for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
		{
			GameObject * object = itor->second;
			assert( object );
			delete object;
		}
		objects.clear();
	}
	
	void UpdateObjects( GameObjectUpdate updates[], int updateCount )
	{
		assert( updates );
		
		// 1. mark all objects as pending removal
		//  - allows us to detect deleted objects in O(n) instead of O(n^2)
		
		for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
		{
 			GameObject * object = itor->second;
			assert( object );
			object->remove = true;
		}
		
		// 2. pass over all cubes in new render state
		//  - add cube if does not exist, update if exists
		//  - clear remove flag for each entry we touch
		
		for ( int i = 0; i < (int) updateCount; ++i )
		{
			object_map::iterator itor = objects.find( updates[i].id );
			
			if ( itor == objects.end() )
			{
				// add new game object
				
				printf( "new game object %d\n", updates[i].id );
				
				GameObject * object = new GameObject();
				
				object->id = updates[i].id;
				object->position = updates[i].position;
				object->orientation = updates[i].orientation;
				object->scale = updates[i].scale;
				object->r = updates[i].r;
				object->g = updates[i].g;
				object->b = updates[i].b;
				object->a = 0.0f;
				object->visible = false;

				objects.insert( std::make_pair( updates[i].id, object ) );
			}
			else
			{
				// update existing
				
				GameObject * object = itor->second;
				
				assert( object );

				object->position = updates[i].position;
				object->orientation = updates[i].orientation;
				object->scale = updates[i].scale;
				object->remove = false;

				if ( !object->blending )
				{
					if ( object->visible && !updates[i].visible )
					{
						// start fade out
						object->blending = true;
						object->blend_start = 1.0f;
						object->blend_finish = 0.0f;
						object->blend_time = 0.0f;
					}
					else if ( !object->visible && updates[i].visible )
					{
						// start fade in
						object->blending = true;
						object->blend_start = 0.0f;
						object->blend_finish = 1.0f;
						object->blend_time = 0.0f;
					}
				}
			}
		}
		
		// delete objects that do not exist in update
		
		object_map::iterator itor = objects.begin();
		while ( itor != objects.end() )
		{
			GameObject * object = itor->second;
			assert( object );
			if ( object->remove )
			{
				printf( "delete game object %d\n", object->id );
				delete object;
				objects.erase( itor++ );
			}
			else
			{
				++itor;
			}
		}
	}
	
	void Update( float deltaTime )
	{
		for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
		{
 			GameObject * object = itor->second;
			assert( object );
			
			/*
			// blend colors

			const float dr = updates[i].r - object->r;
			const float dg = updates[i].g - object->g;
			const float db = updates[i].b - object->b;

			const float epsilon = 0.01f;

			if ( math::abs(dr) > epsilon )
				object->r += dr * ColorChangeTightness;
			else
				object->r = updates[i].r;

			if ( math::abs(dg) > epsilon )
				object->g += dg * ColorChangeTightness;
			else
				object->g = updates[i].g;

			if ( math::abs(db) > epsilon )
				object->b += db * ColorChangeTightness;
			else
				object->b = updates[i].b;
				*/
		
			// update alpha blend
		
			if ( object->blending )
			{
				object->blend_time += deltaTime * 2.0f;
				
				if ( object->blend_finish == 1.0f )
					object->blend_time += deltaTime;		// hack: blend in twice as quick
				
				if ( object->blend_time > 1.0f )
				{
					object->a = object->blend_finish;
					object->visible = object->blend_finish != 0.0f;
					object->blending = false;
				}
				else
				{
					const float t = object->blend_time;
					const float t2 = t*t;
					const float t3 = t2*t;
					object->a = 3*t2 - 2*t3;
					if ( object->visible )
					 	object->a = 1.0f - object->a;
				}
			}
		}
	}
	
	GameObject * GetObject( unsigned int id )
	{
		object_map::iterator itor = objects.find( id );
		if ( itor != objects.end() )
			return itor->second;
		else
			return NULL;
	}
	
	void GetRenderState( RenderState & renderState )
	{
		renderState.numCubes = objects.size();
		
		assert( renderState.numCubes < (unsigned int) MaxCubes );
		
		unsigned int i = 0;
		
		for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
		{
			GameObject * object = itor->second;
			assert( object );
			renderState.cubes[i].position = object->position;
			renderState.cubes[i].orientation = object->orientation;
			renderState.cubes[i].scale = object->scale;
			renderState.cubes[i].r = 1.0f;//object->r;
			renderState.cubes[i].g = 0.0f;//object->g;
			renderState.cubes[i].b = 0.0f;//object->b;
			renderState.cubes[i].a = object->a;
			i++;
		}

		// sort back to front
		std::sort( renderState.cubes, renderState.cubes + renderState.numCubes );
	}
	
private:
	
	typedef std::map<unsigned int, GameObject*> object_map;
	
	object_map objects;
};

#endif
