#include "Cubes.h"
#include "Global.h"
#include "Render.h"
#include "Console.h"
#include "ShaderManager.h"

void CubesInternal::Initialize( core::Allocator & allocator, const CubesConfig & config )
{
    this->config = config;

    // create cube simulations

    CORE_ASSERT( config.num_simulations >= 0 );
    CORE_ASSERT( config.num_simulations <= MaxSimulations );

    if ( config.num_simulations > 0 )
    {
        simulation = CORE_NEW_ARRAY( allocator, CubesSimulation, config.num_simulations );

        game::Config game_config;

        game_config.maxObjects = CubeSteps * CubeSteps + MaxPlayers + 1;      // note: +1 because 0 is null (or possibly "world")
        game_config.deactivationTime = 0.25f;
        game_config.cellSize = 2.0f;
        game_config.cellWidth = CubeSteps / game_config.cellSize + 2 * 2;     // note: double so we have some extra space at the edge of the world
        game_config.cellHeight = game_config.cellWidth;
        game_config.activationDistance = 100.0f;

        game_config.simConfig.ERP = 0.25f;
        game_config.simConfig.CFM = 0.001f;
        game_config.simConfig.MaxIterations = 64;
        game_config.simConfig.MaximumCorrectingVelocity = 250.0f;
        game_config.simConfig.ContactSurfaceLayer = 0.01f;
        game_config.simConfig.Elasticity = 0.0f;
        game_config.simConfig.LinearDrag = 0.001f;
        game_config.simConfig.AngularDrag = 0.001f;
        game_config.simConfig.Friction = 200.0f;

        for ( int i = 0; i < config.num_simulations; ++i )
        {
            simulation[i].game_instance = CORE_NEW( allocator, GameInstance, game_config );

            simulation[i].game_instance->InitializeBegin();

            simulation[i].game_instance->AddPlane( math::Vector(0,0,1), 0 );

            AddCube( simulation[i].game_instance, 1, math::Vector(0,0,10) );

            const float origin = -CubeSteps / 2.0f;
            const float z = hypercube::NonPlayerCubeSize / 2.0f;
            const int count = CubeSteps;
            for ( int y = 0; y < count; ++y )
                for ( int x = 0; x < count; ++x )
                    AddCube( simulation[i].game_instance, 0, math::Vector(x+origin,y+origin,z) );

            simulation[i].game_instance->InitializeEnd();

            // todo: players joining and leaving and local player per-sim
            // should definitely be parameterized not hardcoded like this
            simulation[i].game_instance->OnPlayerJoined( 0 );
            simulation[i].game_instance->SetLocalPlayer( 0 );
            simulation[i].game_instance->SetPlayerFocus( 0, 1 );

            simulation[i].game_instance->SetFlag( game::FLAG_Push );
            simulation[i].game_instance->SetFlag( game::FLAG_Pull );
        }
    }
    else
    {
        simulation = nullptr;
    }

#ifdef CLIENT

    // create views

    CORE_ASSERT( config.num_views >= 0 );
    CORE_ASSERT( config.num_views <= MaxViews );

    if ( config.num_views > 0 )
    {
        view = CORE_NEW_ARRAY( allocator, CubesView, config.num_views );
    }
    else
    {
        view = nullptr;
    }

#endif // #ifdef CLIENT
}

void CubesInternal::Free( core::Allocator & allocator )
{
    if ( simulation )
    {
        for ( int i = 0; i < config.num_simulations; ++i )
        {
            CORE_DELETE( allocator, GameInstance, simulation[i].game_instance );
            simulation[i].game_instance = nullptr;
        }

        CORE_DELETE_ARRAY( allocator, simulation, config.num_simulations );

        simulation = nullptr;
    }

#ifdef CLIENT
    if ( view )
    {
        CORE_DELETE_ARRAY( allocator, view, config.num_views );

        view = nullptr;
    }
#endif // #ifdef CLIENT
}

