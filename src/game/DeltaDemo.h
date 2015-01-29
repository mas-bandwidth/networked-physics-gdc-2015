#ifndef DELTA_DEMO_H
#define DELTA_DEMO_H

#ifdef CLIENT

#include "Demo.h"

struct CubesInternal;
struct CubesSettings;
struct DeltaInternal;

class DeltaDemo : public Demo
{
public:

    DeltaDemo( core::Allocator & allocator );

    ~DeltaDemo();

    virtual bool Initialize() override;

    virtual void Shutdown() override;

    virtual void Update() override;

    virtual bool Clear() override;

    virtual void Render() override;

    virtual bool KeyEvent( int key, int scancode, int action, int mods ) override;

    virtual bool CharEvent( unsigned int code ) override;

    virtual const char * GetName() const override { return "delta"; }

    virtual int GetNumModes() const override;

    virtual const char * GetModeDescription( int mode ) const override;

private:

    core::Allocator * m_allocator;

    CubesInternal * m_internal;

    CubesSettings * m_settings;

    DeltaInternal * m_delta;
};

#endif // #ifdef CLIENT

#endif // #ifndef DELTA_DEMO_H
