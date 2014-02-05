/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef VIEW_OBJECT_H
#define VIEW_OBJECT_H

const int MaxViewObjects = 1024;

namespace view
{
	struct ObjectState
	{
		bool pendingDeactivation;
		bool enabled;
		unsigned int id;
		unsigned int owner;
		unsigned int authority;
		unsigned int framesSinceLastUpdate;
		float scale;
		math::Vector position;
		math::Quaternion orientation;
		math::Vector linearVelocity;
		math::Vector angularVelocity;

		ObjectState()
		{
			id = 0;
			owner = MaxPlayers;
			authority = MaxPlayers;
			position = math::Vector(0,0,0);
			orientation = math::Quaternion(1,0,0,0);
			enabled = true;
			linearVelocity = math::Vector(0,0,0);
			angularVelocity = math::Vector(0,0,0);
			scale = 1.0f;
			pendingDeactivation = true;
			framesSinceLastUpdate = 0;
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
