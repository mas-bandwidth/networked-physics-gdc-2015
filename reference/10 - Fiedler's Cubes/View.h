/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef VIEW_H
#define VIEW_H

#include "Config.h"
#include "Render.h"
#include "ViewObject.h"

namespace view
{
	const float PositionSmoothingTightness = 0.075f;
	const float OrientationSmoothingTightness = 0.2f;

	struct ObjectUpdate
	{
		unsigned int id;
		unsigned int owner;
		unsigned int authority;
		math::Vector position;
		math::Quaternion orientation;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
		float scale;
		float r,g,b;
		bool flash;
		bool visible;
	};

	struct Object
	{
		Object()
		{
			id = 0;
			remove = false;
			visible = false;
			blending = false;
			positionError = math::Vector(0,0,0);
	 		visualOrientation = math::Quaternion(1,0,0,0);
			framesSinceFlash = -1;
		}

		unsigned int id;
		math::Vector position;
		math::Quaternion orientation;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
		math::Vector positionError;
		math::Quaternion visualOrientation;
		math::Vector previousPosition;
		math::Vector previousLinearVelocity;
		math::Vector previousAngularVelocity;
		math::Quaternion previousOrientation;
		math::Vector interpolatedPosition;
		math::Quaternion interpolatedOrientation;
		float scale;
		float r,g,b,a;
		float t;
		bool remove;
		bool visible;
		bool blending;
		float blend_time;
		float blend_start;
		float blend_finish;
		int framesSinceFlash;
	};

	enum InterpolationMode
	{
		INTERPOLATE_Linear,
		INTERPOLATE_Hermite,
		INTERPOLATE_Extrapolate,
		INTERPOLATE_Max
	};

	// ---------------------------------------------------------------
	
	class ObjectManager
	{
	public:

		ObjectManager()
		{
			// ...
		}

		~ObjectManager()
		{
			Reset();
		}

		void Reset()
		{
			for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
			{
				Object * object = itor->second;
				assert( object );
				delete object;
			}
			objects.clear();
		}

		void UpdateObjects( ObjectUpdate updates[], int updateCount, bool updateState = true, bool showFlash = false )
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
					object->position = updates[i].position;
					object->orientation = updates[i].orientation;
					object->previousPosition = updates[i].position;
					object->previousLinearVelocity = updates[i].linearVelocity;
					object->previousAngularVelocity = updates[i].angularVelocity;
					object->previousOrientation = updates[i].orientation;
					object->positionError = math::Vector(0,0,0);
					object->visualOrientation = updates[i].orientation;
					object->linearVelocity = updates[i].linearVelocity;
					object->angularVelocity = updates[i].angularVelocity;
					object->scale = updates[i].scale;
					object->r = updates[i].r;
					object->g = updates[i].g;
					object->b = updates[i].b;
					object->a = 0.0f;
					object->visible = false;
					
					if ( updates[i].flash )
						object->framesSinceFlash = 0;