void CubesInternal::AddCube( GameInstance * game_instance, int player, const math::Vector & position )
{
    hypercube::DatabaseObject object;
    cubes::CompressPosition( position, object.position );
    cubes::CompressOrientation( math::Quaternion(1,0,0,0), object.orientation );
    object.enabled = player;
    object.session = 0;             // todo: do we still need sessions?
    object.player = player;
    activation::ObjectId id = game_instance->AddObject( object, position.x, position.y );
    if ( player )
        game_instance->DisableObject( id );
}

extern "C" 
{
    extern unsigned long dRandGetSeed();
    extern void dRandSetSeed( unsigned int seed );
}

void CubesInternal::Update( const CubesUpdateConfig & update_config )
{
    const float deltaTime = global.timeBase.deltaTime;

#ifdef CLIENT
    if ( global.console->IsActive() )
        input = game::Input();
#endif // #ifdef CLIENT

    if ( simulation )
    {
        // update input

#ifdef CLIENT
        for ( int i = 0; i < config.num_simulations; ++i )
        {
            simulation[i].game_instance->SetPlayerInput( 0, update_config.input[i] );
        }
#endif // #ifdef CLIENT

        // update simulations

        for ( int i = 0; i < config.num_simulations; ++i )
        {
            if ( update_config.run_update[i] )
            {
                dRandSetSeed( simulation[i].frame );

                simulation[i].game_instance->Update( deltaTime );

                simulation[i].frame++;
            }
        }
    }

#ifdef CLIENT

    if ( view )
    {
        for ( int i = 0; i < config.num_views; ++i )
        {
            if ( simulation && i < config.num_simulations )
            {
                // todo: we need a mapping describing where the view should get its view packet from
                // either from a simulation, or it should be passed in explicitly by the demo (eg. snapshot interpolation)

                simulation[i].game_instance->GetViewPacket( view[i].packet );
            }

            getViewObjectUpdates( view[i].updates, view[i].packet );
                
            view[i].objects.UpdateObjects( view[i].updates, view[i].packet.objectCount );

            view[i].objects.Update( deltaTime );

            // todo: the current focus object per-view should be parameterized per-view

            view::Object * player = view[i].objects.GetObject( 1 );

            math::Vector origin = player ? player->position : math::Vector(0,0,0);

            math::Vector lookat = origin - math::Vector(0,0,1);

            math::Vector position = lookat + math::Vector(0,-11,5);

            view[i].camera.EaseIn( lookat, position );
        }
    }

#endif // #ifdef CLIENT
}

#ifdef CLIENT

bool CubesInternal::Clear()
{
    render.ClearScreen();

    return true;
}

void CubesInternal::Render( const CubesRenderConfig & render_config )
{
    if ( !view )
        return;

    const int width = global.displayWidth;
    const int height = global.displayHeight;

    if ( render_config.render_mode == CUBES_RENDER_FULLSCREEN )
    {
        CORE_ASSERT( config.num_views >= 1 );

        render.ResizeDisplay( width, height );

        view[0].objects.GetRenderState( view[0].cubes );

        render.BeginScene( 0, 0, width, height );

        render.SetCamera( view[0].camera.position, view[0].camera.lookat, view[0].camera.up );
        
        render.SetLightPosition( view[0].camera.lookat + math::Vector( 25.0f, -50.0f, 100.0f ) );

        render.RenderCubes( view[0].cubes );
        
        render.RenderCubeShadows( view[0].cubes );

        render.RenderShadowQuad();
        
        render.EndScene();
    }
    else if ( render_config.render_mode == CUBES_RENDER_SPLITSCREEN )
    {
        CORE_ASSERT( config.num_views >= 2 );

        render.ResizeDisplay( width/2, height );

        // left view

        view[0].objects.GetRenderState( view[0].cubes );

        render.BeginScene( 0, 0, width/2, height );

        render.SetCamera( view[0].camera.position, view[0].camera.lookat, view[0].camera.up );
        
        render.SetLightPosition( view[0].camera.lookat + math::Vector( 25.0f, -50.0f, 100.0f ) );

        render.RenderCubes( view[0].cubes );
        
        render.RenderCubeShadows( view[0].cubes );
        
        render.EndScene();

        // right view

        view[1].objects.GetRenderState( view[1].cubes );

        render.BeginScene( width/2, 0, width, height );

        render.SetCamera( view[1].camera.position, view[1].camera.lookat, view[1].camera.up );
        
        render.SetLightPosition( view[1].camera.lookat + math::Vector( 25.0f, -50.0f, 100.0f ) );

        render.RenderCubes( view[1].cubes );
        
        render.RenderCubeShadows( view[1].cubes );
        
        render.EndScene();

        // shadow quad on top of all scenes

        render.RenderShadowQuad();

        // todo: splitscreen divider

        // ...
    }
    else if ( render_config.render_mode == CUBES_RENDER_QUADSCREEN )
    {
        // ...
    }
}

