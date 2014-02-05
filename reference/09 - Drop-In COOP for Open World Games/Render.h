// render (OpenGL)

#ifndef RENDER_H
#define RENDER_H

#include "Platform.h"
#include "Display.h"

#define BLEND_CUBE_COLORS
#define RENDER_SHADOWS

#include <map>

const float ShadowAlphaThreshold = 0.25f;

class Render
{
public:
	
	Render( int displayWidth, int displayHeight )
	{
		this->displayWidth = displayWidth;
		this->displayHeight = displayHeight;
		
		cameraPosition = math::Vector( 0.0f, -15.0f, 6.0f );
		cameraLookAt = math::Vector( 0.0f, 0.0f, 0.0f );

		lightPosition = math::Vector( 25.0f, -25.0f, 50.0f );
		
		// setup cube display list
	
		cubeDisplayList = glGenLists( 1 );
	
		glNewList( cubeDisplayList, GL_COMPILE );
	
		glBegin( GL_QUADS );

			const float s = 0.5f;

			glNormal3f(  0,  0, +s );
			glVertex3f( -s, -s, +s );
			glVertex3f( -s, +s, +s );
			glVertex3f( +s, +s, +s );
			glVertex3f( +s, -s, +s );

			glNormal3f(  0,  0,  -s );
			glVertex3f( -s, -s, -s );
			glVertex3f( +s, -s, -s );
			glVertex3f( +s, +s, -s );
			glVertex3f( -s, +s, -s );

			glNormal3f(  0, +s,  0 );
			glVertex3f( -s, +s, -s );
			glVertex3f( +s, +s, -s );
			glVertex3f( +s, +s, +s );
			glVertex3f( -s, +s, +s );

			glNormal3f(  0, -s,  0 );
			glVertex3f( -s, -s, -s );
			glVertex3f( -s, -s, +s );
			glVertex3f( +s, -s, +s );
			glVertex3f( +s, -s, -s );

			glNormal3f( +s,  0,  0 );
			glVertex3f( +s, -s, -s );
			glVertex3f( +s, -s, +s );
			glVertex3f( +s, +s, +s );
			glVertex3f( +s, +s, -s );

			glNormal3f( -s,  0,  0 );
			glVertex3f( -s, -s, -s );
			glVertex3f( -s, +s, -s );
			glVertex3f( -s, +s, +s );
			glVertex3f( -s, -s, +s );

		glEnd();
       
		glEndList();
	}
	
	~Render()
	{
		// free display lists
		
		glDeleteLists( cubeDisplayList, 1 );
	}
	
	void SetCamera( const math::Vector & position, const math::Vector & lookAt )
	{
		cameraPosition = position;
		cameraLookAt = lookAt;
	}
	
