#include "InterpolationDemo.h"

#ifdef CLIENT

InterpolationDemo::InterpolationDemo( core::Allocator & allocator )
{
    m_allocator = &allocator;

    // ...
}

InterpolationDemo::~InterpolationDemo()
{
    // ...
}

bool InterpolationDemo::Initialize()
{
    // ...

    return true;
}

void InterpolationDemo::Update()
{
    // ...
}

void InterpolationDemo::Render()
{
    // ...
}

bool InterpolationDemo::KeyEvent( int key, int scancode, int action, int mods )
{
    // ...

    return false;
}

bool InterpolationDemo::CharEvent( unsigned int code )
{
    // ...

    return false;
}

#endif // #ifdef CLIENT