					objects.insert( std::make_pair( updates[i].id, object ) );
				}
				else
				{
					// update existing

					Object * object = itor->second;

					assert( object );
					
					if ( !showFlash )
					{
						object->r += ( updates[i].r - object->r ) * ColorChangeTightness;
						object->g += ( updates[i].g - object->g ) * ColorChangeTightness;
						object->b += ( updates[i].b - object->b ) * ColorChangeTightness;
					}
					else
					{
						if ( updates[i].flash )
						{
							object->r = 0.3f;
							object->g = 0.3f;
							object->b = 1.0f;
							object->framesSinceFlash = 0;
						}
						else
						{
							if ( object->framesSinceFlash == FlashFrames )
							{
								object->r = 0.8f;
								object->g = 0.8f;
								object->b = 0.85f;
							}
							else if ( object->framesSinceFlash > FlashFrames )
							{
								object->r += ( 0.8f - object->r ) * ColorChangeTightness;
								object->g += ( 0.8f - object->g ) * ColorChangeTightness;
								object->b += ( 0.85f - object->b ) * ColorChangeTightness;
							}
							object->framesSinceFlash++;
						}
					}
					
					if ( updateState )
					{
						object->positionError = ( object->position + object->positionError ) - updates[i].position;
						if ( object->positionError.lengthSquared() > 50.0f )
						{
							object->positionError = math::Vector(0,0,0);
							object->visualOrientation = updates[i].orientation;
						}

						object->previousPosition = object->position;
						object->previousLinearVelocity = object->linearVelocity;
						object->previousAngularVelocity = object->angularVelocity;
						object->previousOrientation = object->orientation;
						object->position = updates[i].position;
						object->orientation = updates[i].orientation;
						object->linearVelocity = updates[i].linearVelocity;
						object->angularVelocity = updates[i].angularVelocity;
						object->scale = updates[i].scale;
					}
					
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
		
		void InterpolateObjects( float t, float stepSize, InterpolationMode interpolationMode )
		{
			for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
			{
	 			Object * object = itor->second;

				assert( object );
		
				if ( interpolationMode == INTERPOLATE_Linear )
				{
					object->interpolatedPosition = object->previousPosition + ( object->position - object->previousPosition ) * t;
					object->interpolatedOrientation = math::slerp( object->previousOrientation, object->orientation, t );
				}
				else if ( interpolationMode == INTERPOLATE_Hermite )
				{
					math::hermite_spline( t, object->previousPosition, object->position, object->previousLinearVelocity * stepSize, object->linearVelocity * stepSize, object->interpolatedPosition );
					math::Quaternion spin0 = 0.5f * math::Quaternion( 0, object->previousAngularVelocity.x, object->previousAngularVelocity.y, object->previousAngularVelocity.z ) * object->previousOrientation;
					math::Quaternion spin1 = 0.5f * math::Quaternion( 0, object->angularVelocity.x, object->angularVelocity.y, object->angularVelocity.z ) * object->orientation;
					math::hermite_spline( t, object->previousOrientation, object->orientation, spin0 * stepSize, spin1 * stepSize, object->interpolatedOrientation );
				}
				else if ( interpolationMode == INTERPOLATE_Extrapolate )
				{
					math::Vector p0 = object->previousPosition + object->previousLinearVelocity * stepSize;
					math::Vector p1 = object->position + object->linearVelocity * stepSize;
					math::Vector t0 = object->previousLinearVelocity * stepSize;
					math::Vector t1 = object->linearVelocity * stepSize;
					math::hermite_spline( t, p0, p1, t0, t1, object->interpolatedPosition );

					math::Quaternion q0 = object->previousOrientation;
					math::Quaternion q1 = object->orientation;
					math::Quaternion s0 = 0.5f * math::Quaternion( 0, object->previousAngularVelocity.x, object->previousAngularVelocity.y, object->previousAngularVelocity.z ) * object->previousOrientation * stepSize;
					math::Quaternion s1 = 0.5f * math::Quaternion( 0, object->angularVelocity.x, object->angularVelocity.y, object->angularVelocity.z ) * object->orientation * stepSize;
					math::hermite_spline( t, q0, q1, s0, s1, object->interpolatedOrientation );
				}
			}
		}

		void ExtrapolateObjects( float deltaTime )
		{
			for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
			{
	 			Object * object = itor->second;

				assert( object );

				object->position += object->linearVelocity * deltaTime;

				math::Quaternion orientationSpin = 0.5f * math::Quaternion( 0, object->angularVelocity.x, object->angularVelocity.y, object->angularVelocity.z ) * object->orientation;
				object->orientation += orientationSpin * deltaTime;
				object->orientation.normalize();

				math::Quaternion visualOrientationSpin = 0.5f * math::Quaternion( 0, object->angularVelocity.x, object->angularVelocity.y, object->angularVelocity.z ) * object->visualOrientation;
				object->visualOrientation += visualOrientationSpin * deltaTime;
				object->visualOrientation.normalize();
			}
		}

		void Update( float deltaTime )
		{
			for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
			{
	 			Object * object = itor->second;

				assert( object );

				// update error smoothing
				
				object->positionError *= ( 1.0f - PositionSmoothingTightness );
				object->visualOrientation = math::slerp( object->visualOrientation, object->orientation, OrientationSmoothingTightness );

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
			}
		}

		Object * GetObject( unsigned int id )
		{
			object_map::iterator itor = objects.find( id );
			if ( itor != objects.end() )
				return itor->second;
			else
				return NULL;
		}

		void GetRenderState( render::Cubes & renderState, bool interpolation = false, bool smoothing = true )
		{
			renderState.numCubes = objects.size();

			assert( renderState.numCubes <= MaxObjectsToRender );

			unsigned int i = 0;

			for ( object_map::iterator itor = objects.begin(); itor != objects.end(); ++itor )
			{
				Object * object = itor->second;
				assert( object );
				if ( !interpolation )
				{
					if ( smoothing )
					{
						renderState.cube[i].position = object->position + object->positionError;
						renderState.cube[i].orientation = object->visualOrientation;
					}
					else
					{
						renderState.cube[i].position = object->position;
						renderState.cube[i].orientation = object->orientation;
					}
				}
				else
				{
					renderState.cube[i].position = object->interpolatedPosition;
					renderState.cube[i].orientation = object->interpolatedOrientation;
				}
				renderState.cube[i].scale = object->scale;
				renderState.cube[i].r = object->r;
				renderState.cube[i].g = object->g;
				renderState.cube[i].b = object->b;
				renderState.cube[i].a = object->a;
				i++;
			}

			// sort back to front
			std::sort( renderState.cube, renderState.cube + renderState.numCubes );
		}

	private:

		typedef std::map<unsigned int, Object*> object_map;

		object_map objects;
	
		friend void MergeViewObjectSets( ObjectManager * gameObjects[], int maxPlayers, ObjectManager & output, int primaryPlayer, bool blendColors );
	};
	