bool CubesInternal::KeyEvent( int key, int scancode, int action, int mods )
{
    if ( mods != 0 )
        return false;

    if ( action == GLFW_PRESS || action == GLFW_REPEAT )
    {
        switch ( key )
        {
            case GLFW_KEY_LEFT:     input.left = true;      return true;
            case GLFW_KEY_RIGHT:    input.right = true;     return true;
            case GLFW_KEY_UP:       input.up = true;        return true;
            case GLFW_KEY_DOWN:     input.down = true;      return true;
            case GLFW_KEY_SPACE:    input.push = true;      return true;
            case GLFW_KEY_Z:        input.pull = true;      return true;
        }
    }
    else if ( action == GLFW_RELEASE )
    {
        switch ( key )
        {
            case GLFW_KEY_LEFT:     input.left = false;     return true;
            case GLFW_KEY_RIGHT:    input.right = false;    return true;
            case GLFW_KEY_UP:       input.up = false;       return true;
            case GLFW_KEY_DOWN:     input.down = false;     return true;
            case GLFW_KEY_SPACE:    input.push = false;     return true;
            case GLFW_KEY_Z:        input.pull = false;     return true;
        }
    }

    return false;
}

struct CubeVertex
{
    float x,y,z;
    float nx,ny,nz;
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

uint16_t cube_indices[] = 
{
    0, 1, 2,
    0, 2, 3,

    4, 5, 6,
    4, 6, 7,

    8, 9, 10,
    8, 10, 11,

    12, 13, 14,
    12, 14, 15,

    16, 17, 18,
    16, 18, 19,

    20, 21, 22,
    20, 22, 23,
};

struct DebugVertex
{
    float x,y,z;
    float r,g,b,a;
};

CubesRender::CubesRender()
{
    initialized = false;    
    displayWidth = 0;
    displayHeight = 0;
    cubes_vao = 0;
    cubes_vbo = 0;
    cubes_ibo = 0;
    cubes_instance_buffer = 0;
    shadow_vao = 0;
    shadow_vbo = 0;
    mask_vao = 0;
    mask_vbo = 0;
}

CubesRender::~CubesRender()
{
    glDeleteVertexArrays( 1, &cubes_vao );
    glDeleteBuffers( 1, &cubes_vbo );
    glDeleteBuffers( 1, &cubes_ibo );
    glDeleteBuffers( 1, &cubes_instance_buffer );

    cubes_vao = 0;
    cubes_vbo = 0;
    cubes_ibo = 0;
    cubes_instance_buffer = 0;

    glDeleteVertexArrays( 1, &shadow_vao );
    glDeleteBuffers( 1, &shadow_vbo );

    shadow_vao = 0;
    shadow_vbo = 0;

    glDeleteVertexArrays( 1, &mask_vao );
    glDeleteBuffers( 1, &mask_vbo );

    mask_vao = 0;
    mask_vbo = 0;
}

void CubesRender::Initialize()
{
    CORE_ASSERT( !initialized );

    uint32_t shadow_shader = global.shaderManager->GetShader( "Shadow" );
    uint32_t cubes_shader = global.shaderManager->GetShader( "Cubes" );
    uint32_t debug_shader = global.shaderManager->GetShader( "Debug" );

    if ( !shadow_shader || !cubes_shader || !debug_shader )
        return;

    // setup cubes draw call
    {
        glUseProgram( cubes_shader );

        const int position_location = glGetAttribLocation( cubes_shader, "VertexPosition" );
        const int normal_location = glGetAttribLocation( cubes_shader, "VertexNormal" );
        const int color_location = glGetAttribLocation( cubes_shader, "VertexColor" );
        const int model_location = glGetAttribLocation( cubes_shader, "Model" );
        const int model_view_location = glGetAttribLocation( cubes_shader, "ModelView" );
        const int model_view_projection_location = glGetAttribLocation( cubes_shader, "ModelViewProjection" );

        glGenBuffers( 1, &cubes_vbo );
        glBindBuffer( GL_ARRAY_BUFFER, cubes_vbo );
        glBufferData( GL_ARRAY_BUFFER, sizeof( cube_vertices ), cube_vertices, GL_STATIC_DRAW );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
     
        glGenBuffers( 1, &cubes_ibo );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, cubes_ibo );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( cube_indices ), cube_indices, GL_STATIC_DRAW );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

