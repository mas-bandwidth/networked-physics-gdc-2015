/*
	Networked Physics Demo
	Copyright © 2008-2015 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "View.h"

namespace view
{
	void ObjectManager::Reset()
	{
		for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
		{
			Object * object = itor->second;
			assert( object );
			delete object;
		}
		objects.clear();
	}

	void ObjectManager::UpdateObjects( ObjectUpdate updates[], int updateCount )
	{
		assert( updates );

		// 1. mark all objects as pending removal
		//  - allows us to detect deleted objects in O(n) instead of O(n^2)

		for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
		{
 			Object * object = itor->second;
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
				// add new view object

				Object * object = new Object();

				object->id = updates[i].id;
				object->authority = updates[i].authority;
				object->position = updates[i].position;
				object->orientation = updates[i].orientation;
				object->scale = updates[i].scale;
				getAuthorityColor( object->authority, object->r, object->g, object->b, MaxPlayers );
				object->a = 0.0f;
				object->visible = false;
				
				uint32_t id = updates[i].id;

				objects.insert( std::make_pair( id, object ) );
			}
			else
			{
				// update existing

				Object * object = itor->second;

				assert( object );

				object->authority = updates[i].authority;

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
			Object * object = itor->second;
			assert( object );
			if ( object->remove )
			{
				delete object;
				objects.erase( itor++ );
			}
			else
			{
				++itor;
			}
		}
	}
	
	void ObjectManager::Update( float deltaTime )
	{
		for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
		{
 			Object * object = itor->second;

			assert( object );

			// update alpha blend

			if ( object->blending )
			{
				object->blend_time += deltaTime * 4.0f;

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

			// update color blend

			const float tightness = ( object->authority == 0 ) ? ColorChangeTightnessAuthority : ColorChangeTightnessDefault;

			float target_r, target_g, target_b;
			getAuthorityColor( object->authority, target_r, target_g, target_b, MaxPlayers );

			object->r += ( target_r - object->r ) * tightness;
			object->g += ( target_g - object->g ) * tightness;
			object->b += ( target_b - object->b ) * tightness;
		}
	}

	Object * ObjectManager::GetObject( unsigned int id )
	{
		object_map::iterator itor = objects.find( id );
		if ( itor != objects.end() )
			return itor->second;
		else
			return NULL;
	}

    void ObjectManager::GetRenderState( Cubes & renderState, const vectorial::vec3f * position_error, const vectorial::quat4f * orientation_error )
    {
        renderState.numCubes = objects.size();

        assert( renderState.numCubes <= MaxViewObjects );

        unsigned int i = 0;
        for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
        {
            Object * object = itor->second;
            assert( object );

            const int id = object->id;

            vectorial::mat4f translation = vectorial::mat4f::translation( object->position + ( position_error ? position_error[id] : vectorial::vec3f(0,0,0) ) );
            vectorial::mat4f rotation = vectorial::mat4f::rotation( object->orientation * ( orientation_error ? orientation_error[id] : vectorial::quat4f(0,0,0,1) ) );
            vectorial::mat4f scale = vectorial::mat4f::scale( object->scale * 0.5f );

            vectorial::mat4f inv_translation = vectorial::mat4f::translation( - ( object->position + ( position_error ? position_error[id] : vectorial::vec3f(0,0,0) ) ) );
            vectorial::mat4f inv_rotation = transpose( rotation );
            vectorial::mat4f inv_scale = vectorial::mat4f::scale( 1.0f / ( object->scale * 0.5f ) );

            renderState.cube[i].transform = translation * rotation * scale;
            renderState.cube[i].inverse_transform = inv_rotation * inv_translation * inv_scale;
            
            renderState.cube[i].r = object->r;
            renderState.cube[i].g = object->g;
            renderState.cube[i].b = object->b;
            renderState.cube[i].a = object->a;

            i++;
        }
    }

	// ------------------------------------------------------

	Camera::Camera()
	{
		position = vectorial::vec3f::zero();
		lookat = vectorial::vec3f::zero();
		up = vectorial::vec3f(0,0,1);
	}

	void Camera::EaseIn( const vectorial::vec3f & new_lookat, const vectorial::vec3f & new_position )
	{
		const float epsilon = 0.00001f;

		if ( vectorial::length_squared( new_lookat - lookat ) > epsilon )
			lookat += ( new_lookat - lookat ) * 0.15f;

		if ( vectorial::length_squared( new_position - position ) > epsilon )
			position += ( new_position - position ) * 0.15f;
		
		up = vectorial::cross( position - lookat, vectorial::vec3f(1,0,0) );
	}

	void Camera::Snap( const vectorial::vec3f & new_lookat, const vectorial::vec3f & new_position )
	{
		lookat = new_lookat;
		position = new_position;
		up = vectorial::cross( position - lookat, vectorial::vec3f(1,0,0) );
	}

	// ------------------------------------------------------

	void getAuthorityColor( int authority, float & r, float & g, float & b, int maxPlayers )
	{
		assert( authority >= 0 );
		assert( authority <= MaxPlayers );
		
		if ( authority >= maxPlayers )
		{
			r = 0.75f;
			g = 0.75f;
			b = 0.8f;
		}
		else if ( authority == 0 )
		{
			r = 0.9f;
			g = 0.0f;
			b = 0.0f;
		}
		else if ( authority == 1 )
		{
			r = 0.3f;
			g = 0.3f;
			b = 1.0f;
		}
		else if ( authority == 2 )
		{
			r = 0.0f;
			g = 0.9f;
			b = 0.1f;
		}
		else if ( authority == 3 )
		{
			r = 1.0f;
			g = 0.95f;
			b = 0.2f;
		}
	}

	void getViewObjectUpdates( view::ObjectUpdate * updates, const view::Packet & viewPacket )
	{
		for ( int i = 0; i < (int) viewPacket.objectCount; ++i )
		{
			updates[i].id = viewPacket.object[i].id;
			updates[i].authority = viewPacket.object[i].authority;
			updates[i].position = vectorial::vec3f( viewPacket.object[i].position.x, viewPacket.object[i].position.y, viewPacket.object[i].position.z );
			updates[i].orientation = vectorial::quat4f( viewPacket.object[i].orientation.x, viewPacket.object[i].orientation.y, viewPacket.object[i].orientation.z, viewPacket.object[i].orientation.w );
			updates[i].scale = viewPacket.object[i].scale;
			updates[i].visible = !viewPacket.object[i].pendingDeactivation;
		}
	}
}