	void ClearScreen()
	{
		glViewport( 0, 0, displayWidth, displayHeight );
		glDisable( GL_SCISSOR_TEST );
		glClearStencil( 0 );
		glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );		
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	}

	void RenderScene( const RenderState & renderState, float x1, float y1, float x2, float y2 )
	{
		// setup viewport & scissor
	
		glViewport( x1, y1, x2 - x1, y2 - y1 );
		glScissor( x1, y1, x2 - x1, y2 - y1 );
		glEnable( GL_SCISSOR_TEST );

		// setup view

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		float fov = 40.0f;
		gluPerspective( fov, ( x2 - x1 ) / ( y2 - y1 ), 0.1f, 100.0f );
	
		// setup camera
	
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		gluLookAt( cameraPosition.x, cameraPosition.y, cameraPosition.z,
		           cameraLookAt.x, cameraLookAt.y, cameraLookAt.z,
				   0.0f, 0.0f, 1.0f );
				
		// perform frustum culling on cubes
		
		#ifdef FRUSTUM_CULLING
		FrustumCullCubes( renderState );
		#endif
							
		// enable alpha blending
		
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
							
		// setup lights

        glShadeModel( GL_SMOOTH );
	
        glEnable( GL_LIGHT0 );

        GLfloat lightAmbientColor[] = { 0.5, 0.5, 0.5, 1.0 };
        glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lightAmbientColor );

        glLightf( GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f );

		GLfloat position[4];
		position[0] = lightPosition.x;
		position[1] = lightPosition.y;
		position[2] = lightPosition.z;
		position[3] = 1.0f;
        glLightfv( GL_LIGHT0, GL_POSITION, position );

		// enable depth buffering and backface culling
	
		glEnable( GL_DEPTH_TEST );
	    glDepthFunc( GL_LEQUAL );
    
	    glEnable( GL_CULL_FACE );
	    glCullFace( GL_BACK );
	    glFrontFace( GL_CW );

		// render scene

		glEnable( GL_LIGHTING );
		glEnable( GL_NORMALIZE );

		RenderFloor();

		RenderHibernationArea( renderState );
		
		RenderCubes( renderState );

		glDisable( GL_LIGHTING );
		glDisable( GL_NORMALIZE );

		// disable alpha blending
		
		glDisable( GL_BLEND );
	}
	
	void RenderShadows( const RenderState & renderState, float x1, float y1, float x2, float y2 )
	{
	#ifdef RENDER_SHADOWS
		
		// render shadows

		RenderCubeShadowVolumes( renderState.numCubes, renderState.cubes, lightPosition );
	
		RenderShadow();
		
	#endif
	}
	
	void RenderFloor()
	{
		const float r = 1.0f;
		const float g = 1.0f;
		const float b = 1.0f;
		const float a = 1.0f;

	    glDisable( GL_CULL_FACE );
	
		glColor4f( r, g, b, a );

        GLfloat color[] = { r, g, b, a };

		glMaterialfv( GL_FRONT, GL_AMBIENT, color );
		glMaterialfv( GL_FRONT, GL_DIFFUSE, color );

		const float s = 20.0f;

        glBegin( GL_QUADS );

			glNormal3f( 0, 0, +1 );
			glVertex3f( +s, +s, 0 );
			glVertex3f( +s, -s, 0 );
			glVertex3f( -s, -s, 0 );
			glVertex3f( -s, +s, 0 );

		glEnd();

	    glEnable( GL_CULL_FACE );
	}

	void RenderHibernationArea( const RenderState & renderState )
	{
		const float r = renderState.hibernationArea.r;
		const float g = renderState.hibernationArea.g;
		const float b = renderState.hibernationArea.b;
		const float a = renderState.hibernationArea.a;

	    glDisable( GL_CULL_FACE );
	
		glColor4f( r, g, b, a );

        GLfloat color[] = { r, g, b, a };

		glMaterialfv( GL_FRONT, GL_AMBIENT, color );
		glMaterialfv( GL_FRONT, GL_DIFFUSE, color );

		const int steps = 40;
		const float radius = renderState.hibernationArea.radius;
		const float thickness = 0.1f;
		const float r1 = radius - thickness;
		const float r2 = radius + thickness;

		const math::Vector & origin = renderState.hibernationArea.origin;

		const float bias = 0.001f;
		const float deltaAngle = 2*math::pi / steps;

		float angle = renderState.hibernationArea.startAngle;

        glBegin( GL_QUADS );
		for ( float i = 0; i <= steps; i++ )
		{
			glVertex3f( r1 * math::cos(angle) + origin.x, r1 * math::sin(angle) + origin.y, bias );
			glVertex3f( r2 * math::cos(angle) + origin.x, r2 * math::sin(angle) + origin.y, bias );
			glVertex3f( r2 * math::cos(angle+deltaAngle*0.5f) + origin.x, r2 * math::sin(angle+deltaAngle*0.5f) + origin.y, bias );
			glVertex3f( r1 * math::cos(angle+deltaAngle*0.5f) + origin.x, r1 * math::sin(angle+deltaAngle*0.5f) + origin.y, bias );
			angle += deltaAngle;
		}
		glEnd();

	    glEnable( GL_CULL_FACE );
	}
	
	void RenderCubes( const RenderState & renderState )
	{
		for ( int i = 0; i < (int) renderState.numCubes; ++i )
		{
			if ( cubeData[i].culled )
				continue;

			PushTransform( renderState.cubes[i].position, renderState.cubes[i].orientation, renderState.cubes[i].scale );

			const float r = renderState.cubes[i].r;
			const float g = renderState.cubes[i].g;
			const float b = renderState.cubes[i].b;
			const float a = renderState.cubes[i].a;

	        GLfloat color[] = { r, g, b, a };

			glMaterialfv( GL_FRONT, GL_AMBIENT, color );
			glMaterialfv( GL_FRONT, GL_DIFFUSE, color );

			RenderCube();

			PopTransform();
		}
	}
	
	void RenderCube()
	{
		glCallList( cubeDisplayList );
	}

	void RenderCubeShadowVolumes( int numCubes, const RenderState::Cube cubes[], const math::Vector & lightPosition )
	{
		#ifdef DEBUG_SHADOW_VOLUMES
		
			// visualize shadow volumes in pink

			glDepthMask( GL_FALSE );
			
			glColor3f( 1.0f, 0.75f, 0.8f );

			for ( int i = 0; i < numCubes; ++i )
			{
				if ( cubeData[i].culled )
					continue;

				const RenderState::Cube & cube = cubes[i];

				PushTransform( cube.position, cube.orientation, cube.scale );

				math::Vector localLightPosition = InverseTransform( lightPosition, cube.position, cube.orientation, cube.scale );

	            RenderShadowVolume( cube, localLightPosition );

				PopTransform();
			}

			glDepthMask( GL_TRUE);
		
		#else
			
			// render shadow volumes to stencil buffer
		
			glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
			glDepthMask( GL_FALSE );
            
			glEnable( GL_STENCIL_TEST );

            glCullFace( GL_BACK );
            glStencilFunc( GL_ALWAYS, 0x0, 0xff );
            glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );

			for ( int i = 0; i < numCubes; ++i )
			{
				if ( cubeData[i].culled || cubes[i].a < ShadowAlphaThreshold )
					continue;

				const RenderState::Cube & cube = cubes[i];

				PushTransform( cube.position, cube.orientation, cube.scale );

				math::Vector localLightPosition = InverseTransform( lightPosition, cube.position, cube.orientation, cube.scale );

	            RenderShadowVolume( cube, localLightPosition );
	
				PopTransform();
			}

            glCullFace( GL_FRONT );
            glStencilOp( GL_KEEP, GL_KEEP, GL_DECR );

			for ( int i = 0; i < numCubes; ++i )
			{
				if ( cubeData[i].culled || cubes[i].a < ShadowAlphaThreshold )
					continue;

				const RenderState::Cube & cube = cubes[i];

				PushTransform( cube.position, cube.orientation, cube.scale );

				math::Vector localLightPosition = InverseTransform( lightPosition, cube.position, cube.orientation, cube.scale );

	            RenderShadowVolume( cube, localLightPosition );
	
				PopTransform();
			}
		
			glCullFace( GL_BACK );
			glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
			glDepthMask( GL_TRUE);

			glDisable( GL_STENCIL_TEST );
			
		#endif
	}
	
	void RenderShadowVolume( const RenderState::Cube & cube, const math::Vector & light )
	{
        glBegin( GL_QUADS );

		const float s = 0.5f;

        RenderSilhouette( light, math::Vector(-s,+s,-s), math::Vector(+s,+s,-s) );
        RenderSilhouette( light, math::Vector(+s,+s,-s), math::Vector(+s,+s,+s) );
        RenderSilhouette( light, math::Vector(+s,+s,+s), math::Vector(-s,+s,+s) );
        RenderSilhouette( light, math::Vector(-s,+s,+s), math::Vector(-s,+s,-s) );

        RenderSilhouette( light, math::Vector(-s,-s,-s), math::Vector(+s,-s,-s) );
		RenderSilhouette( light, math::Vector(+s,-s,-s), math::Vector(+s,-s,+s) );
        RenderSilhouette( light, math::Vector(+s,-s,+s), math::Vector(-s,-s,+s) );
		RenderSilhouette( light, math::Vector(-s,-s,+s), math::Vector(-s,-s,-s) );

        RenderSilhouette( light, math::Vector(-s,+s,-s), math::Vector(-s,-s,-s) );
        RenderSilhouette( light, math::Vector(+s,+s,-s), math::Vector(+s,-s,-s) );
        RenderSilhouette( light, math::Vector(+s,+s,+s), math::Vector(+s,-s,+s) );
        RenderSilhouette( light, math::Vector(-s,+s,+s), math::Vector(-s,-s,+s) );

        glEnd();
	}

    void RenderSilhouette( const math::Vector & light, math::Vector a, math::Vector b )
    {
        // determine edge normals

        math::Vector midpoint = ( a + b ) * 0.5f;
        
        math::Vector leftNormal;

        if ( midpoint.x > math::epsilon || midpoint.x < -math::epsilon )
            leftNormal = math::Vector( midpoint.x, 0, 0 );
        else
            leftNormal = math::Vector( 0, midpoint.y, 0 );

        math::Vector rightNormal = midpoint - leftNormal;

        // check if silhouette edge

        const math::Vector differenceA = a - light;

        const float leftDot = leftNormal.dot( differenceA );
		const float rightDot = rightNormal.dot( differenceA );

        if ( ( leftDot < 0 && rightDot > 0 ) || ( leftDot > 0 && rightDot < 0 ) )
        {
            // extrude quad

            const math::Vector differenceB = b - light;

            math::Vector _a = a + differenceA * ShadowDistance;
            math::Vector _b = b + differenceB * ShadowDistance;

            // ensure correct winding order for silhouette edge

            const math::Vector cross = ( b - a ).cross( differenceA );
            
            if ( cross.dot( a ) < 0 )
            {
                math::Vector t = a;
                a = b;
                b = t;

                t = _a;
                _a = _b;
                _b = t;
            }

            // render extruded quad

            glVertex3f( b.x, b.y, b.z );
            glVertex3f( a.x, a.y, a.z ); 
            glVertex3f( _a.x, _a.y, _a.z ); 
            glVertex3f( _b.x, _b.y, _b.z );
        }
    }
	
	void RenderShadow()
	{
		#ifndef DEBUG_SHADOW_VOLUMES
		
		EnterScreenSpace();
	
		glDisable( GL_DEPTH_TEST );

		glEnable( GL_STENCIL_TEST );

		glStencilFunc( GL_NOTEQUAL, 0x0, 0xff );
		glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

		glColor4f( 0.0f, 0.0f, 0.0f, 0.15f );

		glBegin( GL_QUADS );

			glVertex2f( 0, 0 );
			glVertex2f( 0, 1 );
			glVertex2f( 1, 1 );
			glVertex2f( 1, 0 );

		glEnd();

		glDisable( GL_BLEND );

		glDisable( GL_STENCIL_TEST );
		
		LeaveScreenSpace();
		
		#endif
	}
	