        glGenBuffers( 1, &cubes_instance_buffer );

        glGenVertexArrays( 1, &cubes_vao );

        glBindVertexArray( cubes_vao );

        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, cubes_ibo );

        glBindBuffer( GL_ARRAY_BUFFER, cubes_vbo );

        if ( position_location >= 0 )
        {
            glEnableVertexAttribArray( position_location );
            glVertexAttribPointer( position_location, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (GLubyte*)0 );
        }

        if ( normal_location >= 0 )
        {
            glEnableVertexAttribArray( normal_location );
            glVertexAttribPointer( normal_location, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), (GLubyte*)(3*4) );
        }

        glBindBuffer( GL_ARRAY_BUFFER, cubes_instance_buffer );

        if ( color_location >= 0 )
        {
            glEnableVertexAttribArray( color_location );
            glVertexAttribPointer( color_location, 4, GL_FLOAT, GL_FALSE, sizeof( CubeInstance ), (void*) ( offsetof( CubeInstance, r ) ) );
            glVertexAttribDivisor( color_location, 1 );
        }

        for ( unsigned int i = 0; i < 4 ; ++i )
        {
            if ( model_location >= 0 )
            {
                glEnableVertexAttribArray( model_location + i );
                glVertexAttribPointer( model_location + i, 4, GL_FLOAT, GL_FALSE, sizeof( CubeInstance ), (void*) ( offsetof( CubeInstance, model ) + ( sizeof(float) * 4 * i ) ) );
                glVertexAttribDivisor( model_location + i, 1 );
            }

            if ( model_view_location >= 0 )
            {
                glEnableVertexAttribArray( model_view_location + i );
                glVertexAttribPointer( model_view_location + i, 4, GL_FLOAT, GL_FALSE, sizeof( CubeInstance ), (void*) ( offsetof( CubeInstance, modelView ) + ( sizeof(float) * 4 * i ) ) );
                glVertexAttribDivisor( model_view_location + i, 1 );
            }

            if ( model_view_projection_location >= 0 )
            {
                glEnableVertexAttribArray( model_view_projection_location + i );
                glVertexAttribPointer( model_view_projection_location + i, 4, GL_FLOAT, GL_FALSE, sizeof( CubeInstance ), (void*) ( offsetof( CubeInstance, modelViewProjection ) + ( sizeof(float) * 4 * i ) ) );
                glVertexAttribDivisor( model_view_projection_location + i, 1 );
            }
        }

        glBindBuffer( GL_ARRAY_BUFFER, 0 );

        glBindVertexArray( 0 );

        glUseProgram( 0 );
    }

    // setup shadow draw call
    {
        glUseProgram( shadow_shader );

        const int position_location = glGetAttribLocation( shadow_shader, "VertexPosition" );

        glGenBuffers( 1, &shadow_vbo );
     
        glGenVertexArrays( 1, &shadow_vao );

        glBindVertexArray( shadow_vao );

        glBindBuffer( GL_ARRAY_BUFFER, shadow_vbo );

        if ( position_location >= 0 )
        {
            glEnableVertexAttribArray( position_location );
            glVertexAttribPointer( position_location, 3, GL_FLOAT, GL_FALSE, sizeof(vectorial::vec3f), (GLubyte*)0 );
        }

        glBindBuffer( GL_ARRAY_BUFFER, 0 );

        glBindVertexArray( 0 );

        glUseProgram( 0 );
    }

    // setup mask draw call
    {
        glUseProgram( debug_shader );

        const int position_location = glGetAttribLocation( debug_shader, "VertexPosition" );
        const int color_location = glGetAttribLocation( debug_shader, "VertexColor" );

        const float shadow_alpha = 0.25f;

        DebugVertex mask_vertices[6] =
        {
            { 0, 0, 0, 0, 0, 0, shadow_alpha },
            { 1, 0, 0, 0, 0, 0, shadow_alpha },
            { 1, 1, 0, 0, 0, 0, shadow_alpha },
            { 0, 0, 0, 0, 0, 0, shadow_alpha },
            { 1, 1, 0, 0, 0, 0, shadow_alpha },
            { 0, 1, 0, 0, 0, 0, shadow_alpha },
        };

        glGenBuffers( 1, &mask_vbo );
        glBindBuffer( GL_ARRAY_BUFFER, mask_vbo );
        glBufferData( GL_ARRAY_BUFFER, sizeof( DebugVertex ) * 6, mask_vertices, GL_STATIC_DRAW );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
     
        glGenVertexArrays( 1, &mask_vao );

        glBindVertexArray( mask_vao );

        glBindBuffer( GL_ARRAY_BUFFER, mask_vbo );

        if ( position_location >= 0 )
        {
            glEnableVertexAttribArray( position_location );
            glVertexAttribPointer( position_location, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (GLubyte*)0 );
        }

        if ( color_location >= 0 )
        {
            glEnableVertexAttribArray( color_location );
            glVertexAttribPointer( color_location, 4, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (GLubyte*) ( 3 * 4 ) );
        }

        glBindBuffer( GL_ARRAY_BUFFER, 0 );

        glBindVertexArray( 0 );

        glUseProgram( 0 );
    }

    initialized = true;

    check_opengl_error( "after cubes render init" );
}

