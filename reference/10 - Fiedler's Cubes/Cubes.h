/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef CUBES_H
#define CUBES_H

#include "Config.h"
#include "Engine.h"
#include "ViewObject.h"
#include "Simulation.h"

namespace cubes
{
	using namespace engine;
	
	struct ActiveObject
	{
		ObjectId id;
 		ActiveId activeId;
 		uint32_t enabled : 1;
 		uint32_t activated : 1;
		uint32_t framesSinceLastUpdate : 8;
		math::Vector position;
		math::Quaternion orientation;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
		float scale;

		bool IsPlayer() const
		{
			return scale > 1.0f;
		}

		void GetPositionXY( float & x, float & y )
		{
			x = position.x;
			y = position.y;
		}

		void ActiveToSimulation( SimulationObjectState & simulationObject )
		{
			simulationObject.position = position;
			simulationObject.orientation = orientation;
			simulationObject.orientation.normalize();
			simulationObject.linearVelocity = linearVelocity;
			simulationObject.angularVelocity = angularVelocity;
			simulationObject.enabled = enabled;
			simulationObject.scale = scale;
		}
		
		void SimulationToActive( const SimulationObjectState & simulationObject )
		{
			position = simulationObject.position;
			orientation = simulationObject.orientation;
			linearVelocity = simulationObject.linearVelocity;
			angularVelocity = simulationObject.angularVelocity;
			enabled = simulationObject.enabled;
		}
		
		void ActiveToView( view::ObjectState & viewObjectState, int authority, bool pendingDeactivation, int framesSinceLastUpdate )
		{
			viewObjectState.id = id;
			viewObjectState.authority = authority;
			viewObjectState.position = position;
			viewObjectState.orientation = orientation;
			viewObjectState.enabled = enabled;
			viewObjectState.linearVelocity = linearVelocity;
			viewObjectState.angularVelocity = angularVelocity;
			viewObjectState.scale = scale;
			viewObjectState.pendingDeactivation = pendingDeactivation;
			viewObjectState.framesSinceLastUpdate = framesSinceLastUpdate;
		}
		
		void Clamp( float bound_x, float bound_y )
		{
			if ( position.x < -bound_x || position.x > bound_x ||
				 position.y < -bound_y || position.y > bound_y )
			{
				const float damping = 0.935f;
				linearVelocity.x *= damping;
				linearVelocity.y *= damping;
			}
			position.x = math::clamp( position.x, -bound_x, +bound_x );
			position.y = math::clamp( position.y, -bound_y, +bound_y );
//			position.z = math::clamp( position.z, -PositionBoundZ, +PositionBoundZ );
			linearVelocity.x = math::clamp( linearVelocity.x, -MaxLinearVelocity, +MaxLinearVelocity );
			linearVelocity.y = math::clamp( linearVelocity.y, -MaxLinearVelocity, +MaxLinearVelocity );
			linearVelocity.z = math::clamp( linearVelocity.z, -MaxLinearVelocity, +MaxLinearVelocity );
			angularVelocity.x = math::clamp( angularVelocity.x, -MaxAngularVelocity, +MaxAngularVelocity );
			angularVelocity.y = math::clamp( angularVelocity.y, -MaxAngularVelocity, +MaxAngularVelocity );
			angularVelocity.z = math::clamp( angularVelocity.z, -MaxAngularVelocity, +MaxAngularVelocity );
		}
		
		void GetPosition( math::Vector & position )
		{
			position = this->position;
		}
	};

	struct DatabaseObject
	{
 		uint32_t enabled : 1;
 		uint32_t activated : 1;
		float scale;
		math::Vector position;
		math::Quaternion orientation;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
		
		void DatabaseToActive( ActiveObject & activeObject )
		{
			activeObject.framesSinceLastUpdate = 0;
			activeObject.enabled = enabled;
			activeObject.activated = activated;
			activeObject.position = position;
			activeObject.orientation = orientation;
			activeObject.scale = scale;
			activeObject.linearVelocity = linearVelocity;
			activeObject.angularVelocity = angularVelocity;
		}

		void ActiveToDatabase( const ActiveObject & activeObject )
		{
			enabled = activeObject.enabled;
			activated = activeObject.activated;
			position = activeObject.position;
 			orientation = activeObject.orientation;
			scale = activeObject.scale;
			linearVelocity = activeObject.linearVelocity;
			angularVelocity = activeObject.angularVelocity;
		}

		void GetPosition( math::Vector & position )
		{
			position = this->position;
		}
	};
}

#endif
