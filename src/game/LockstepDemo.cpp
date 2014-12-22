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
    CubesConfig config;
    
    config.num_simulations = 2;
    config.num_views = 2;

    m_internal->Initialize( *m_allocator );

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
