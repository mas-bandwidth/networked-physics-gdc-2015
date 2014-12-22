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
    m_internal->Initialize( *m_allocator );

    return true;
}

void CubesDemo::Update()
{
    m_internal->Update();
}

bool CubesDemo::Clear()
{
    return m_internal->Clear();
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

#endif // #ifdef CLIENT
