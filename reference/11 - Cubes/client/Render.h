/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef RENDER_H
#define RENDER_H

#ifdef HAS_OPENGL

#include "client/Platform.h"
#include "shared/Mathematics.h"
#include "vectorial/vec2f.h"
#include "vectorial/vec3f.h"
#include "vectorial/vec4f.h"
#include "vectorial/mat4f.h"

namespace render
{
	// abstract render state

	struct ActivationArea
	{
		math::Vector origin;
		float radius;
		float startAngle;
		float r,g,b,a;
	};

	struct Grid
	{
		float r,g,b,a;
	};

	struct Cubes
	{
		Cubes()
		{
			numCubes = 0;
		}
	
		struct Cube
		{
            vectorial::mat4f transform;
            vectorial::mat4f inverse_transform;
			float r,g,b,a;
			bool operator < ( const Cube & other ) const 
			{
				// for back to front sort only! 
				return simd4f_get_y( transform.value.w ) > simd4f_get_y( other.transform.value.w ); 
			}
		};

		int numCubes;
		Cube cube[MaxViewObjects];
	};

	// render interface

	class Interface
	{
	public:
	
		Interface( int displayWidth, int displayHeight );
		~Interface();
        
        void ResizeDisplay( int displayWidth, int displayHeight );
		
		int GetDisplayWidth() const;
		int GetDisplayHeight() const;
	
		void SetLightPosition( const math::Vector & position );
	
		void SetCamera( const math::Vector & position, const math::Vector & lookAt, const math::Vector & up );
	
		void ClearScreen();

		void BeginScene( float x1, float y1, float x2, float y2 );
	 
		void RenderFrameTime( float renderTime, float simTime, float frameTime );

		void RenderDroppedFrames( int simDroppedFrames, int netDroppedFrames, int viewDroppedFrames );

		void RenderActivationArea( const ActivationArea & activationArea, float alpha );

		void RenderCubes( const Cubes & cubes, float alpha = 1.0f );
		
		void RenderCubeShadows( const Cubes & cubes );
	
		void RenderShadowQuad();
	
		void DivideSplitscreen();
	
		void DivideQuadscreen();

        void LetterBox( float pixels );

		math::Vector InverseTransform( const math::Vector & input, const math::Vector & position, const math::Quaternion & orientation, float scale );
		
		void EnterScreenSpace();

		void EnterRealScreenSpace( float width, float height );
	
		void LeaveScreenSpace();
		
	private:
		
		void CalculateFrustumPlanes( math::Frustum & frustum );
			
	private:
	
		int displayWidth;
		int displayHeight;
	
		math::Vector cameraPosition;
		math::Vector cameraLookAt;
		math::Vector cameraUp;

		math::Vector lightPosition;
	
		struct CubeData
		{
			CubeData()
			{
				culled = false;
			}

			bool culled;
		};
	
		CubeData cubeData[MaxViewObjects];
		
		struct RenderImpl * impl;
	};
}

#endif

#endif
