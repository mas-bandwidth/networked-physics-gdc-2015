/*
    Networked Physics Demo

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
		
		void ActiveToView( view::ObjectState & viewObjectState, int _authority, bool pendingDeactivation )
		{
			viewObjectState.id = id;
			viewObjectState.authority = _authority;
			viewObjectState.position = position;
			viewObjectState.orientation = orientation;
			viewObjectState.enabled = enabled;
			viewObjectState.linearVelocity = linearVelocity;
			viewObjectState.angularVelocity = angularVelocity;
			viewObjectState.scale = player ? PlayerCubeSize : NonPlayerCubeSize;
			viewObjectState.pendingDeactivation = pendingDeactivation;
		}
		
		void GetPosition( math::Vector & _position )
		{
			position = _position;
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

		void GetPosition( math::Vector & _position )
		{
			DecompressPosition( position, _position );
		}
	};
}

#endif
