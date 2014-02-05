/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#ifndef RENDER_H
#define RENDER_H

const int DisplayWidth = 800;
const int DisplayHeight = 600;
const float ColorChangeTightness = 0.1f;
const float ShadowAlphaThreshold = 0.25f;
const int MaxObjectsToRender = 1024;
const int NumDynamicVBOs = 1;
const int VBOSize = (24*3+24*3+24*4)*MaxObjectsToRender;
const int FlashFrames = 10;

#include "Platform.h"
#include <map>

namespace render
{
	using namespace platform;
	
	// ----------------------------

	GLfloat vertices[] =
	{
		-1, -1, +1,
		-1, +1, +1,
		+1, +1, +1,
		+1, -1, +1,
		-1, -1, -1,
		+1, -1, -1,
		+1, +1, -1,
		-1, +1, -1,
	};

	GLfloat normals[] =
	{
		0,  0, +1,
		0,  0, +1,
		0,  0, +1,
		0,  0, +1,

		0,  0, -1,
		0,  0, -1,
		0,  0, -1,
		0,  0, -1,

		0, +1,  0,
		0, +1,  0,
		0, +1,  0,
		0, +1,  0,

		0, -1,  0,
		0, -1,  0,
		0, -1,  0,
		0, -1,  0,

		+1,  0,  0,
		+1,  0,  0,
		+1,  0,  0,
		+1,  0,  0,

		-1,  0,  0,
		-1,  0,  0,
		-1,  0,  0,
		-1,  0,  0
	};

