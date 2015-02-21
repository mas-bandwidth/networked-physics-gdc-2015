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
    Shutdown();
    m_allocator = nullptr;
}

bool CubesDemo::Initialize()
{
    CubesConfig config;

    config.num_simulations = 1;
    config.num_views = 1;
    config.soften_simulation = true;

    m_internal->Initialize( *m_allocator, config, m_settings );

    return true;
}

void CubesDemo::Shutdown()
{
    CORE_ASSERT( m_allocator );

    if ( m_settings )
    {
        CORE_DELETE( *m_allocator, CubesSettings, m_settings );
        m_settings = nullptr;
    }

    if ( m_internal )
    {
        m_internal->Free( *m_allocator );
        CORE_DELETE( *m_allocator, CubesInternal, m_internal );
        m_internal = nullptr;
    }
}

void CubesDemo::Update()
{
    CubesUpdateConfig update_config;

    update_config.sim[0].num_frames = 1;
    update_config.sim[0].frame_input[0] = m_internal->GetLocalInput();

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
