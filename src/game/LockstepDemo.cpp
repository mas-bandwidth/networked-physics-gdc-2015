#include "LockstepDemo.h"
#include "Cubes.h"

#ifdef CLIENT

#include "Global.h"
#include "Render.h"
#include "Console.h"

LockstepDemo::LockstepDemo( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_internal = CORE_NEW( allocator, CubesInternal );
}

LockstepDemo::~LockstepDemo()
{
    CORE_ASSERT( m_internal );
    CORE_ASSERT( m_allocator );
    m_internal->Free( *m_allocator );
    CORE_DELETE( *m_allocator, CubesInternal, m_internal );
    m_internal = nullptr;
    m_allocator = nullptr;
}

bool LockstepDemo::Initialize()
{
    game::Config config;

    config.maxObjects = CubeSteps * CubeSteps + MaxPlayers + 1;     // note: why is the +1 required here?! seems wrong.
    config.deactivationTime = 0.5f;
    config.cellSize = 5.0f;
    config.cellWidth = CubeSteps / config.cellSize + 2 * 2;     // note: double so we have some extra space at the edge of the world
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

void LockstepDemo::Update()
{
    m_internal->Update();
}

bool LockstepDemo::Clear()
{
    return m_internal->Clear();
}

void LockstepDemo::Render()
{
    m_internal->Render();
}

bool LockstepDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool LockstepDemo::CharEvent( unsigned int code )
{
    return false;
}

#endif // #ifdef CLIENT
