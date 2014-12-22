#ifndef CUBES_DEMO_H
#define CUBES_DEMO_H

#ifdef CLIENT

#include "Demo.h"

struct CubesInternal;

class CubesDemo : public Demo
{
public:

    CubesDemo( core::Allocator & allocator );

    ~CubesDemo();

    virtual bool Initialize() override;

    virtual void Update() override;

    virtual bool Clear() override;

    virtual void Render() override;

    virtual bool KeyEvent( int key, int scancode, int action, int mods ) override;

    virtual bool CharEvent( unsigned int code ) override;

    virtual const char * GetName() const override { return "cubes"; }

private:

    core::Allocator * m_allocator;

    CubesInternal * m_internal;
};

#endif // #ifdef CLIENT

#endif // #ifndef CUBES_DEMO_H
