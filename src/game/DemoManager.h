#ifndef DEMO_MANAGER_H
#define DEMO_MANAGER_H

#ifdef CLIENT

#include "Demo.h"

namespace core { class Allocator; }

class DemoManager
{
public:

    DemoManager( core::Allocator & allocator );
    
    ~DemoManager();

    bool LoadDemo( const char * name );

    void UnloadDemo();

    bool ReloadDemo();

    void ResetDemo();

    Demo * GetDemo();

    bool KeyEvent( int key, int scancode, int action, int mods );

    bool CharEvent( unsigned int code );

private:

    core::Allocator * m_allocator;

    Demo * m_demo;
};

#endif // #ifdef CLIENT

#endif // #ifndef DEMO_MANAGER