void CubesRender::ResizeDisplay( int _displayWidth, int _displayHeight )
{
    displayWidth = _displayWidth;
    displayHeight = _displayHeight;
}

void CubesRender::SetLightPosition( const math::Vector & position )
{
    lightPosition = position;
}

void CubesRender::SetCamera( const math::Vector & position, const math::Vector & lookAt, const math::Vector & up )
{
    cameraPosition = position;
    cameraLookAt = lookAt;
    cameraUp = up;
}

void CubesRender::ClearScreen()
{
    glViewport( 0, 0, global.displayWidth, global.displayHeight );
    glClearStencil( 0 );
    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );     
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}

void CubesRender::BeginScene( float x1, float y1, float x2, float y2 )
{
    if ( !initialized )
        Initialize();

    if ( !initialized )
        return;

    glViewport( x1, y1, x2 - x1, y2 - y1 );

    glEnable( GL_SCISSOR_TEST );
    glScissor( x1, y1, x2 - x1, y2 - y1 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
}
        
void CubesRender::RenderCubes( const view::Cubes & cubes )
{
    if ( cubes.numCubes == 0 )
        return;

    uint32_t shader = global.shaderManager->GetShader( "Cubes" );
    if ( !shader )
        return;

    glUseProgram( shader );

    const int eye_location = glGetUniformLocation( shader, "EyePosition" );
    const int light_location = glGetUniformLocation( shader, "LightPosition" );

    if ( eye_location >= 0 )
        glUniform3fv( eye_location, 1, &cameraPosition[0] );

    if ( light_location >= 0 )
        glUniform3fv( light_location, 1, &lightPosition[0] );

    vectorial::mat4f viewMatrix = vectorial::mat4f::lookAt( vectorial::vec3f( cameraPosition.x, cameraPosition.y, cameraPosition.z ),
                                                            vectorial::vec3f( cameraLookAt.x, cameraLookAt.y, cameraLookAt.z ),
                                                            vectorial::vec3f( cameraUp.x, cameraUp.y, cameraUp.z ) );

    vectorial::mat4f projectionMatrix = vectorial::mat4f::perspective( 40.0f, displayWidth / displayHeight, 0.1f, 100.0f );

    for ( int i = 0; i < cubes.numCubes; ++i )
    {
        CubeInstance & instance = instance_data[i];
        instance.r = cubes.cube[i].r;
        instance.g = cubes.cube[i].g;
        instance.b = cubes.cube[i].b;
        instance.a = cubes.cube[i].a;
        instance.model = cubes.cube[i].transform;
        instance.modelView = viewMatrix * instance.model;
        instance.modelViewProjection = projectionMatrix * instance.modelView;
    }

    glBindBuffer( GL_ARRAY_BUFFER, cubes_instance_buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(CubeInstance) * cubes.numCubes, instance_data, GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    glBindVertexArray( cubes_vao );

    glDrawElementsInstanced( GL_TRIANGLES, sizeof( cube_indices ) / 2, GL_UNSIGNED_SHORT, nullptr, cubes.numCubes );

    glBindVertexArray( 0 );

    glUseProgram( 0 );

    check_opengl_error( "after draw cubes" );
}

inline void GenerateSilhoutteVerts( int & vertex_index,
                                    __restrict vectorial::vec3f * vertices,
                                    const vectorial::mat4f & transform,
                                    const vectorial::vec3f & local_light,
                                    const vectorial::vec3f & world_light,
                                    vectorial::vec3f a, 
                                    vectorial::vec3f b,
                                    const vectorial::vec3f & left_normal,
                                    const vectorial::vec3f & right_normal,
                                    bool left_dot,
                                    bool right_dot )
{
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
        
        // emit extruded quad as two triangles
        
        vertices[vertex_index]   = world_b;
        vertices[vertex_index+1] = world_a;
        vertices[vertex_index+2] = extruded_a;
        vertices[vertex_index+3] = world_b;
        vertices[vertex_index+4] = extruded_a;
        vertices[vertex_index+5] = extruded_b;
        
        vertex_index += 6;
    }
}

static vectorial::vec3f a(-1,+1,-1);
static vectorial::vec3f b(+1,+1,-1);
static vectorial::vec3f c(+1,+1,+1);
static vectorial::vec3f d(-1,+1,+1);
static vectorial::vec3f e(-1,-1,-1);
static vectorial::vec3f f(+1,-1,-1);
static vectorial::vec3f g(+1,-1,+1);
static vectorial::vec3f h(-1,-1,+1);

static vectorial::vec3f normals[6] =
{
    vectorial::vec3f(+1,0,0),
    vectorial::vec3f(-1,0,0),
    vectorial::vec3f(0,+1,0),
    vectorial::vec3f(0,-1,0),
    vectorial::vec3f(0,0,+1),
    vectorial::vec3f(0,0,-1)
};


void CubesRender::RenderCubeShadows( const view::Cubes & cubes )
{
    // make sure we have the shadow shader before going any further

    GLuint shader = global.shaderManager->GetShader( "Shadow" );
    if ( !shader )
        return;

    // generate shadow silhoutte vertices

    vectorial::vec3f world_light( lightPosition.x, lightPosition.y, lightPosition.z );
    
    int vertex_index = 0;
    
    for ( int i = 0; i < (int) cubes.numCubes; ++i )
    {
        const view::Cube & cube = cubes.cube[i];

        if ( cube.a < ShadowAlphaThreshold )
            continue;

        vectorial::vec3f local_light = transformPoint( cube.inverse_transform, world_light );
        
        bool dot[6];
        dot[0] = vectorial::dot( normals[0], local_light ) > 0;
        dot[1] = vectorial::dot( normals[1], local_light ) > 0;
        dot[2] = vectorial::dot( normals[2], local_light ) > 0;
        dot[3] = vectorial::dot( normals[3], local_light ) > 0;
        dot[4] = vectorial::dot( normals[4], local_light ) > 0;
        dot[5] = vectorial::dot( normals[5], local_light ) > 0;
        
        assert( vertex_index < MaxCubeShadowVertices );

        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, a, b, normals[5], normals[2], dot[5], dot[2] );
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, b, c, normals[0], normals[2], dot[0], dot[2] );
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, c, d, normals[4], normals[2], dot[4], dot[2] );
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, d, a, normals[1], normals[2], dot[1], dot[2] );
        
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, e, f, normals[3], normals[5], dot[3], dot[5] );
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, f, g, normals[3], normals[0], dot[3], dot[0] );
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, g, h, normals[3], normals[4], dot[3], dot[4] );
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, h, e, normals[3], normals[1], dot[3], dot[1] );

        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, a, e, normals[1], normals[5], dot[1], dot[5] );
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, b, f, normals[5], normals[0], dot[5], dot[0] );
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, c, g, normals[0], normals[4], dot[0], dot[4] );
        GenerateSilhoutteVerts( vertex_index, shadow_vertices, cube.transform, local_light, world_light, d, h, normals[4], normals[1], dot[4], dot[1] );

        assert( vertex_index < MaxCubeShadowVertices );
    }

    // upload vertices to shadow vbo

    glBindBuffer( GL_ARRAY_BUFFER, shadow_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vectorial::vec3f ) * vertex_index, shadow_vertices, GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // setup for zpass stencil shadow rendering in one pass

