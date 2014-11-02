#ifndef INTERPOLATION_DEMO_H
#define INTERPOLATION_DEMO_H

#ifdef CLIENT

#include "Demo.h"

class InterpolationDemo : public Demo
{
public:

    InterpolationDemo( core::Allocator & allocator );

    ~InterpolationDemo();

    virtual bool Initialize() override;

    virtual void Update() override;

    virtual void Render() override;

    virtual bool KeyEvent( int key, int scancode, int action, int mods ) override;

    virtual bool CharEvent( unsigned int code ) override;

private:

    core::Allocator * m_allocator;
};

#endif // #ifdef CLIENT

#endif // #ifndef INTERPOLATION_DEMO_H
