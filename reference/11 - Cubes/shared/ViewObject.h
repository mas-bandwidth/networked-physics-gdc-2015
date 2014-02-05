/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef VIEW_OBJECT_H
#define VIEW_OBJECT_H

#include "shared/Config.h"

namespace view
{
	struct ObjectState
	{
		math::Quaternion orientation;
		math::Vector position;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
		uint32_t pendingDeactivation : 1;
		uint32_t enabled : 1;
		uint32_t id : 20;
		uint32_t owner : 3;
		uint32_t authority : 3;
		float scale;

		ObjectState()
		{
			// NETHACK: do we really need a default construct for this?
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
		math::Vector origin;
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