#if !DEBUG_CUBE_SHADOWS

    glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP_EXT );
    glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP_EXT );
    glStencilMask( ~0 );
    glStencilFunc( GL_ALWAYS, 0, ~0 );

    glDepthMask( GL_FALSE );
    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
    glEnable( GL_STENCIL_TEST );
    glDisable( GL_CULL_FACE ); 

#else

    glDisable( GL_CULL_FACE );

#endif

    // render shadow silhouette triangles

    glUseProgram( shader );

    glm::mat4 viewMatrix = glm::lookAt( glm::vec3( cameraPosition.x, cameraPosition.y, cameraPosition.z ), 
                                        glm::vec3( cameraLookAt.x, cameraLookAt.y, cameraLookAt.z ), 
                                        glm::vec3( cameraUp.x, cameraUp.y, cameraUp.z ) );

    glm::mat4 projectionMatrix = glm::perspective( 40.0f, displayWidth / (float)displayHeight, 0.1f, 100.0f );

    glm::mat4 modelViewProjection = projectionMatrix * viewMatrix;

    int location = glGetUniformLocation( shader, "ModelViewProjection" );
    if ( location >= 0 )
        glUniformMatrix4fv( location, 1, GL_FALSE, &modelViewProjection[0][0] );

    glBindVertexArray( shadow_vao );

    glDrawArrays( GL_TRIANGLES, 0, vertex_index );

    glBindVertexArray( 0 );

    glUseProgram( 0 );

    // clean up

    glCullFace( GL_BACK );
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glDepthMask( GL_TRUE );
    glDisable( GL_STENCIL_TEST );
   
    check_opengl_error( "after cube shadows" );
}