	// -----------------------------------------

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
			math::Vector position;
			math::Quaternion orientation;
			float scale;
			float r,g,b,a;
			float target_r, target_g, target_b;
			bool operator < ( const Cube & other ) const { return position.y > other.position.y; }		// for back to front sort only!
		};

		int numCubes;
		Cube cube[MaxObjectsToRender];
	};

	// renderer

	class Render
	{
	public:
	
		Render( int displayWidth, int displayHeight )
		{
			this->displayWidth = displayWidth;
			this->displayHeight = displayHeight;
		
			cameraPosition = math::Vector( 0.0f, -15.0f, 4.0f );
			cameraLookAt = math::Vector( 0.0f, 0.0f, 0.0f );
			cameraUp = math::Vector( 0.0f, 0.0f, 1.0f );

			lightPosition = math::Vector( 25.0f, -25.0f, 50.0f );
		
	        // create vertex buffer objects

	        glGenBuffersARB( 1, &static_vbo );
			/*
	        glBindBufferARB( GL_ARRAY_BUFFER_ARB, static_vbo );
	        glBufferDataARB( GL_ARRAY_BUFFER_ARB, sizeof(vertices)+sizeof(normals)+sizeof(colors), 0, GL_STATIC_DRAW_ARB );
	        glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 0, sizeof(vertices), vertices );                             // copy vertices starting from 0 offest
	        glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, sizeof(vertices), sizeof(normals), normals );                // copy normals after vertices
	        glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, sizeof(vertices)+sizeof(normals), sizeof(colors), colors );  // copy colours after normals
			*/

			dynamic_index = 0;
			for ( int i = 0; i < NumDynamicVBOs; ++i )
			{
				dynamic_vbo[i] = new DynamicVBO();
		        glGenBuffersARB( 1, &dynamic_vbo[i]->id );
			}
		}
	
		~Render()
		{
		    glDeleteBuffersARB( 1, &static_vbo );
			for ( int i = 0; i < NumDynamicVBOs; ++i )
			{
		    	glDeleteBuffersARB( 1, &dynamic_vbo[i]->id );
				delete dynamic_vbo[i];
			}
		}
		
		int GetDisplayWidth() const
		{
			return displayWidth;
		}
		
		int GetDisplayHeight() const
		{
			return displayHeight;
		}
	
		void SetLightPosition( const math::Vector & position )
		{
			lightPosition = position;
		}
	
		void SetCamera( const math::Vector & position, const math::Vector & lookAt, const math::Vector & up )
		{
			cameraPosition = position;
			cameraLookAt = lookAt;
			cameraUp = up;
		}
	
		void ClearScreen()
		{
			glViewport( 0, 0, displayWidth, displayHeight );
			glDisable( GL_SCISSOR_TEST );
			glClearStencil( 0 );
			glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );		
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		}

		void BeginScene( float x1, float y1, float x2, float y2 )
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
				
			// perform frustum culling on cubes
		
			/*
			#ifdef FRUSTUM_CULLING
			RenderState culledRenderState = renderState;
			FrustumCullCubes( culledRenderState );
			#endif
			*/
							
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
    
			// enable lighting etc...

			glEnable( GL_LIGHTING );
		}
	 
		void RenderFrameTime( float renderTime, float simTime[], int maxPlayers, float frameTime )
		{
			glDisable( GL_DEPTH_TEST );

			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

			const float w = 0.5f;
			const float h = 0.01f;
		
			const float renderFraction = renderTime / frameTime;

			glColor4f( 1.0f, 0.0f, 1.0f, 0.5f );

			glBegin( GL_QUADS );

				glVertex2f( 0, 1.0f - h );
				glVertex2f( 0, 1.0f );
				glVertex2f( renderFraction * w, 1.0f );
				glVertex2f( renderFraction * w, 1.0f - h );

			glEnd();

			float base = 1.0f - h;
		
			for ( int i = 0; i < maxPlayers; ++i )
			{
				const float simFraction = simTime[i] / frameTime;

				switch ( i )
				{
					case 0:	glColor4f( 1.0f/1.5, 0.15f/1.5, 0.15f/1.5, 0.75f ); break;
					case 1: glColor4f( 0.3f/1.5, 0.3f/1.5, 1.0f/1.5, 0.75f ); break;
					case 2: glColor4f( 0.0f/1.5, 1.0f/1.5, 0.0f, 0.75f ); break;
					case 3: glColor4f( 1.0f/1.5, 1.0f/1.5, 0.0f, 0.75f ); break;
					default: break;
				} 

				glBegin( GL_QUADS );

					glVertex2f( 0, base - h );
					glVertex2f( 0, base );
					glVertex2f( simFraction * w, base );
					glVertex2f( simFraction * w, base - h );

				glEnd();
			
				base -= h;
			}

			glDisable( GL_BLEND );

			LeaveScreenSpace();
		}

		void RenderDroppedFrames( int simDroppedFrames, int netDroppedFrames, int viewDroppedFrames )
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

			LeaveScreenSpace();
		}

		void RenderActivationArea( const ActivationArea & activationArea, float alpha )
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

			const int steps = 40;
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

		void RenderCubes( const Cubes & cubes, float alpha = 1.0f )
		{
			assert( cubes.numCubes < MaxObjectsToRender );

		    glEnable( GL_CULL_FACE );
		    glCullFace( GL_BACK );
		    glFrontFace( GL_CW );

			if ( cubes.numCubes > 0 )
			{
				DynamicVBO & vbo = GetDynamicVBO();
			
				int total_bytes = cubes.numCubes * (24*3+24*3+24*4) * sizeof(float);
				int vertex_bytes = cubes.numCubes * 24*3 * sizeof(float);
				int normal_bytes = cubes.numCubes * 24*3 * sizeof(float);
				int color_bytes = cubes.numCubes * 24*4 * sizeof(float);
			
				int normal_offset = cubes.numCubes * 24*3 * sizeof(float);
				int color_offset = normal_offset * 2;
			
				GLfloat * dynamic_vertices = vbo.data;
				GLfloat * dynamic_normals = vbo.data + cubes.numCubes * 24*3;
				GLfloat * dynamic_colors = dynamic_normals + cubes.numCubes * 24*3;
			
				int stride = 24*3;
				int index = 0;
				int color_index = 0;
				for ( int i = 0; i < (int) cubes.numCubes; ++i )
				{
					math::Matrix matrix;
					cubes.cube[i].orientation.to_matrix( matrix );
				
					const float x = cubes.cube[i].position.x;
					const float y = cubes.cube[i].position.y;
					const float z = cubes.cube[i].position.z;
					const float s = cubes.cube[i].scale * 0.5f;
				
					int vertex_index = 0;
					for ( int j = 0; j < 8; ++j )
					{
						math::Vector in( vertices[vertex_index], vertices[vertex_index+1], vertices[vertex_index+2] );
						math::Vector out;
						matrix.transform3x3( in, out );
										
						dynamic_vertices[vertex_index+index+0] = x + out.x * s;
						dynamic_vertices[vertex_index+index+1] = y + out.y * s;
						dynamic_vertices[vertex_index+index+2] = z + out.z * s;
					
						vertex_index += 3;
					}

					#define COPY_VERTEX( to, from ) dynamic_vertices[index+to*3]   = dynamic_vertices[index+from*3];		\
					                                dynamic_vertices[index+to*3+1] = dynamic_vertices[index+from*3+1];		\
													dynamic_vertices[index+to*3+2] = dynamic_vertices[index+from*3+2];      \

					COPY_VERTEX( 8, 7 );
					COPY_VERTEX( 9, 6 );
					COPY_VERTEX( 10, 2 );
					COPY_VERTEX( 11, 1 );
					
					COPY_VERTEX( 12, 4 );
					COPY_VERTEX( 13, 0 );
					COPY_VERTEX( 14, 3 );
					COPY_VERTEX( 15, 5 );

					COPY_VERTEX( 16, 5 );
					COPY_VERTEX( 17, 3 );
					COPY_VERTEX( 18, 2 );
					COPY_VERTEX( 19, 6 );

					COPY_VERTEX( 20, 4 );
					COPY_VERTEX( 21, 7 );
					COPY_VERTEX( 22, 1 );
					COPY_VERTEX( 23, 0 );
				
					int normal_index = 0;
					for ( int j = 0; j < 24; j += 4 )
					{
						// todo: transform without requiring vector conversion...
						math::Vector in( normals[normal_index], normals[normal_index+1], normals[normal_index+2] );
						math::Vector out;
						matrix.transform3x3( in, out );
					
						dynamic_normals[normal_index+index] = out.x;
						dynamic_normals[normal_index+index+1] = out.y;
						dynamic_normals[normal_index+index+2] = out.z;

						dynamic_normals[normal_index+index+3] = out.x;
						dynamic_normals[normal_index+index+4] = out.y;
						dynamic_normals[normal_index+index+5] = out.z;

						dynamic_normals[normal_index+index+6] = out.x;
						dynamic_normals[normal_index+index+7] = out.y;
						dynamic_normals[normal_index+index+8] = out.z;

						dynamic_normals[normal_index+index+9] = out.x;
						dynamic_normals[normal_index+index+10] = out.y;
						dynamic_normals[normal_index+index+11] = out.z;
					
						normal_index += 3 * 4;
					}

					const float r = cubes.cube[i].r;
					const float g = cubes.cube[i].g;
					const float b = cubes.cube[i].b;
					const float a = cubes.cube[i].a;
				
					for ( int j = 0; j < 24; ++j )
					{
						dynamic_colors[color_index] = r;
						dynamic_colors[color_index+1] = g;
						dynamic_colors[color_index+2] = b;
						dynamic_colors[color_index+3] = a * alpha;
						color_index += 4;
					}
				
					index += stride;
				}
			
				glEnable( GL_COLOR_MATERIAL );
				glColorMaterial ( GL_FRONT, GL_AMBIENT_AND_DIFFUSE ) ;
			
		        glBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo.id );

		        glBufferDataARB( GL_ARRAY_BUFFER_ARB, total_bytes, 0, GL_STREAM_DRAW_ARB );
		        glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 0, vertex_bytes, dynamic_vertices );
		        glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, normal_offset, normal_bytes, dynamic_normals );
		        glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, color_offset, color_bytes, dynamic_colors );

				glEnableClientState( GL_VERTEX_ARRAY );
				glEnableClientState( GL_NORMAL_ARRAY );
				glEnableClientState( GL_COLOR_ARRAY );
		
		        glVertexPointer( 3, GL_FLOAT, 0, 0 );
		        glNormalPointer( GL_FLOAT, 0, (void*) normal_offset );
		        glColorPointer( 4, GL_FLOAT, 0, (void*) color_offset );

				glDrawArrays( GL_QUADS, 0, 24 * cubes.numCubes );

				glDisableClientState( GL_VERTEX_ARRAY );
				glDisableClientState( GL_NORMAL_ARRAY );
				glDisableClientState( GL_COLOR_ARRAY );

		        glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );

				glDisable( GL_COLOR_MATERIAL );
			}
		}
		
		void RenderCubeShadows( const Cubes & cubes )
		{
			RenderCubeShadowVolumes( cubes, lightPosition );
		}
	
		void RenderCubeShadowVolumes( const Cubes & cubes, const math::Vector & lightPosition )
		{
			#ifdef VISUALIZE_SHADOW_VOLUMES
	
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

			// new dynamic VBO shadows

			DynamicVBO & vbo = GetDynamicVBO();

			GLfloat * dynamic_vertices = vbo.data;
			GLfloat * vertex_ptr = dynamic_vertices;

			for ( int i = 0; i < (int) cubes.numCubes; ++i )
			{
				const Cubes::Cube & cube = cubes.cube[i];

				if ( cubeData[i].culled || cube.a < ShadowAlphaThreshold )
					continue;

				math::Matrix matrix;
				cube.orientation.to_matrix( matrix );

				const float scale = cube.scale * 0.5f;

				math::Vector light = InverseTransform( lightPosition, cube.position, cube.orientation, cube.scale );

		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(-1,+1,-1), math::Vector(+1,+1,-1) );
		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(+1,+1,-1), math::Vector(+1,+1,+1) );
		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(+1,+1,+1), math::Vector(-1,+1,+1) );
		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(-1,+1,+1), math::Vector(-1,+1,-1) );

		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(-1,-1,-1), math::Vector(+1,-1,-1) );
				RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(+1,-1,-1), math::Vector(+1,-1,+1) );
		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(+1,-1,+1), math::Vector(-1,-1,+1) );
				RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(-1,-1,+1), math::Vector(-1,-1,-1) );

		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(-1,+1,-1), math::Vector(-1,-1,-1) );
		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(+1,+1,-1), math::Vector(+1,-1,-1) );
		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(+1,+1,+1), math::Vector(+1,-1,+1) );
		        RenderSilhouette( vertex_ptr, matrix, cube.position, scale, light, math::Vector(-1,+1,+1), math::Vector(-1,-1,+1) );
			}
		
			int vertex_count = vertex_ptr - dynamic_vertices;

			if ( vertex_count > 0 )
			{
				int vertex_bytes = vertex_count * sizeof(float);
		        glBindBufferARB( GL_ARRAY_BUFFER_ARB, vbo.id );
		        glBufferDataARB( GL_ARRAY_BUFFER_ARB, vertex_bytes, dynamic_vertices, GL_STREAM_DRAW_ARB );
				glEnableClientState( GL_VERTEX_ARRAY );
		        glVertexPointer( 3, GL_FLOAT, 0, 0 );
				glDrawArrays( GL_QUADS, 0, vertex_count / 3 );
				glDisableClientState( GL_VERTEX_ARRAY );
		        glBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
			}
		
			#ifdef VISUALIZE_SHADOW_VOLUMES
		
				glDepthMask( GL_TRUE);
			
			#else
		
				glCullFace( GL_BACK );
				glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
				glDepthMask( GL_TRUE);

				glDisable( GL_STENCIL_TEST );
				glDisable( GL_STENCIL_TEST_TWO_SIDE_EXT );
		
			#endif
		}

		void RenderSilhouette( GLfloat *& vertex_ptr,
							   const math::Matrix & matrix, const math::Vector & position, float scale,
							   const math::Vector & light, math::Vector a, math::Vector b )
	    {
	        // determine edge normals

	        math::Vector midpoint = ( a + b ) * 0.5f;
        
	        math::Vector leftNormal;

			const float epsilon = 0.00001f;

	        if ( midpoint.x > epsilon || midpoint.x < -epsilon )
	            leftNormal = math::Vector( midpoint.x, 0, 0 );
	        else
	            leftNormal = math::Vector( 0, midpoint.y, 0 );

	        math::Vector rightNormal = midpoint - leftNormal;

	        // check if silhouette edge

			math::Vector difference = midpoint - light;

			const float leftDot = leftNormal.dot( difference );
			const float rightDot = rightNormal.dot( difference );

	        if ( ( leftDot < 0 && rightDot > 0 ) || ( leftDot > 0 && rightDot < 0 ) )
	        {
	            // ensure correct winding order for silhouette edge

	            const math::Vector cross = ( b - midpoint ).cross( difference );
            
	            if ( cross.dot( midpoint ) < 0 )
	            {
	                math::Vector tmp = a;
	                a = b;
	                b = tmp;
	            }

	            // transform into world space

				math::Vector transformed_a, transformed_b;
				matrix.transform3x3( a, transformed_a );
				matrix.transform3x3( b, transformed_b );
			
				transformed_a = position + transformed_a * scale;
				transformed_b = position + transformed_b * scale;

				// extrude to ground plane z=0 in world space
			
				math::Vector differenceA = transformed_a - lightPosition;
				math::Vector differenceB = transformed_b - lightPosition;

				const float at = lightPosition.z / differenceA.z;
				const float bt = lightPosition.z / differenceB.z;
			
				math::Vector extruded_a = lightPosition - differenceA * at;
				math::Vector extruded_b = lightPosition - differenceB * bt;
			
				// emit extruded quad

				vertex_ptr[0] = transformed_b.x;
				vertex_ptr[1] = transformed_b.y;
				vertex_ptr[2] = transformed_b.z;

				vertex_ptr[3] = transformed_a.x;
				vertex_ptr[4] = transformed_a.y;
				vertex_ptr[5] = transformed_a.z;

				vertex_ptr[6] = extruded_a.x;
				vertex_ptr[7] = extruded_a.y;
				vertex_ptr[8] = extruded_a.z;

				vertex_ptr[9] = extruded_b.x;
				vertex_ptr[10] = extruded_b.y;
				vertex_ptr[11] = extruded_b.z;
			
				vertex_ptr += 12;
	        }
	    }

		void RenderShadowQuad()
		{
			#if !defined( DEBUG_SHADOW_VOLUMES )

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
	
		void DivideSplitscreen()
		{
			float width = displayWidth;
			float height = displayHeight;
			
			glViewport( 0, 0, width, height );
			glDisable( GL_SCISSOR_TEST );
		
			const float s = width / 2;
			const float h = ( height - s ) / 2;
			const float thickness = width * 0.0015f;
		
			EnterRealScreenSpace( width, height );
		
			glDisable( GL_DEPTH_TEST );
			glDisable( GL_BLEND );

			glColor4f( 0, 0, 0, 1 );

			glBegin( GL_QUADS );
		
				glVertex2f( s - thickness, 0 );
				glVertex2f( s + thickness, 0 );
				glVertex2f( s + thickness, height );
				glVertex2f( s - thickness, height );

				glVertex2f( 0, 0 );
				glVertex2f( width, 0 );
				glVertex2f( width, h );
				glVertex2f( 0, h );

				glVertex2f( 0, height - h );
				glVertex2f( width, height - h );
				glVertex2f( width, height );
				glVertex2f( 0, height );
		
			glEnd();
		
			LeaveScreenSpace();
		}
	
		void DivideQuadscreen()
		{
			float width = displayWidth;
			float height = displayHeight;
			
			glViewport( 0, 0, width, height );
			glDisable( GL_SCISSOR_TEST );
		
			const float s = height / 2;
			const float w = ( width - s*2 ) / 2;
			const float thickness = width * 0.0015f;
		
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
	
		// ------------------------------------------------------------s
	
		math::Vector InverseTransform( const math::Vector & input, const math::Vector & position, const math::Quaternion & orientation, float scale )
		{
			assert( scale >= 0.0f );
		
			math::Vector output = input - position;
			output /= scale;
			output = orientation.inverse().transform( output );
		
			return output;
		}
		
	public:
	
		void EnterScreenSpace()
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

		void EnterRealScreenSpace( float width, float height )
		{
		    glMatrixMode( GL_PROJECTION );
		    glPushMatrix();
		    glLoadIdentity();
		    glOrtho( 0, width, height, 0, 1, -1 );

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
		
	private:
		
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
	
		void PushTransform( const math::Vector & position, const math::Quaternion & orientation, float scale_x, float scale_y, float scale_z )
		{
			glPushMatrix();

			glTranslatef( position.x, position.y, position.z ); 

			const float pi = 3.1415926f;

			float angle;
			math::Vector axis;
			orientation.axisAngle( axis, angle );
			glRotatef( angle / pi * 180, axis.x, axis.y, axis.z );

			glScalef( scale_x, scale_y, scale_z );
		}
	
		void PopTransform()
		{
			glPopMatrix();
		}

		struct Frustum
		{
			math::Plane left, right, front, back, top, bottom;
		};
	
		/*
		void FrustumCullCubes( RenderState & renderState )
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
	
		CubeData cubeData[MaxObjectsToRender];

		GLuint static_vbo;
	
		struct DynamicVBO
		{
			GLuint id;
			GLfloat data[ VBOSize ];
		};

		int dynamic_index;
		DynamicVBO * dynamic_vbo[NumDynamicVBOs];

		DynamicVBO & GetDynamicVBO()
		{
			dynamic_index ++;
			if ( dynamic_index >= NumDynamicVBOs )
				dynamic_index = 0;
			return *dynamic_vbo[dynamic_index];
		}
	};
}

#endif
