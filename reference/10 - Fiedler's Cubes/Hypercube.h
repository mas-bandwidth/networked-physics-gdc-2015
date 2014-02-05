/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef HYPERCUBE_H
#define HYPERCUBE_H

#include "Config.h"
#include "Engine.h"
#include "ViewObject.h"
#include "Simulation.h"

namespace hypercube
{
	using namespace engine;
	
	const float PlayerCubeSize = 1.5f;
	const float NonPlayerCubeSize = 0.4f;

	struct ActiveObject
	{
 		uint64_t id : 24;
		uint64_t activeId : 16;
 		uint64_t enabled : 1;
 		uint64_t activated : 1;
 		uint64_t confirmed : 4;
		uint64_t corrected : 4;
		uint64_t player : 1;
		uint64_t framesSinceLastUpdate : 8;				// note: i don't really like this here, it's a debugging thing...
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
		
		void ActiveToView( view::ObjectState & viewObjectState, int authority, bool pendingDeactivation, int framesSinceLastUpdate )
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
		
		bool IsConfirmed( int playerId ) const
		{
			/*
				Confirmed if all lower player ids
				have their confirm bits set.
			*/
			const int mask = ( 1<<playerId ) - 1;
			return ( confirmed & mask ) == mask;
		}
		
		bool IsConfirmedBitSet( int playerId ) const
		{
			return confirmed & (1<<playerId);
		}
		
		void SetConfirmed( int playerId )
		{
			confirmed |= (1<<playerId);
		}

		bool CanApplyCorrection( int playerId ) const
		{
			/*
				Can apply correction *only if* 
				lower player ids have not already
				applied a correction...
			*/
			const int mask = ( 1<<playerId ) - 1;
			return ( corrected & mask ) == 0;
		}
		
		void SetCorrected( int playerId )
		{
			corrected |= (1<<playerId);
		}
	};

	struct DatabaseObject
	{
		uint64_t position;
		uint32_t orientation;
		uint32_t enabled : 1;
 		uint32_t activated : 1;
 		uint32_t confirmed : 4;
		uint32_t corrected : 4;
		uint32_t player : 1;
		
		void DatabaseToActive( ActiveObject & activeObject )
		{
			activeObject.framesSinceLastUpdate = 0;
			activeObject.enabled = enabled;
			activeObject.activated = activated;
			activeObject.confirmed = confirmed;
			activeObject.corrected = corrected;
			activeObject.player = player;
			DecompressPosition( position, activeObject.position );
			DecompressOrientation( orientation, activeObject.orientation );
			activeObject.linearVelocity = math::Vector(0,0,0);
			activeObject.angularVelocity = math::Vector(0,0,0);
		}

		void ActiveToDatabase( const ActiveObject & activeObject )
		{
			activated = activeObject.activated;
			confirmed = activeObject.confirmed;
			corrected = activeObject.corrected;
			player = activeObject.player;
			enabled = activeObject.enabled;
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
