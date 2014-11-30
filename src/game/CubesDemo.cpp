#include "CubesDemo.h"

#ifdef CLIENT

#include "Global.h"
#include "Render.h"
#include "ShaderManager.h"
#include "core/Memory.h"
#include "cubes/Game.h"
#include "cubes/View.h"
#include "cubes/Hypercube.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;

const int Steps = 50;
const int MaxCubes = 1024 * 4;
const int MaxShadowVertices = 1024 * 32;

#define DEBUG_SHADOWS 0

typedef game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> GameInstance;

struct CubeInstance
{
    glm::vec4 color;
    glm::mat4 model;
    glm::mat4 modelView;
    glm::mat4 modelViewProjection;
};

class RenderInterface
{
public:

    RenderInterface();
    ~RenderInterface();

    void ResizeDisplay( int displayWidth, int displayHeight );
    
    int GetDisplayWidth() const;

    int GetDisplayHeight() const;

    void SetLightPosition( const math::Vector & position );

    void SetCamera( const math::Vector & position, const math::Vector & lookAt, const math::Vector & up );

    void ClearScreen();

    void BeginScene( float x1, float y1, float x2, float y2 );
 
    void RenderCubes( const view::Cubes & cubes, float alpha = 1.0f );
    
    void RenderCubeShadows( const view::Cubes & cubes );

    void RenderShadowQuad();

    void EndScene();
                
private:

    void Initialize();

    bool initialized;

    int displayWidth;
    int displayHeight;

    math::Vector cameraPosition;
    math::Vector cameraLookAt;
    math::Vector cameraUp;

    math::Vector lightPosition;

    uint32_t cubes_vao;
    uint32_t cubes_vbo;
    uint32_t cubes_ibo;
    uint32_t cubes_instance_buffer;

    uint32_t shadow_vao;
    uint32_t shadow_vbo;

    uint32_t mask_vao;
    uint32_t mask_vbo;

    vectorial::vec3f shadow_vertices[MaxShadowVertices];

    CubeInstance instance_data[MaxCubes];
};

struct CubesInternal
{
    GameInstance * gameInstance = nullptr;
    RenderInterface renderInterface;
    math::Vector origin;
    view::Camera camera;
    view::Packet viewPacket;
    view::ObjectManager viewObjectManager;
    game::Input gameInput;

    void Initialize( core::Allocator & allocator, game::Config & config )
    {
        gameInstance = CORE_NEW( allocator, GameInstance, config );

        origin = math::Vector(0,0,0);

        gameInstance->InitializeBegin();

        gameInstance->AddPlane( math::Vector(0,0,1), 0 );

        AddCube( gameInstance, 1, math::Vector(0,0,10) );

        const int border = 10.0f;
        const float origin = -Steps / 2 + border;
        const float z = hypercube::NonPlayerCubeSize / 2;
        const int count = Steps - border * 2;
        for ( int y = 0; y < count; ++y )
            for ( int x = 0; x < count; ++x )
                AddCube( gameInstance, 0, math::Vector(x+origin,y+origin,z) );

        gameInstance->InitializeEnd();

        gameInstance->OnPlayerJoined( 0 );
        gameInstance->SetLocalPlayer( 0 );
        gameInstance->SetPlayerFocus( 0, 1 );

        gameInstance->SetFlag( game::FLAG_Push );
        gameInstance->SetFlag( game::FLAG_Pull );
    }

    void Free( core::Allocator & allocator )
    {
        CORE_DELETE( allocator, GameInstance, gameInstance );
        gameInstance = nullptr;
    }

    void AddCube( GameInstance * gameInstance, int player, const math::Vector & position )
    {
        hypercube::DatabaseObject object;
        cubes::CompressPosition( position, object.position );
        cubes::CompressOrientation( math::Quaternion(1,0,0,0), object.orientation );
        object.dirty = player;
        object.enabled = player;
        object.session = 0;
        object.player = player;
        activation::ObjectId id = gameInstance->AddObject( object, position.x, position.y );
        if ( player )
            gameInstance->DisableObject( id );
    }

