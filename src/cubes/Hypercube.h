/*
	Networked Physics Demo
	Copyright © 2008-2015 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef CUBES_HYPERCUBE_H
#define CUBES_HYPERCUBE_H

#include "Config.h"
#include "Engine.h"
#include "Simulation.h"
#include "ViewObject.h"

namespace hypercube
{
	using namespace cubes;
	
	const float PlayerCubeSize = 1.5f;
	const float NonPlayerCubeSize = 0.4f;

	struct ActiveObject
	{
 		uint64_t id : 20;
		uint64_t activeId : 16;
 		uint64_t enabled : 1;
		uint64_t player : 1;
		uint64_t authority : 3;
		uint64_t authorityTime : 8;
		math::Quaternion orientation;
		math::Vector position;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
		
		bool IsPlayer() const
		{
			return player;
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
			simulationObject.linearVelocity = linearVelocity;
			simulationObject.angularVelocity = angularVelocity;
			simulationObject.enabled = enabled;
			simulationObject.scale = player ? PlayerCubeSize : NonPlayerCubeSize;
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
			viewObjectState.scale = player ? PlayerCubeSize : NonPlayerCubeSize;
			viewObjectState.pendingDeactivation = pendingDeactivation;
		}
		
		void GetPosition( math::Vector & position )
		{
			position = this->position;
		}
	};

	struct DatabaseObject
	{
		uint64_t position;
		uint32_t orientation;
		uint32_t enabled : 1;
		uint32_t session : 8;
		uint32_t player : 1;
		
		void DatabaseToActive( ActiveObject & activeObject )
		{
 			activeObject.enabled = enabled;
			activeObject.player = player;
			DecompressPosition( position, activeObject.position );
			DecompressOrientation( orientation, activeObject.orientation );
			activeObject.linearVelocity = math::Vector(0,0,0);
			activeObject.angularVelocity = math::Vector(0,0,0);
			activeObject.authority = MaxPlayers;
		}

		void ActiveToDatabase( const ActiveObject & activeObject )
		{
			enabled = activeObject.enabled;
			player = activeObject.player;
			CompressPosition( activeObject.position, position );
			CompressOrientation( activeObject.orientation, orientation );
		}

		void GetPosition( math::Vector & position )
		{
			DecompressPosition( this->position, position );
		}
	};
}

#endif