	// helper function to merge multiple sets of game objects into a single one that can be used for rendering
	//  - useful for combining the view from multiple nodes into a single set of game objects

	inline void MergeViewObjectSets( ObjectManager * gameObjects[], int maxPlayers, ObjectManager & output, int primaryPlayer, bool blendColors )
	{
		for ( int i = maxPlayers - 1; i >= 0; --i )
		{
			if ( i == primaryPlayer )
				continue;
			
			for ( ObjectManager::object_map::iterator itor = gameObjects[i]->objects.begin(); itor != gameObjects[i]->objects.end(); ++itor )
			{
				int key = itor->first;
	 			Object * srcObject = itor->second;
				Object * dstObject = output.objects[key];
				if ( !dstObject )
				{
					dstObject = new Object();
					*dstObject = *srcObject;
					output.objects[key] = dstObject;
				}
				else
				{
					float currentAlpha = dstObject->a;
					float newAlpha = srcObject->a;
					float newR = ( srcObject->r + dstObject->r ) * 0.5f;
					float newG = ( srcObject->g + dstObject->g ) * 0.5f;
					float newB = ( srcObject->b + dstObject->b ) * 0.5f;
					*dstObject = *srcObject;
					dstObject->a = std::max( currentAlpha, newAlpha );
					if ( blendColors )
					{
						dstObject->r = newR;
						dstObject->g = newG;
						dstObject->b = newB;
					}
				}
			}
		}

		int i = primaryPlayer;
		for ( ObjectManager::object_map::iterator itor = gameObjects[i]->objects.begin(); itor != gameObjects[i]->objects.end(); ++itor )
		{
			int key = itor->first;
			Object * srcObject = itor->second;
			Object * dstObject = output.objects[key];
			if ( !dstObject )
			{
				dstObject = new Object();
				*dstObject = *srcObject;
				output.objects[key] = dstObject;
			}
			else
			{
				float currentAlpha = dstObject->a;
				float newAlpha = srcObject->a;
				float newR = ( srcObject->r + dstObject->r ) * 0.5f;
				float newG = ( srcObject->g + dstObject->g ) * 0.5f;
				float newB = ( srcObject->b + dstObject->b ) * 0.5f;
				*dstObject = *srcObject;
				dstObject->a = std::max( currentAlpha, newAlpha );
				if ( blendColors )
				{
					dstObject->r = newR;
					dstObject->g = newG;
					dstObject->b = newB;
				}
			}
		}
	}
	