    void Update()
    {
        const float deltaTime = global.timeBase.deltaTime;

        gameInstance->SetPlayerInput( 0, gameInput );

        gameInstance->Update( deltaTime );

        gameInstance->GetViewPacket( viewPacket );

        if ( viewPacket.objectCount >= 1 )
        {
            view::ObjectUpdate updates[MaxViewObjects];
            getViewObjectUpdates( updates, viewPacket );
            viewObjectManager.UpdateObjects( updates, viewPacket.objectCount );
        }

        viewObjectManager.ExtrapolateObjects( deltaTime );

        viewObjectManager.Update( deltaTime );

        view::Object * playerCube = viewObjectManager.GetObject( 1 );
        if ( playerCube )
            origin = playerCube->position + playerCube->positionError;

        math::Vector lookat = origin - math::Vector(0,0,1);

        math::Vector position = lookat + math::Vector(0,-11,5);

        camera.EaseIn( lookat, position ); 
    }

    void Render()
    {
        renderInterface.ResizeDisplay( global.displayWidth, global.displayHeight );

        renderInterface.ClearScreen();

        view::Cubes cubes;
        viewObjectManager.GetRenderState( cubes );

        const int width = renderInterface.GetDisplayWidth();
        const int height = renderInterface.GetDisplayHeight();

        renderInterface.BeginScene( 0, 0, width, height );

        renderInterface.SetCamera( camera.position, camera.lookat, camera.up );
        
        renderInterface.SetLightPosition( camera.lookat + math::Vector( 25.0f, -50.0f, 100.0f ) );

        view::ActivationArea activationArea;
        view::setupActivationArea( activationArea, origin, 5.0f, global.timeBase.time );
        
        renderInterface.RenderCubes( cubes );
        
        renderInterface.RenderCubeShadows( cubes );

        renderInterface.RenderShadowQuad();
        
        renderInterface.EndScene();
    }

    bool KeyEvent( int key, int scancode, int action, int mods )
    {
        if ( mods != 0 )
            return false;

        if ( action == GLFW_PRESS || action == GLFW_REPEAT )
        {
            switch ( key )
            {
                case GLFW_KEY_LEFT:     gameInput.left = true;      return true;
                case GLFW_KEY_RIGHT:    gameInput.right = true;     return true;
                case GLFW_KEY_UP:       gameInput.up = true;        return true;
                case GLFW_KEY_DOWN:     gameInput.down = true;      return true;
                case GLFW_KEY_SPACE:    gameInput.push = true;      return true;
                case GLFW_KEY_Z:        gameInput.pull = true;      return true;
            }
        }
        else if ( action == GLFW_RELEASE )
        {
            switch ( key )
            {
                case GLFW_KEY_LEFT:     gameInput.left = false;      return true;
                case GLFW_KEY_RIGHT:    gameInput.right = false;     return true;
                case GLFW_KEY_UP:       gameInput.up = false;        return true;
                case GLFW_KEY_DOWN:     gameInput.down = false;      return true;
                case GLFW_KEY_SPACE:    gameInput.push = false;      return true;
                case GLFW_KEY_Z:        gameInput.pull = false;      return true;
            }
        }

        return false;
    }
};

CubesDemo::CubesDemo( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_internal = CORE_NEW( allocator, CubesInternal );
}

CubesDemo::~CubesDemo()
{
    CORE_ASSERT( m_internal );
    CORE_ASSERT( m_allocator );
    m_internal->Free( *m_allocator );
    CORE_DELETE( *m_allocator, CubesInternal, m_internal );
    m_internal = nullptr;
    m_allocator = nullptr;
}

bool CubesDemo::Initialize()
{
    game::Config config;

    config.maxObjects = Steps * Steps + MaxPlayers + 1;
    config.deactivationTime = 0.5f;
    config.cellSize = 5.0f;
    config.cellWidth = Steps / config.cellSize + 2;
    config.cellHeight = config.cellWidth;
    config.activationDistance = 100.0f;
    config.simConfig.ERP = 0.25f;
    config.simConfig.CFM = 0.001f;
    config.simConfig.MaxIterations = 64;
    config.simConfig.MaximumCorrectingVelocity = 250.0f;
    config.simConfig.ContactSurfaceLayer = 0.01f;
    config.simConfig.Elasticity = 0.0f;
    config.simConfig.LinearDrag = 0.01f;
    config.simConfig.AngularDrag = 0.01f;
    config.simConfig.Friction = 200.0f;

    m_internal->Initialize( *m_allocator, config );

    return true;
}

void CubesDemo::Update()
{
    m_internal->Update();
}

void CubesDemo::Render()
{
    m_internal->Render();
}

bool CubesDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool CubesDemo::CharEvent( unsigned int code )
{
    return false;
}

// ---------------------------------------------------------------------------------------------------------

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

RenderInterface::RenderInterface()
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

