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

const int Steps = 1024;
const int MaxVertices = 1024;

typedef game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> GameInstance;

struct LitVertex
{
    float x,y,z;
    float nx,ny,nz;
    float r,g,b,a;
};

struct UnlitVertex
{
    float x,y,z;
    float r,g,b,a;
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
 
    void RenderActivationArea( const view::ActivationArea & activationArea, float alpha );

    void RenderCubes( const view::Cubes & cubes, float alpha = 1.0f );
    
    void RenderCubeShadows( const view::Cubes & cubes );

    void RenderShadowQuad();

    void EndScene();
                
private:

    void Initialize();

    void BeginLit();
    void AddLitVertex( const LitVertex & vertex );
    void EndLit();

    void BeginUnlit();
    void AddUnlitVertex( const UnlitVertex & vertex );
    void EndUnlit();

    int displayWidth;
    int displayHeight;

    math::Vector cameraPosition;
    math::Vector cameraLookAt;
    math::Vector cameraUp;

    math::Vector lightPosition;

    uint32_t vbo_lit;
    uint32_t vbo_unlit;
    uint32_t vao_lit;
    uint32_t vao_unlit;

    LitVertex lit_vertices[MaxVertices];
    UnlitVertex unlit_vertices[MaxVertices];
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
        gameInstance->SetPlayerInput( 0, gameInput );

        const float deltaTime = global.timeBase.deltaTime;

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
        #ifdef WIDESCREEN
        math::Vector position = lookat + math::Vector(0,-11,5);
        #else
        math::Vector position = lookat + math::Vector(0,-12,6);
        #endif

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
        
        math::Vector lightPosition = math::Vector( 25.0f, -25.0f, 50.0f );
        lightPosition += camera.lookat;
        renderInterface.SetLightPosition( lightPosition );

        view::ActivationArea activationArea;
        view::setupActivationArea( activationArea, origin, 5.0f, global.timeBase.time );
        
        renderInterface.RenderActivationArea( activationArea, 1.0f );

        renderInterface.RenderCubes( cubes );
        
        #ifdef SHADOWS
        renderInterface.RenderCubeShadows( cubes );
        renderInterface.RenderShadowQuad();
        #endif
        
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
    config.cellSize = 4.0f;
    config.cellWidth = Steps / config.cellSize + 2;
    config.cellHeight = config.cellWidth;
    config.activationDistance = 5.0f;
    config.simConfig.ERP = 0.1f;
    config.simConfig.CFM = 0.001f;
    config.simConfig.MaxIterations = 12;
    config.simConfig.MaximumCorrectingVelocity = 100.0f;
    config.simConfig.ContactSurfaceLayer = 0.05f;
    config.simConfig.Elasticity = 0.3f;
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

enum VertexType
{
    VERTEX_LIT,
    VERTEX_UNLIT   
};

/*
static void RenderBegin( VertexType type, const math::Matrix & model, const math::Matrix & view, const math::Matrix & projection );
static void RenderVertexUnlit( const math::Vector & position, float r, float g, float b, float a );
static void RenderVertexLit( const math::Vector & position, const math::Vector & normal, float r, float g, float b, float a );
static void RenderEnd();
*/


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

struct ShadowVBO
{
    uint32_t id;
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