void CubesRender::RenderShadowQuad()
{
    GLuint shader = global.shaderManager->GetShader( "Debug" );
    if ( !shader )
        return;

    float x1 = 0;
    float y1 = 0;
    float x2 = global.displayWidth;
    float y2 = global.displayHeight;

    glViewport( x1, y1, x2 - x1, y2 - y1 );

    glEnable( GL_SCISSOR_TEST );
    glScissor( x1, y1, x2 - x1, y2 - y1 );

    glUseProgram( shader );

    glEnable( GL_DEPTH_TEST );

    glEnable( GL_STENCIL_TEST );

    glStencilFunc( GL_NOTEQUAL, 0x0, 0xff );
    glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glm::mat4 modelViewProjection = glm::ortho( 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f );

    int location = glGetUniformLocation( shader, "ModelViewProjection" );
    if ( location >= 0 )
        glUniformMatrix4fv( location, 1, GL_FALSE, &modelViewProjection[0][0] );

    glBindVertexArray( mask_vao );

    glDrawArrays( GL_TRIANGLES, 0, 6 );

    glBindVertexArray( 0 );

    glUseProgram( 0 );

    glDisable( GL_BLEND );

    glDisable( GL_STENCIL_TEST );

    check_opengl_error( "after shadow quad" );
}

void CubesRender::EndScene()
{
    glDisable( GL_SCISSOR_TEST );
    glDisable( GL_BLEND );
}

#endif // #ifdef CLIENT
