/*
	Networked Physics Demo
	Copyright © 2008-2015 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef CUBES_VIEW_H
#define CUBES_VIEW_H

#include "ViewObject.h"
#include "vectorial/vec3f.h"
#include "vectorial/mat4f.h"
#include <map>				// todo: remove this

namespace view
{
    struct Cube
    {
        float r,g,b,a;
        vectorial::mat4f transform;
        vectorial::mat4f inverse_transform;
    };
    
	struct Cubes
	{
	    Cubes()
	    {
	        numCubes = 0;
	    }

	    int numCubes;
	    Cube cube[MaxViewObjects];
	};

	struct ObjectUpdate
	{
		// todo: convert these guys to vectorial
		math::Quaternion orientation;
		math::Vector position;
		math::Vector linearVelocity;
		math::Vector angularVelocity;
		float scale;
		float r,g,b;
 		uint32_t id : 20;
		uint32_t authority : 3;
		uint32_t visible : 1;
	};

	struct Object
	{
		Object()
		{
			this->id = 0;
			authority = 0;
			remove = 0;
			visible = 0;
			blending = 0;
			positionError = math::Vector(0,0,0);
	 		visualOrientation = math::Quaternion(1,0,0,0);
		}

		unsigned int id;
		int authority;
		float scale;
		float r,g,b,a;
		double t;
		bool remove;
		bool visible;
		bool blending;
		float blend_time;
		float blend_start;
		float blend_finish;

		// todo: convert these guys to vectorial
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
	};

	enum InterpolationMode
	{
        INTERPOLATE_None,
		INTERPOLATE_Linear,
		INTERPOLATE_Hermite,
		INTERPOLATE_Extrapolate,
		INTERPOLATE_Max
	};

	/*	
		Manages the set of objects on the view-side which is typically
		a copy following the set of objects in the simulation active set
	*/
	
	class ObjectManager
	{
	public:

		ObjectManager();
		~ObjectManager();

		void Reset();

		void UpdateObjects( ObjectUpdate updates[], int updateCount, bool updateState = true );
		
		void InterpolateObjects( double t, float stepSize, InterpolationMode interpolationMode );

		void ExtrapolateObjects( float deltaTime );

		void Update( float deltaTime, int maxPlayers = MaxPlayers );

		Object * GetObject( unsigned int id );

		void GetRenderState( Cubes & renderState, bool interpolation = false, bool smoothing = false );

	private:

		// todo: convert this to use the core hash instead
		typedef std::map<unsigned int, Object*> object_map;
		object_map objects;
	};

	// camera object

	struct Camera
	{
		// todo: convert these guys to use vectorial
		math::Vector position;
		math::Vector lookat;
		math::Vector up;

		Camera();
	
		void EaseIn( const math::Vector & new_lookat, const math::Vector & new_position );
	
		void Snap( const math::Vector & new_lookat, const math::Vector & new_position );
	};

	// helper functions

	void getAuthorityColor( int authority, float & r, float & g, float & b, int maxPlayers = MaxPlayers );

	void getViewObjectUpdates( view::ObjectUpdate * updates, const view::Packet & viewPacket );
}

#endif