protected:
	
	math::Vector InverseTransform( const math::Vector & input, const math::Vector & position, const math::Quaternion & orientation, float scale )
	{
		assert( scale >= 0.0f );
		
		math::Vector output = input - position;
		output /= scale;
		output = orientation.inverse().transform( output );
		
		return output;
	}
	
	void EnterScreenSpace()
	{
	    glMatrixMode( GL_PROJECTION );
	    glPushMatrix();
	    glLoadIdentity();
	    glOrtho( 0, 1, 0, 1, 1, -1 );

	    glMatrixMode( GL_MODELVIEW );
	    glPushMatrix();
	    glLoadIdentity();
	}
	
	void LeaveScreenSpace()
	{
	    glMatrixMode( GL_PROJECTION );
	    glPopMatrix();

	    glMatrixMode( GL_MODELVIEW );
	    glPopMatrix();
	}
		
	void PushTransform( const math::Vector & position, const math::Quaternion & orientation, float scale )
	{
		glPushMatrix();

		glTranslatef( position.x, position.y, position.z ); 

		const float pi = 3.1415926f;

		float angle;
		math::Vector axis;
		orientation.axisAngle( axis, angle );
		glRotatef( angle / pi * 180, axis.x, axis.y, axis.z );

		glScalef( scale, scale, scale );
	}
	
	void PopTransform()
	{
		glPopMatrix();
	}

	struct Frustum
	{
		math::Plane left, right, front, back, top, bottom;
	};
	
	void FrustumCullCubes( RenderState & renderState )
	{
		Frustum frustum;
		CalculateFrustumPlanes( frustum );
		
		for ( int i = 0; i < (int) renderState.numCubes; ++i )
		{
			float radius = renderState.cubes[i].scale * 2.0f;
			
			math::Vector point( renderState.cubes[i].position.x, 
			                    renderState.cubes[i].position.y,
			                    renderState.cubes[i].position.z );
			
			cubeData[i].culled = true;
			
			float distance_left = frustum.left.distance( point );
			if ( distance_left < -radius )
				continue;
			
			float distance_right = frustum.right.distance( point );
			if ( distance_right < -radius )
				continue;
						
			float distance_front = frustum.front.distance( point );
			if ( distance_front < -radius )
				continue;
			
			float distance_back = frustum.back.distance( point );
			if ( distance_back < -radius )
				continue;

			float distance_top = frustum.top.distance( point );
			if ( distance_top < -radius )
				continue;
			
			float distance_bottom = frustum.bottom.distance( point );
			if ( distance_bottom < -radius )
				continue;

			cubeData[i].culled = false;
		}
	}

	void CalculateFrustumPlanes( Frustum & frustum )
	{
		math::Matrix projection;
		glGetFloatv(GL_PROJECTION_MATRIX, projection.data());

		math::Matrix modelview;
		glGetFloatv(GL_MODELVIEW_MATRIX, modelview.data());

		math::Matrix clip = modelview * projection;

		frustum.left.normal.x = clip(0,3) + clip(0,0);
		frustum.left.normal.y = clip(1,3) + clip(1,0);
		frustum.left.normal.z = clip(2,3) + clip(2,0);
		frustum.left.constant = - (clip(3,3) + clip(3,0));
		frustum.left.normalize();

		frustum.right.normal.x = clip(0,3) - clip(0,0);
		frustum.right.normal.y = clip(1,3) - clip(1,0);
		frustum.right.normal.z = clip(2,3) - clip(2,0);
		frustum.right.constant = - (clip(3,3) - clip(3,0));
		frustum.right.normalize();	

		frustum.bottom.normal.x = clip(0,3) + clip(0,1);
		frustum.bottom.normal.y = clip(1,3) + clip(1,1);
		frustum.bottom.normal.z = clip(2,3) + clip(2,1);
		frustum.bottom.constant = - clip(3,3) + clip(3,1);
		frustum.bottom.normalize();

		frustum.top.normal.x = clip(0,3) - clip(0,1);
		frustum.top.normal.y = clip(1,3) - clip(1,1);
		frustum.top.normal.z = clip(2,3) - clip(2,1);
		frustum.top.constant = - (clip(3,3) - clip(3,1));
		frustum.top.normalize();

		frustum.front.normal.x = clip(0,3) + clip(0,2);
		frustum.front.normal.y = clip(1,3) + clip(1,2);
		frustum.front.normal.z = clip(2,3) + clip(2,2);
		frustum.front.constant = - (clip(3,3) + clip(3,2));
		frustum.front.normalize();

		frustum.back.normal.x = clip(0,3) - clip(0,2);
		frustum.back.normal.y = clip(1,3) - clip(1,2);
		frustum.back.normal.z = clip(2,3) - clip(2,2);
		frustum.back.constant = - (clip(3,3) - clip(3,2));
		frustum.back.normalize();
	}
			
private:
	
	int displayWidth;
	int displayHeight;
	
	int cubeDisplayList;

	math::Vector cameraPosition;
	math::Vector cameraLookAt;

	math::Vector lightPosition;
	
	struct CubeData
	{
		CubeData()
		{
			culled = false;
		}

		bool culled;
	};
	
	CubeData cubeData[MaxCubes];
};

#endif
