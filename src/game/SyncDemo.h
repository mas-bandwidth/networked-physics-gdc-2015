#ifndef SYNC_DEMO_H
#define SYNC_DEMO_H

#ifdef CLIENT

#include "Demo.h"

struct CubesInternal;
struct CubesSettings;
struct SyncInternal;

class SyncDemo : public Demo
{
public:

    SyncDemo( core::Allocator & allocator );

    ~SyncDemo();

    virtual bool Initialize() override;

    virtual void Shutdown() override;

    virtual void Update() override;

    virtual bool Clear() override;

    virtual void Render() override;

    virtual bool KeyEvent( int key, int scancode, int action, int mods ) override;

    virtual bool CharEvent( unsigned int code ) override;

    virtual const char * GetName() const override { return "sync"; }

    virtual int GetNumModes() const override;

    virtual const char * GetModeDescription( int mode ) const override;

private:

    core::Allocator * m_allocator;

    CubesInternal * m_internal;

    CubesSettings * m_settings;

    SyncInternal * m_sync;
};

#endif // #ifdef CLIENT

#endif // #ifndef DELTA_DEMO_H
