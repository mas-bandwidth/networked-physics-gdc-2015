#include "CubesDemo.h"

#ifdef CLIENT

#include "Global.h"
#include "core/Memory.h"
#include "cubes/Game.h"
#include "cubes/View.h"
#include "cubes/Render.h"
#include "cubes/Hypercube.h"

const int Steps = 1024;

typedef game::Instance<hypercube::DatabaseObject, hypercube::ActiveObject> GameInstance;

struct CubesInternal
{
    GameInstance * gameInstance = nullptr;
    render::Interface * renderInterface = nullptr;
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
    
    //renderInterface->ClearScreen();

    render::Cubes cubes;
    m_internal->viewObjectManager.GetRenderState( cubes );

    /*
    int width = renderInterface->GetDisplayWidth();
    int height = renderInterface->GetDisplayHeight();

    renderInterface->BeginScene( 0, 0, width, height );
    setCameraAndLight( renderInterface, camera );
    ActivationArea activationArea;
    view::setupActivationArea( activationArea, origin, 5.0f, t );
    renderInterface->RenderActivationArea( activationArea, 1.0f );
    renderInterface->RenderCubes( cubes );
    #ifdef SHADOWS
    if ( shadows )
    {
        renderInterface->RenderCubeShadows( cubes );
        renderInterface->RenderShadowQuad();
    }
    #endif
    */
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
};

#endif
