#ifndef CUBES_DEMO_H
#define CUBES_DEMO_H

#ifdef CLIENT

#include "Demo.h"

struct CubesInternal;
struct CubesSettings;

class CubesDemo : public Demo
{
public:

    CubesDemo( core::Allocator & allocator );

    ~CubesDemo();

    virtual bool Initialize() override;

    virtual void Shutdown() override;

    virtual void Update() override;

    virtual bool Clear() override;

    virtual void Render() override;

    virtual bool KeyEvent( int key, int scancode, int action, int mods ) override;

    virtual bool CharEvent( unsigned int code ) override;

    virtual const char * GetName() const override { return "cubes"; }

    virtual int GetNumModes() const override { return 0; }

    virtual const char * GetModeDescription( int mode ) const override { return ""; }

private:

    core::Allocator * m_allocator;

    CubesInternal * m_internal;

    CubesSettings * m_settings;
};

#endif // #ifdef CLIENT

#endif // #ifndef CUBES_DEMO_H
