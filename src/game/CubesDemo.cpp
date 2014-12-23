#include "CubesDemo.h"
#include "Cubes.h"

#ifdef CLIENT

#include "Global.h"
#include "Render.h"
#include "Console.h"

CubesDemo::CubesDemo( core::Allocator & allocator )
{
    m_allocator = &allocator;
    m_internal = CORE_NEW( allocator, CubesInternal );
    m_settings = CORE_NEW( allocator, CubesSettings );
}

CubesDemo::~CubesDemo()
{
    CORE_ASSERT( m_internal );
    CORE_ASSERT( m_settings );
    CORE_ASSERT( m_allocator );
    m_internal->Free( *m_allocator );
    CORE_DELETE( *m_allocator, CubesInternal, m_internal );
    CORE_DELETE( *m_allocator, CubesSettings, m_settings );
    m_internal = nullptr;
    m_settings = nullptr;
    m_allocator = nullptr;
}

bool CubesDemo::Initialize()
{
    CubesConfig config;

    config.num_simulations = 1;
    config.num_views = 1;

    m_internal->Initialize( *m_allocator, config, m_settings );

    return true;
}

void CubesDemo::Update()
{
    CubesUpdateConfig update_config;

    update_config.run_update[0] = true;
    update_config.input[0] = m_internal->GetLocalInput();

    m_internal->Update( update_config );
}

bool CubesDemo::Clear()
{
    return m_internal->Clear();
}

void CubesDemo::Render()
{
    CubesRenderConfig render_config;

    render_config.render_mode = CUBES_RENDER_FULLSCREEN;

    m_internal->Render( render_config );
}

bool CubesDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    return m_internal->KeyEvent( key, scancode, action, mods );
}

bool CubesDemo::CharEvent( unsigned int code )
{
    return false;
}

#endif // #ifdef CLIENT
