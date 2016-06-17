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

#ifndef VIEW_OBJECT_H
#define VIEW_OBJECT_H

#include "Config.h"
#include "Mathematics.h"

namespace view
{
	struct ObjectState
	{
		// todo: convert to vectorial
		math::Quaternion orientation;
		math::Vector position;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
		uint32_t pendingDeactivation : 1;
		uint32_t enabled : 1;
		uint32_t id : 20;
		uint32_t owner : 3;			// todo: these should be generalized to bits required max players
		uint32_t authority : 3;
		float scale;

		ObjectState()
		{
			this->id = 0;
			owner = MaxPlayers;
			authority = MaxPlayers;
			position = math::Vector(0,0,0);
			orientation = math::Quaternion(1,0,0,0);
			linearVelocity = math::Vector(0,0,0);
			angularVelocity = math::Vector(0,0,0);
			scale = 1.0f;
			enabled = 1;
			pendingDeactivation = 0;
		}
	};

	struct Packet
	{
		int droppedFrames;
		float netTime;
		float simTime;
		math::Vector origin;		// todo: convert to vectorial
		int objectCount;
		ObjectState object[MaxViewObjects];
	
		Packet()
		{
			droppedFrames = 0;
			netTime = 0.0f;
			simTime = 0.0f;
			origin = math::Vector(0,0,0);
			objectCount = 0;
		}
	};
}

#endif
