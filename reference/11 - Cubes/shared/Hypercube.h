/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef HYPERCUBE_H
#define HYPERCUBE_H

#include "shared/Config.h"
#include "shared/Engine.h"
#include "shared/Simulation.h"
#include "shared/ViewObject.h"

namespace hypercube
{
	using namespace engine;
	
	const float PlayerCubeSize = 1.5f;
	const float NonPlayerCubeSize = 0.4f;

	struct ActiveObject
	{
 		uint64_t id : 20;
		uint64_t activeId : 16;
 		uint64_t enabled : 1;						// 1 if moving. 0 if at rest.
		uint64_t dirty : 1;							// 1 if object has moved from initial position (server only)
		uint64_t justEnabled : 1;					// 1 if just enabled and has not been sent yet
		uint64_t justActivated : 1;					// 1 if just activated and has not been sent yet
		uint64_t confirmed : 1;						// 1 if client has received at rest state from server (no longer sends updates to server)
		uint64_t session : 8;						// session #. 0 is no session [1,255] server assigned session
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
			if ( enabled )
				dirty = 1;
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

	struct QuantizedObject
	{
 		uint32_t id : 20;
		uint32_t dirty : 1;
 		uint32_t enabled : 1;
		uint32_t justEnabled : 1;
		uint32_t justActivated : 1;
		uint32_t player : 1;
		uint32_t authority : 3;
		uint32_t orientation;
		int32_t position_x;
		int32_t position_y;
		int32_t position_z;
		int32_t linear_velocity_x;
		int32_t linear_velocity_y;
		int32_t linear_velocity_z;
		int32_t angular_velocity_x;
		int32_t angular_velocity_y;
		int32_t angular_velocity_z;
		
		void Load( const ActiveObject & activeObject, float res )
		{
			this->id = activeObject.id;
			dirty = activeObject.dirty;
			enabled = activeObject.enabled;
			justEnabled = activeObject.justEnabled;
			justActivated = activeObject.justActivated;
			player = activeObject.player;
			authority = activeObject.authority;
			CompressOrientation( activeObject.orientation, orientation );
			QuantizeVector( activeObject.position, position_x, position_y, position_z, res );
			QuantizeVector( activeObject.linearVelocity, linear_velocity_x, linear_velocity_y, linear_velocity_z, res );
			QuantizeVector( activeObject.angularVelocity, angular_velocity_x, angular_velocity_y, angular_velocity_z, res );
		}
	
		void Store( ActiveObject & activeObject, float res )
		{
			activeObject.id = id;
			activeObject.dirty = dirty;
			activeObject.enabled = enabled;
			activeObject.justEnabled = justEnabled;
			activeObject.justActivated = justActivated;
			activeObject.player = player;
			activeObject.authority = authority;
			DecompressOrientation( orientation, activeObject.orientation );
			UnquantizeVector( position_x, position_y, position_z, activeObject.position, res );
			UnquantizeVector( linear_velocity_x, linear_velocity_y, linear_velocity_z, activeObject.linearVelocity, res );
			UnquantizeVector( angular_velocity_x, angular_velocity_y, angular_velocity_z, activeObject.angularVelocity, res );
		}
	};
	
	struct DatabaseObject
	{
		uint64_t position;
		uint32_t orientation;
		uint32_t dirty : 1;
		uint32_t enabled : 1;
		uint32_t session : 8;
		uint32_t player : 1;
		
		void DatabaseToActive( ActiveObject & activeObject )
		{
			activeObject.dirty = dirty;
 			activeObject.enabled = enabled;
			activeObject.justActivated = 1;
			activeObject.justEnabled = enabled;
			activeObject.confirmed = 0;
			activeObject.session = session;
			activeObject.player = player;
			DecompressPosition( position, activeObject.position );
			DecompressOrientation( orientation, activeObject.orientation );
			activeObject.linearVelocity = math::Vector(0,0,0);
			activeObject.angularVelocity = math::Vector(0,0,0);
			activeObject.authority = MaxPlayers;
		}

		void ActiveToDatabase( const ActiveObject & activeObject )
		{
			dirty = activeObject.dirty;
			enabled = activeObject.enabled;
 			session = activeObject.session;
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
