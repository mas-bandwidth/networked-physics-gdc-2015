#include "CubesDemo.h"

#ifdef CLIENT

#include "Global.h"
#include "core/Memory.h"
#include "cubes/Game.h"
#include "cubes/View.h"
#include "cubes/Hypercube.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

const int Steps = 1024;

class RenderInterface
{
public:

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

    int displayWidth;
    int displayHeight;

    math::Vector cameraPosition;
    math::Vector cameraLookAt;
    math::Vector cameraUp;

    math::Vector lightPosition;
};

typedef game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> GameInstance;

struct CubesInternal
{
    GameInstance * gameInstance = nullptr;
    RenderInterface renderInterface;
    math::Vector origin;
    view::Camera camera;
    view::Packet viewPacket;
    view::ObjectManager viewObjectManager;
};

CubesDemo::CubesDemo( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_internal = CORE_NEW( allocator, CubesInternal );
}

CubesDemo::~CubesDemo()
{
    CORE_DELETE( *m_allocator, GameInstance, m_internal->gameInstance );
    CORE_DELETE( *m_allocator, CubesInternal, m_internal );
    m_internal = nullptr;
    m_allocator = nullptr;
}

static void AddCube( game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> * gameInstance, int player, const math::Vector & position )
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

    m_internal->gameInstance = CORE_NEW( *m_allocator, GameInstance, config );

    m_internal->origin = math::Vector(0,0,0);

    m_internal->gameInstance->InitializeBegin();

    m_internal->gameInstance->AddPlane( math::Vector(0,0,1), 0 );

    AddCube( m_internal->gameInstance, 1, math::Vector(0,0,10) );

    const int border = 10.0f;
    const float origin = -Steps / 2 + border;
    const float z = hypercube::NonPlayerCubeSize / 2;
    const int count = Steps - border * 2;
    for ( int y = 0; y < count; ++y )
        for ( int x = 0; x < count; ++x )
            AddCube( m_internal->gameInstance, 0, math::Vector(x+origin,y+origin,z) );

    m_internal->gameInstance->InitializeEnd();

    m_internal->gameInstance->OnPlayerJoined( 0 );
    m_internal->gameInstance->SetLocalPlayer( 0 );
    m_internal->gameInstance->SetPlayerFocus( 0, 1 );

    m_internal->gameInstance->SetFlag( game::FLAG_Push );
    m_internal->gameInstance->SetFlag( game::FLAG_Pull );

    return true;
}

void CubesDemo::Update()
{
    // ...
}

void CubesDemo::Render()
{
    const float deltaTime = global.timeBase.deltaTime;

    // update the scene to be rendered
    
    if ( m_internal->viewPacket.objectCount >= 1 )
    {
        view::ObjectUpdate updates[MaxViewObjects];
        getViewObjectUpdates( updates, m_internal->viewPacket );
        m_internal->viewObjectManager.UpdateObjects( updates, m_internal->viewPacket.objectCount );
    }

    m_internal->viewObjectManager.ExtrapolateObjects( deltaTime );

    m_internal->viewObjectManager.Update( deltaTime );

    // update camera

    view::Object * playerCube = m_internal->viewObjectManager.GetObject( 1 );
    if ( playerCube )
        m_internal->origin = playerCube->position + playerCube->positionError;
    math::Vector lookat = m_internal->origin - math::Vector(0,0,1);
    #ifdef WIDESCREEN
    math::Vector position = lookat + math::Vector(0,-11,5);
    #else
    math::Vector position = lookat + math::Vector(0,-12,6);
    #endif
    m_internal->camera.EaseIn( lookat, position ); 

    // render the scene
    
    m_internal->renderInterface.ResizeDisplay( global.displayWidth, global.displayHeight );

    m_internal->renderInterface.ClearScreen();

    view::Cubes cubes;
    m_internal->viewObjectManager.GetRenderState( cubes );

    const int width = m_internal->renderInterface.GetDisplayWidth();
    const int height = m_internal->renderInterface.GetDisplayHeight();

    m_internal->renderInterface.BeginScene( 0, 0, width, height );

    // todo
//    view::setCameraAndLight( &m_internal->renderInterface, m_internal->camera );

    view::ActivationArea activationArea;
    view::setupActivationArea( activationArea, m_internal->origin, 5.0f, global.timeBase.time );
    
    m_internal->renderInterface.RenderActivationArea( activationArea, 1.0f );
    m_internal->renderInterface.RenderCubes( cubes );
    
    /*
    #ifdef SHADOWS
    if ( shadows )
    {
        renderInterface->RenderCubeShadows( cubes );
        renderInterface->RenderShadowQuad();
    }
    #endif
    */
    
    m_internal->renderInterface.EndScene();
}

bool CubesDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    // ...

    return false;
}

bool CubesDemo::CharEvent( unsigned int code )
{
    // ...

    return false;
}

#endif // #ifdef CLIENT









#if 0

    void ProcessInput( const platform::Input & input )
    {
        // pass input to game instance
        
        game::Input gameInput;
        gameInput.left = input.left ? 1.0f : 0.0f;
        gameInput.right = input.right ? 1.0f : 0.0f;
        gameInput.up = input.up ? 1.0f : 0.0f;
        gameInput.down = input.down ? 1.0f : 0.0f;
        gameInput.push = input.space ? 1.0f : 0.0f;
        gameInput.pull = input.z ? 1.0f : 0.0f;
        gameInstance->SetPlayerInput( 0, gameInput );
    }
    
    void Update( float deltaTime )
    {
        // start the worker thread
        workerThread.Start( gameInstance );
        t += deltaTime;
    }

    void WaitForSim()
    {
        workerThread.Join();
        gameInstance->GetViewPacket( viewPacket );
    }

#endif

/*
    void setCameraAndLight( render::Interface * renderInterface, const Camera & camera )
    {
        renderInterface->SetCamera( camera.position, camera.lookat, camera.up );
        math::Vector lightPosition = math::Vector( 25.0f, -25.0f, 50.0f );
        lightPosition += camera.lookat;
        renderInterface->SetLightPosition( lightPosition );
    }
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

void RenderInterface::ResizeDisplay( int displayWidth, int displayHeight )
{
    this->displayWidth = displayWidth;
    this->displayHeight = displayHeight;
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
    glDisable( GL_SCISSOR_TEST );
    glClearStencil( 0 );
    glClearColor( 1.0f, 1.0f, 1.0f, 1.0f );     
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
}

void RenderInterface::BeginScene( float x1, float y1, float x2, float y2 )
{
    /*
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
    // ...
}
