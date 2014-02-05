/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
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
		uint32_t authority : 3;
		uint32_t authorityTime : 8;
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
		
		void ActiveToView( view::ObjectState & viewObjectState, int authority, bool pendingDeactivation )
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
			activeObject.enabled = enabled;
			activeObject.activated = activated;
			activeObject.position = position;
			activeObject.orientation = orientation;
			activeObject.scale = scale;
			activeObject.linearVelocity = linearVelocity;
			activeObject.angularVelocity = angularVelocity;
			activeObject.authority = MaxPlayers;
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