    uint32_t cube_vbo;
    ShadowVBO shadow_vbo;
};

// -----------------------------------------

RenderInterface::RenderInterface()
{
    glGenBuffers( 1, &vbo_lit );
    glGenBuffers( 1, &vbo_unlit );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    vao_lit = 0;
    vao_unlit = 0;
}

RenderInterface::~RenderInterface()
{
    glDeleteVertexArrays( 1, &vao_lit );
    glDeleteVertexArrays( 1, &vao_unlit );
    glDeleteBuffers( 1, &vbo_lit );
    glDeleteBuffers( 1, &vbo_unlit );
    vbo_lit = 0;
    vao_lit = 0;
    vbo_unlit = 0;
    vao_unlit = 0;
}

void RenderInterface::Initialize()
{
    uint32_t lit_shader = global.shaderManager->GetShader( "CubesLit" );
    uint32_t unlit_shader = global.shaderManager->GetShader( "CubesUnlit" );
    if ( !lit_shader || !unlit_shader )
        return;

    // setup lit vao
    {
        glUseProgram( lit_shader );

        glGenVertexArrays( 1, &vao_lit );

        glBindVertexArray( vao_lit );

        glBindBuffer( GL_ARRAY_BUFFER, vbo_lit );

        const int vertex_position_location = glGetAttribLocation( lit_shader, "VertexPosition" );
        const int vertex_normal_location = glGetAttribLocation( lit_shader, "VertexNormal" );
        const int vertex_color_location = glGetAttribLocation( lit_shader, "VertexColor" );

        if ( vertex_position_location >= 0 )
        {
            glEnableVertexAttribArray( vertex_position_location );
            glVertexAttribPointer( vertex_position_location, 3, GL_FLOAT, GL_FALSE, sizeof(LitVertex), (GLubyte*)0 );
        }

        if ( vertex_normal_location >= 0 )
        {
            glEnableVertexAttribArray( vertex_normal_location );
            glVertexAttribPointer( vertex_normal_location, 3, GL_FLOAT, GL_FALSE, sizeof(LitVertex), (GLubyte*)(3*4) );
        }

        if ( vertex_color_location >= 0 )
        {
            glEnableVertexAttribArray( vertex_color_location );
            glVertexAttribPointer( vertex_color_location, 4, GL_FLOAT, GL_FALSE, sizeof(LitVertex), (GLubyte*)(6*4) );
        }
    }

    // setup unlit vao
    {
        glUseProgram( unlit_shader );

        glGenVertexArrays( 1, &vao_unlit );

        glBindVertexArray( vao_unlit );

        glBindBuffer( GL_ARRAY_BUFFER, vbo_unlit );

        const int vertex_position_location = glGetAttribLocation( unlit_shader, "VertexPosition" );
        const int vertex_color_location = glGetAttribLocation( unlit_shader, "VertexColor" );

        if ( vertex_position_location >= 0 )
        {
            glEnableVertexAttribArray( vertex_position_location );
            glVertexAttribPointer( vertex_position_location, 3, GL_FLOAT, GL_FALSE, sizeof(UnlitVertex), (GLubyte*)0 );
        }

        if ( vertex_color_location >= 0 )
        {
            glEnableVertexAttribArray( vertex_color_location );
            glVertexAttribPointer( vertex_color_location, 4, GL_FLOAT, GL_FALSE, sizeof(UnlitVertex), (GLubyte*)(6*4) );
        }
    }

    check_opengl_error( "after cubes render init" );

    glUseProgram( 0 );
    glBindVertexArray( 0 );
}

void RenderInterface::BeginLit()
{
    CORE_ASSERT( )
    // ...
}

void RenderInterface::AddLitVertex( const LitVertex & vertex )
{
    // ...
}

void RenderInterface::EndLit()
{
    // ...
}

void RenderInterface::BeginUnlit()
{
    // ...
}

void RenderInterface::AddUnlitVertex( const UnlitVertex & vertex )
{
    // ...
}

void RenderInterface::EndUnlit()
{
    // ...
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
    if ( vao_lit == 0 )
        Initialize();

    glViewport( x1, y1, x2 - x1, y2 - y1 );

    glEnable( GL_SCISSOR_TEST );
    glScissor( x1, y1, x2 - x1, y2 - y1 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    /*
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
    */
} 

void RenderInterface::RenderActivationArea( const view::ActivationArea & activationArea, float alpha )
{
    /*
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
    */
}
        
void RenderInterface::RenderCubes( const view::Cubes & cubes, float alpha )
{
#ifndef DEBUG_SHADOW_VOLUMES

    /*    
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
    */
    
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
    /*
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
    */
}

void RenderInterface::RenderCubeShadows( const view::Cubes & cubes )
{
    /*
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
    */
}

void RenderInterface::RenderShadowQuad()
{
    #if !defined DEBUG_SHADOW_VOLUMES

    /*
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
    */

    #endif
}

void RenderInterface::EndScene()
{
    glDisable( GL_SCISSOR_TEST );
    glDisable( GL_BLEND );
}

#endif // #ifdef CLIENT