RenderInterface::~RenderInterface()
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

void RenderInterface::Initialize()
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
            glVertexAttribPointer( color_location, 4, GL_FLOAT, GL_FALSE, sizeof( CubeInstance ), (void*) ( offsetof( CubeInstance, color ) ) );
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

void RenderInterface::ResizeDisplay( int _displayWidth, int _displayHeight )
{
    displayWidth = _displayWidth;
    displayHeight = _displayHeight;
}

int RenderInterface::GetDisplayWidth() const
{
    return displayWidth;
}

int RenderInterface::GetDisplayHeight() const
{
    return displayHeight;
}

void RenderInterface::SetLightPosition( const math::Vector & position )
{
    lightPosition = position;
}

void RenderInterface::SetCamera( const math::Vector & position, const math::Vector & lookAt, const math::Vector & up )
{
    cameraPosition = position;
    cameraLookAt = lookAt;
    cameraUp = up;
}

void RenderInterface::ClearScreen()
{
    glViewport( 0, 0, displayWidth, displayHeight );
    glClearStencil( 0 );
    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );     
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}

void RenderInterface::BeginScene( float x1, float y1, float x2, float y2 )
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
        
void RenderInterface::RenderCubes( const view::Cubes & cubes, float alpha )
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

    mat4 viewMatrix = glm::lookAt( glm::vec3( cameraPosition.x, cameraPosition.y, cameraPosition.z ), 
                                   glm::vec3( cameraLookAt.x, cameraLookAt.y, cameraLookAt.z ), 
                                   glm::vec3( cameraUp.x, cameraUp.y, cameraUp.z ) );

    mat4 projectionMatrix = glm::perspective( 40.0f, (float) global.displayWidth / (float) global.displayHeight, 0.1f, 100.0f );

    for ( int i = 0; i < cubes.numCubes; ++i )
    {
        CubeInstance & instance = instance_data[i];

        cubes.cube[i].transform.store( (float*) &instance.model );

        instance.modelView = viewMatrix * instance.model;
        instance.modelViewProjection = projectionMatrix * instance.modelView;

        instance.color = vec4( cubes.cube[i].r, cubes.cube[i].g, cubes.cube[i].b, cubes.cube[i].a );
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
        vertices[vertex_index+3]   = world_b;
        vertices[vertex_index+4] = extruded_a;
        vertices[vertex_index+5] = extruded_b;
        
        vertex_index += 6;
    }
}

void RenderInterface::RenderCubeShadows( const view::Cubes & cubes )
{
    // make sure we have the shadow shader before going any further

    GLuint shader = global.shaderManager->GetShader( "Shadow" );
    if ( !shader )
        return;

    // generate shadow silhoutte vertices

    vectorial::vec3f world_light( lightPosition.x, lightPosition.y, lightPosition.z );
    
    int vertex_index = 0;
    
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
        
        assert( vertex_index < MaxShadowVertices );

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

        assert( vertex_index < MaxShadowVertices );
    }

    // upload vertices to shadow vbo

    glBindBuffer( GL_ARRAY_BUFFER, shadow_vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vectorial::vec3f ) * vertex_index, shadow_vertices, GL_STREAM_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );

    // setup for zpass stencil shadow rendering in one pass

#if !DEBUG_SHADOWS

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

    mat4 viewMatrix = glm::lookAt( glm::vec3( cameraPosition.x, cameraPosition.y, cameraPosition.z ), 
                                   glm::vec3( cameraLookAt.x, cameraLookAt.y, cameraLookAt.z ), 
                                   glm::vec3( cameraUp.x, cameraUp.y, cameraUp.z ) );

    mat4 projectionMatrix = glm::perspective( 40.0f, (float) global.displayWidth / (float) global.displayHeight, 0.1f, 100.0f );

    mat4 modelViewProjection = projectionMatrix * viewMatrix;

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

void RenderInterface::RenderShadowQuad()
{
    GLuint shader = global.shaderManager->GetShader( "Debug" );
    if ( !shader )
        return;

    glUseProgram( shader );

    glEnable( GL_DEPTH_TEST );

    glEnable( GL_STENCIL_TEST );

    glStencilFunc( GL_NOTEQUAL, 0x0, 0xff );
    glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    mat4 modelViewProjection = glm::ortho( 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f );

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

void RenderInterface::EndScene()
{
    glDisable( GL_SCISSOR_TEST );
    glDisable( GL_BLEND );
}

#endif // #ifdef CLIENT