	// ---------------------------------------------------------

	void getAuthorityColor( int authority, float & r, float & g, float & b, int maxPlayers = MaxPlayers )
	{
		assert( authority >= 0 );
		assert( authority <= MaxPlayers );
		
		if ( authority >= maxPlayers )
		{
			r = 0.8f;
			g = 0.8f;
			b = 0.85f;
		}
		else if ( authority == 0 )
		{
			r = 1.0f;
			g = 0.15f;
			b = 0.15f;
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

	void getViewObjectUpdates( view::ObjectUpdate * updates, const view::Packet & viewPacket, int authorityOverride = -1 )
	{
		for ( int i = 0; i < (int) viewPacket.objectCount; ++i )
		{
			updates[i].id = viewPacket.object[i].id;
			updates[i].owner = viewPacket.object[i].owner;
			updates[i].authority = authorityOverride < 0 ? viewPacket.object[i].authority : authorityOverride;
			updates[i].position = viewPacket.object[i].position;
			updates[i].orientation = viewPacket.object[i].orientation;
			updates[i].linearVelocity = viewPacket.object[i].linearVelocity;
			updates[i].angularVelocity = viewPacket.object[i].angularVelocity;
			updates[i].scale = viewPacket.object[i].scale;
			updates[i].visible = !viewPacket.object[i].pendingDeactivation;
			getAuthorityColor( updates[i].authority, updates[i].r, updates[i].g, updates[i].b );
		}
	}
	
	// ------------------------------------------------------

	struct Camera
	{
		math::Vector position;
		math::Vector lookat;
		math::Vector up;

		Camera()
		{
			position = math::Vector(0,0,0);
			lookat = math::Vector(0,0,0);
			up = math::Vector(0,0,1);
		}
	
		void EaseIn( const math::Vector & new_lookat, const math::Vector & new_position )
		{
			if ( ( new_lookat - lookat ).lengthSquared() > 0.000001f )
				lookat += ( new_lookat - lookat ) * 0.2f;

			if ( ( new_position - position ).lengthSquared() > 0.000001f )
				position += ( new_position - position ) * 0.2f;
			
			up = ( position - lookat ).cross( math::Vector(1,0,0) );
		}
	
		void Snap( const math::Vector & new_lookat, const math::Vector & new_position )
		{
			lookat = new_lookat;
			position = new_position;
			up = ( position - lookat ).cross( math::Vector(1,0,0) );
		}
	};

	void setupActivationArea( render::ActivationArea & activationArea, const math::Vector & origin, float radius, float t )
	{
		activationArea.origin = origin;
		activationArea.radius = radius;
		activationArea.startAngle = - t * 0.75f;
		activationArea.r = 0.0f;
		activationArea.g = 0.0f;
		activationArea.b = 0.1f;
		activationArea.a = 0.5f + pow( math::sin( t * 5 ) + 1.0f, 2 ) * 0.8f;
		if ( activationArea.a > 1.0f )
			activationArea.a = 1.0f;
		activationArea.a *= 0.4f;
	}

	void setCameraAndLight( render::Render * render, const Camera & camera )
	{
		render->SetCamera( camera.position, camera.lookat, camera.up );
		math::Vector lightPosition = math::Vector( 25.0f, -25.0f, 50.0f );
		lightPosition += camera.lookat;
		render->SetLightPosition( lightPosition );
	}
}

#endif
