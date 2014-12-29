#include "SnapshotDemo.h"

#ifdef CLIENT

SnapshotDemo::SnapshotDemo( core::Allocator & allocator )
{
    m_allocator = &allocator;

    // ...
}

SnapshotDemo::~SnapshotDemo()
{
    // ...
}

bool SnapshotDemo::Initialize()
{
    // ...

    return true;
}

void SnapshotDemo::Update()
{
    // ...
}

bool SnapshotDemo::Clear()
{
    return false;
}

void SnapshotDemo::Render()
{
    // ...
}

bool SnapshotDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    // ...

    return false;
}

bool SnapshotDemo::CharEvent( unsigned int code )
{
    // ...

    return false;
}

#endif // #ifdef CLIENT
