/*
	Networked Physics Demo
	Copyright © 2008-2011 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "PreCompiled.h"
#include "Render.h"

#ifdef HAS_OPENGL

#if PLATFORM == PLATFORM_MAC
#include <OpenGl/gl.h>
#include <OpenGl/glu.h>
#include <OpenGL/glext.h>
#include <OpenGL/OpenGL.h>
#endif

namespace render
{
	using namespace platform;

    struct CubeVertex
    {
        GLfloat x,y,z;
        GLfloat nx,ny,nz;
    };

    CubeVertex cube_vertices[] = 
    {
        { +1, -1, +1, 0, 0, +1 },
        { -1, -1, +1, 0, 0, +1 },
        { -1, +1, +1, 0, 0, +1 },
        { +1, +1, +1, 0, 0, +1 },

        { -1, -1, -1, 0, 0, -1 },
        { +1, -1, -1, 0, 0, -1 },
        { +1, +1, -1, 0, 0, -1 },
        { -1, +1, -1, 0, 0, -1 },
     
        { +1, +1, -1, +1, 0, 0 },
        { +1, -1, -1, +1, 0, 0 },
        { +1, -1, +1, +1, 0, 0 },
        { +1, +1, +1, +1, 0, 0 },
        
        { -1, -1, -1, -1, 0, 0 },
        { -1, +1, -1, -1, 0, 0 },
        { -1, +1, +1, -1, 0, 0 },
        { -1, -1, +1, -1, 0, 0 },

        { -1, +1, -1, 0, +1, 0 },
        { +1, +1, -1, 0, +1, 0 },
        { +1, +1, +1, 0, +1, 0 },
        { -1, +1, +1, 0, +1, 0 },

        { +1, -1, -1, 0, -1, 0 },
        { -1, -1, -1, 0, -1, 0 },
        { -1, -1, +1, 0, -1, 0 },
        { +1, -1, +1, 0, -1, 0 },
    };
    
    struct ShadowVBO
    {
        GLuint id;
        vectorial::vec3f vertices[ MaxShadowVertices ];
    };

	struct RenderImpl
	{
        RenderImpl()
        {
            glGenBuffers( 1, &shadow_vbo.id );
            glGenBuffers( 1, &cube_vbo );
            glBindBuffer( GL_ARRAY_BUFFER, cube_vbo );
            glBufferData( GL_ARRAY_BUFFER, sizeof( cube_vertices ), cube_vertices, GL_STATIC_DRAW );
        }
        
        ~RenderImpl()
        {
            glDeleteBuffers( 1, &shadow_vbo.id );
            glDeleteBuffers( 1, &cube_vbo );
        }

        GLuint cube_vbo;
        ShadowVBO shadow_vbo;
	};

	// -----------------------------------------

	Interface::Interface( int displayWidth, int displayHeight )
	{
		this->displayWidth = displayWidth;
		this->displayHeight = displayHeight;
	
		cameraPosition = math::Vector( 0.0f, -15.0f, 4.0f );
		cameraLookAt = math::Vector( 0.0f, 0.0f, 0.0f );
		cameraUp = math::Vector( 0.0f, 0.0f, 1.0f );

		lightPosition = math::Vector( 25.0f, -25.0f, 50.0f );
	
        // generate cube vbo object

		this->impl = new RenderImpl();
    }

	Interface::~Interface()
	{
		delete impl;
		impl = NULL;
	}
    
    void Interface::ResizeDisplay( int displayWidth, int displayHeight )
    {
        this->displayWidth = displayWidth;
        this->displayHeight = displayHeight;
    }
	
	int Interface::GetDisplayWidth() const
	{
		return displayWidth;
	}
	
	int Interface::GetDisplayHeight() const
	{
		return displayHeight;
	}

	void Interface::SetLightPosition( const math::Vector & position )
	{
		lightPosition = position;
	}

	void Interface::SetCamera( const math::Vector & position, const math::Vector & lookAt, const math::Vector & up )
	{
		cameraPosition = position;
		cameraLookAt = lookAt;
		cameraUp = up;
	}

	void Interface::ClearScreen()
	{
		glViewport( 0, 0, displayWidth, displayHeight );
		glDisable( GL_SCISSOR_TEST );
		glClearStencil( 0 );
		glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );		
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	}

	void Interface::BeginScene( float x1, float y1, float x2, float y2 )
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
				   cameraUp.x, cameraUp.y, cameraUp.z );
									
		// enable alpha blending
	
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
						
		// setup lights

		glEnable( GL_LIGHTING );
        glEnable( GL_LIGHT0 );

        glShadeModel( GL_SMOOTH );

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
	}
 
	void Interface::RenderFrameTime( float renderTime, float simTime, float frameTime )
	{
		glDisable( GL_LIGHTING );
	    glDisable( GL_CULL_FACE );
		glDisable( GL_DEPTH_TEST );

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

		const float w = 0.5f;
		const float h = 0.01f;

		// draw render frame time
	
		const float renderFraction = renderTime / frameTime;

		glColor4f( 1.0f, 0.0f, 1.0f, 0.5f );

		glBegin( GL_QUADS );
		
			glVertex2f( 0, 1.0f - h );
			glVertex2f( 0, 1.0f );
			glVertex2f( renderFraction * w, 1.0f );
			glVertex2f( renderFraction * w, 1.0f - h );
		
		glEnd();

		float base = 1.0f - h;
	
		// draw sim frame time
	
		const float simFraction = simTime / frameTime;

		glColor4f( 0.0f, 0.75f, 0.75f, 0.75f );

		glBegin( GL_QUADS );

			glVertex2f( 0, base - h );
			glVertex2f( 0, base );
			glVertex2f( simFraction * w, base );
			glVertex2f( simFraction * w, base - h );

		glEnd();
	
		base -= h;

		glDisable( GL_BLEND );
		
		// draw guide lines at 100% frame time, 50% frame time, 200% etc...

		glColor4f( 0,0,0,0.5f );

		glBegin( GL_LINES );
		
		glVertex2f( w*0.5f, 1.0f );
		glVertex2f( w*0.5f, 1.0f - h*2 );
		glVertex2f( w, 1.0f );
		glVertex2f( w, 1.0f - h*2 );
		
		glEnd();
	}

	void Interface::RenderDroppedFrames( int simDroppedFrames, int netDroppedFrames, int viewDroppedFrames )
	{
		glDisable( GL_DEPTH_TEST );

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

		glColor4f( 0.0f, 0.0f, 0.5f, 0.5f );

		glBegin( GL_QUADS );

		float sx = 0.005;
		float sy = 0.008;
		float spacing = 0.002f;

		// view dropped frames
		{
			float x = 0.992f;
			float y = 0.987f;

			for ( int i = 0; i < viewDroppedFrames; ++i )
			{
				glVertex2f( x, y );
				glVertex2f( x, y + sy );
				glVertex2f( x + sx, y + sy );
				glVertex2f( x + sx, y );
				y -= sy + spacing;
			}
		}
	
		// net dropped frames
		{
			float x = 0.992f - 0.007;
			float y = 0.987f;

			for ( int i = 0; i < netDroppedFrames; ++i )
			{
				glVertex2f( x, y );
				glVertex2f( x, y + sy );
				glVertex2f( x + sx, y + sy );
				glVertex2f( x + sx, y );
				y -= sy + spacing;
			}
		}

		// sim dropped frames
		{
			float x = 0.992f - 0.007 * 2;
			float y = 0.987f;

			for ( int i = 0; i < simDroppedFrames; ++i )
			{
				glVertex2f( x, y );
				glVertex2f( x, y + sy );
				glVertex2f( x + sx, y + sy );
				glVertex2f( x + sx, y );
				y -= sy + spacing;
			}
		}
	
		glEnd();

		glDisable( GL_BLEND );
	}

	void Interface::RenderActivationArea( const ActivationArea & activationArea, float alpha )
	{
	    glDisable( GL_CULL_FACE );
		glDisable( GL_DEPTH_TEST );

		const float r = 0.2f;
		const float g = 0.2f;
		const float b = 1.0f;
		const float a = 0.1f * alpha;

		glColor4f( r, g, b, a );

        GLfloat color[] = { r, g, b, a };

		glMaterialfv( GL_FRONT, GL_AMBIENT, color );
		glMaterialfv( GL_FRONT, GL_DIFFUSE, color );

		const int steps = 256;
		const float radius = activationArea.radius - 0.1f;

		const math::Vector & origin = activationArea.origin;

		const float deltaAngle = 2*math::pi / steps;

		float angle = 0.0f;

        glBegin( GL_TRIANGLE_FAN );
		glVertex3f( origin.x, origin.y, 0.0f );
		for ( float i = 0; i <= steps; i++ )
		{
			glVertex3f( radius * math::cos(angle) + origin.x, radius * math::sin(angle) + origin.y, 0.0f );
			angle += deltaAngle;
		}
		glEnd();
	
		glEnable( GL_DEPTH_TEST );
		{
			const float r = activationArea.r;
			const float g = activationArea.g;
			const float b = activationArea.b;
			const float a = activationArea.a * alpha;

			glColor4f( r, g, b, a );

	        GLfloat color[] = { r, g, b, a };

			glMaterialfv( GL_FRONT, GL_AMBIENT, color );
			glMaterialfv( GL_FRONT, GL_DIFFUSE, color );

			const int steps = 40;
			const float radius = activationArea.radius;
			const float thickness = 0.1f;
			const float r1 = radius - thickness;
			const float r2 = radius + thickness;

			const math::Vector & origin = activationArea.origin;

			const float bias = 0.001f;
			const float deltaAngle = 2*math::pi / steps;

			float angle = activationArea.startAngle;

	        glBegin( GL_QUADS );
			for ( float i = 0; i < steps; i++ )
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
	}
            
	void Interface::RenderCubes( const Cubes & cubes, float alpha  )
	{
#ifndef DEBUG_SHADOW_VOLUMES
        
        if ( cubes.numCubes > 0 )
		{
            glEnable( GL_CULL_FACE );
            glCullFace( GL_BACK );
            glFrontFace( GL_CW );

			glEnable( GL_COLOR_MATERIAL );
			glColorMaterial ( GL_FRONT, GL_AMBIENT_AND_DIFFUSE ) ;
                        
	        glBindBuffer( GL_ARRAY_BUFFER, impl->cube_vbo );

            glEnable( GL_RESCALE_NORMAL );

			glEnableClientState( GL_VERTEX_ARRAY );
			glEnableClientState( GL_NORMAL_ARRAY );

            #define BUFFER_OFFSET(i) ( (char*)NULL + (i) )
            glVertexPointer( 3, GL_FLOAT, sizeof(CubeVertex), BUFFER_OFFSET(0) );
            glNormalPointer( GL_FLOAT, sizeof(CubeVertex), BUFFER_OFFSET(12) );

            float matrix[16];
            for ( int i = 0; i < cubes.numCubes; ++i )
            {
                glPushMatrix();
                cubes.cube[i].transform.store( matrix );
                glMultMatrixf( matrix );
                glColor4f( cubes.cube[i].r, cubes.cube[i].g, cubes.cube[i].b, cubes.cube[i].a );
                glDrawArrays( GL_QUADS, 0, 6 * 4 );
                glPopMatrix();
            }
            
			glDisableClientState( GL_VERTEX_ARRAY );
			glDisableClientState( GL_NORMAL_ARRAY );
          
	        glBindBuffer( GL_ARRAY_BUFFER, 0 );
            
			glDisable( GL_COLOR_MATERIAL );
            
            glDisable( GL_RESCALE_NORMAL );
		}
        
#endif
	}
    
	inline void RenderSilhouette( int & vertex_index,
                                  vectorial::vec3f * vertices,
                                  vectorial::mat4f transform,
                                  vectorial::vec3f local_light,
                                  vectorial::vec3f world_light,
                                  vectorial::vec3f a, 
                                  vectorial::vec3f b,
                                  vectorial::vec3f left_normal,
                                  vectorial::vec3f right_normal,
                                  bool left_dot,
                                  bool right_dot )
    {
#ifdef DEBUG_SHADOW_VOLUMES
        
        vectorial::vec3f world_a = transformPoint( transform, a );
        vectorial::vec3f world_b = transformPoint( transform, b );
        
        glVertex3f( world_a.x(), world_a.y(), world_a.z() );
        glVertex3f( world_b.x(), world_b.y(), world_b.z() );
        
        {
            vectorial::vec3f world_l = transformPoint( transform, local_light );
            glVertex3f( world_a.x(), world_a.y(), world_a.z() );
            glVertex3f( world_l.x(), world_l.y(), world_l.z() );
            glVertex3f( world_b.x(), world_b.y(), world_b.z() );
            glVertex3f( world_l.x(), world_l.y(), world_l.z() );
        }
        
#endif
        
        // check if silhouette edge
        if ( left_dot ^ right_dot )
        {
            // ensure correct winding order for silhouette edge

            const vectorial::vec3f cross = vectorial::cross( b - a, local_light );
            if ( dot( cross, a ) < 0 )
            {
                vectorial::vec3f tmp = a;
                a = b;
                b = tmp;
            }
            
            // transform into world space
            
            vectorial::vec3f world_a = transformPoint( transform, a );
            vectorial::vec3f world_b = transformPoint( transform, b );
            
			// extrude to ground plane z=0 in world space
            
            vectorial::vec3f difference_a = world_a - world_light;
            vectorial::vec3f difference_b = world_b - world_light;
            
			const float at = world_light.z() / difference_a.z();
			const float bt = world_light.z() / difference_b.z();
            
            vectorial::vec3f extruded_a = world_light - difference_a * at;
            vectorial::vec3f extruded_b = world_light - difference_b * bt;
            
			// emit extruded quad
            
            vertices[vertex_index]   = world_b;
            vertices[vertex_index+1] = world_a;
            vertices[vertex_index+2] = extruded_a;
            vertices[vertex_index+3] = extruded_b;
            
			vertex_index += 4;
        }
    }

	void Interface::RenderCubeShadows( const Cubes & cubes )
	{
		#ifdef DEBUG_SHADOW_VOLUMES

            glDepthMask( GL_FALSE );
	        glDisable( GL_CULL_FACE );  
			glColor3f( 1.0f, 0.75f, 0.8f );

		#else

			glDepthMask(0);  
	        glColorMask(0,0,0,0);  
	        glDisable(GL_CULL_FACE);  
	        glEnable(GL_STENCIL_TEST);  
	        glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);  

	        glActiveStencilFaceEXT(GL_BACK);  
	        glStencilOp(GL_KEEP,            // stencil test fail  
	                    GL_KEEP,            // depth test fail  
	                    GL_DECR_WRAP_EXT);  // depth test pass  
	        glStencilMask(~0);  
	        glStencilFunc(GL_ALWAYS, 0, ~0);  

	        glActiveStencilFaceEXT(GL_FRONT);  
	        glStencilOp(GL_KEEP,            // stencil test fail  
	                    GL_KEEP,            // depth test fail  
	                    GL_INCR_WRAP_EXT);  // depth test pass  
	        glStencilMask(~0);  
	        glStencilFunc(GL_ALWAYS, 0, ~0);  

		#endif

        vectorial::vec3f world_light( lightPosition.x, lightPosition.y, lightPosition.z );
        
        int vertex_index = 0;
        vectorial::vec3f * vertices = impl->shadow_vbo.vertices;
        
        #ifdef DEBUG_SHADOW_VOLUMES
        glPushMatrix();
        float matrix[16];
        cube.transform.store( matrix );
        glColor3f( 1, 0, 0 );
        glBegin( GL_LINES );
        #endif
        
        vectorial::vec3f a(-1,+1,-1);
        vectorial::vec3f b(+1,+1,-1);
        vectorial::vec3f c(+1,+1,+1);
        vectorial::vec3f d(-1,+1,+1);
        vectorial::vec3f e(-1,-1,-1);
        vectorial::vec3f f(+1,-1,-1);
        vectorial::vec3f g(+1,-1,+1);
        vectorial::vec3f h(-1,-1,+1);
        
        vectorial::vec3f normals[6] =
        {
            vectorial::vec3f(+1,0,0),
            vectorial::vec3f(-1,0,0),
            vectorial::vec3f(0,+1,0),
            vectorial::vec3f(0,-1,0),
            vectorial::vec3f(0,0,+1),
            vectorial::vec3f(0,0,-1)
        };

		for ( int i = 0; i < (int) cubes.numCubes; ++i )
		{
			const Cubes::Cube & cube = cubes.cube[i];

			if ( cubeData[i].culled || cube.a < ShadowAlphaThreshold )
				continue;

            vectorial::vec3f local_light = transformPoint( cube.inverse_transform, world_light );
            
            bool dot[6];
            dot[0] = vectorial::dot( normals[0], local_light ) > 0;
            dot[1] = vectorial::dot( normals[1], local_light ) > 0;
            dot[2] = vectorial::dot( normals[2], local_light ) > 0;
            dot[3] = vectorial::dot( normals[3], local_light ) > 0;
            dot[4] = vectorial::dot( normals[4], local_light ) > 0;
            dot[5] = vectorial::dot( normals[5], local_light ) > 0;
            
	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, a, b, normals[5], normals[2], dot[5], dot[2] );
	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, b, c, normals[0], normals[2], dot[0], dot[2] );
	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, c, d, normals[4], normals[2], dot[4], dot[2] );
	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, d, a, normals[1], normals[2], dot[1], dot[2] );
            
	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, e, f, normals[3], normals[5], dot[3], dot[5] );
			RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, f, g, normals[3], normals[0], dot[3], dot[0] );
	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, g, h, normals[3], normals[4], dot[3], dot[4] );
			RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, h, e, normals[3], normals[1], dot[3], dot[1] );

	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, a, e, normals[1], normals[5], dot[1], dot[5] );
	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, b, f, normals[5], normals[0], dot[5], dot[0] );
	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, c, g, normals[0], normals[4], dot[0], dot[4] );
	        RenderSilhouette( vertex_index, vertices, cube.transform, local_light, world_light, d, h, normals[4], normals[1], dot[4], dot[1] );
		}
        
        #ifdef DEBUG_SHADOW_VOLUMES
        glEnd();
        glPopMatrix();
        #endif

        int vertex_count = vertex_index;
        
        if ( vertex_count > 0 )
        {
            int vertex_bytes = vertex_count * sizeof( vectorial::vec3f );
            glBindBufferARB( GL_ARRAY_BUFFER_ARB, impl->shadow_vbo.id );
            glBufferDataARB( GL_ARRAY_BUFFER_ARB, vertex_bytes, vertices, GL_STREAM_DRAW_ARB );
            glEnableClientState( GL_VERTEX_ARRAY );
            glVertexPointer( 3, GL_FLOAT, sizeof( vectorial::vec3f ), NULL );
            glDrawArrays( GL_QUADS, 0, vertex_count );
            glDisableClientState( GL_VERTEX_ARRAY );
            glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
        }

        #ifndef DEBUG_SHADOW_VOLUMES
        	
			glCullFace( GL_BACK );
			glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
			glDepthMask( GL_TRUE);

			glDisable( GL_STENCIL_TEST );
			glDisable( GL_STENCIL_TEST_TWO_SIDE_EXT );

        #else
        
            glDepthMask( GL_TRUE );
        
		#endif
    }
    
	void Interface::RenderShadowQuad()
	{
		#if !defined DEBUG_SHADOW_VOLUMES

        EnterScreenSpace();

		glDisable( GL_LIGHTING );
		glDisable( GL_DEPTH_TEST );

		glEnable( GL_STENCIL_TEST );

		glStencilFunc( GL_NOTEQUAL, 0x0, 0xff );
		glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

		glColor4f( 0.0f, 0.0f, 0.0f, 0.1f );

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

	void Interface::DivideSplitscreen()
	{
		float width = displayWidth;
		float height = displayHeight;
		
		glViewport( 0, 0, width, height );
		glDisable( GL_SCISSOR_TEST );
	
		const float s = width / 2;
//		const float h = ( height - s ) / 2;
		const float thickness = width * 0.0025f;
	
		EnterRealScreenSpace( width, height );
	
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_BLEND );

		glColor4f( 0, 0, 0, 1 );

		glBegin( GL_QUADS );
	
			glVertex2f( s - thickness, 0 );
			glVertex2f( s + thickness, 0 );
			glVertex2f( s + thickness, height );
			glVertex2f( s - thickness, height );

            /*
			glVertex2f( 0, 0 );
			glVertex2f( width, 0 );
			glVertex2f( width, h );
			glVertex2f( 0, h );

			glVertex2f( 0, height - h );
			glVertex2f( width, height - h );
			glVertex2f( width, height );
			glVertex2f( 0, height );
            */
	
		glEnd();
	
		LeaveScreenSpace();
	}

	void Interface::DivideQuadscreen()
	{
		float width = displayWidth;
		float height = displayHeight;
		
		glViewport( 0, 0, width, height );
		glDisable( GL_SCISSOR_TEST );
	
		const float s = height / 2;
		const float w = ( width - s*2 ) / 2;
        const float thickness = width * 0.0025f;
	
		EnterRealScreenSpace( width, height );
	
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_BLEND );

		glColor4f(0,0,0,1);
	
		glBegin( GL_QUADS );
	
			glVertex2f( 0, s + thickness );
			glVertex2f( 0, s - thickness );
			glVertex2f( width, s - thickness );
			glVertex2f( width, s + thickness );
	
			glVertex2f( width/2 - thickness, 0 );
			glVertex2f( width/2 + thickness, 0 );
			glVertex2f( width/2 + thickness, height );
			glVertex2f( width/2 - thickness, height );

			glVertex2f( 0, 0 );
			glVertex2f( w, 0 );
			glVertex2f( w, height );
			glVertex2f( 0, height );

			glVertex2f( width - w, 0 );
			glVertex2f( width, 0 );
			glVertex2f( width, height );
			glVertex2f( width - w, height );

		glEnd();
	
		LeaveScreenSpace();
	}

    void Interface::LetterBox( float pixels )
    {
        float width = displayWidth;
        float height = displayHeight;
        
        glViewport( 0, 0, width, height );
    
        EnterRealScreenSpace( width, height );
    
        glDisable( GL_SCISSOR_TEST );
        glDisable( GL_DEPTH_TEST );
        glDisable( GL_BLEND );

        glColor4f( 0, 0, 0, 1 );
    
        const float h = pixels / 2;

        glBegin( GL_QUADS );
    
            glVertex2f( 0, 0 );
            glVertex2f( 0, h );
            glVertex2f( width, h );
            glVertex2f( width, 0 );
    
            glVertex2f( 0, height - h );
            glVertex2f( 0, height );
            glVertex2f( width, height );
            glVertex2f( width, height - h );

        glEnd();
    
        LeaveScreenSpace();
    }

	// ------------------------------------------------------------

	math::Vector Interface::InverseTransform( const math::Vector & input, const math::Vector & position, const math::Quaternion & orientation, float scale )
	{
		assert( scale >= 0.0f );
	
		math::Vector output = input - position;
		output /= scale;
		output = orientation.inverse().transform( output );
	
		return output;
	}
	
	void Interface::EnterScreenSpace()
	{
		glViewport( 0, 0, displayWidth, displayHeight );
		glScissor( 0, 0, displayWidth, displayHeight );

	    glMatrixMode( GL_PROJECTION );
	    glPushMatrix();
	    glLoadIdentity();
	    glOrtho( 0, 1, 0, 1, 1, -1 );

	    glMatrixMode( GL_MODELVIEW );
	    glPushMatrix();
	    glLoadIdentity();
	}

	void Interface::EnterRealScreenSpace( float width, float height )
	{
	    glMatrixMode( GL_PROJECTION );
	    glPushMatrix();
	    glLoadIdentity();
	    glOrtho( 0, width, height, 0, 1, -1 );

	    glMatrixMode( GL_MODELVIEW );
	    glPushMatrix();
	    glLoadIdentity();
	}

	void Interface::LeaveScreenSpace()
	{
	    glMatrixMode( GL_MODELVIEW );
	    glPopMatrix();

	    glMatrixMode( GL_PROJECTION );
	    glPopMatrix();
	}
	
	/*
	void Interface::FrustumCullCubes( RenderState & renderState )
	{
		Frustum frustum;
		CalculateFrustumPlanes( frustum );
	
		for ( int i = 0; i < (int) renderState.numCubes; ++i )
		{
			float radius = renderState.cube[i].scale * 2.0f;
		
			math::Vector point( renderState.cube[i].position.x, 
			                    renderState.cube[i].position.y,
			                    renderState.cube[i].position.z );
		
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
	*/

	void Interface::CalculateFrustumPlanes( math::Frustum & frustum )
	{
		math::Matrix projection;
		glGetFloatv( GL_PROJECTION_MATRIX, projection.data() );

		math::Matrix modelview;
		glGetFloatv( GL_MODELVIEW_MATRIX, modelview.data() );

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
}

#endif
