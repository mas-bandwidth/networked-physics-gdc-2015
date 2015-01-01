#ifndef SNAPSHOT_DEMO_H
#define SNAPSHOT_DEMO_H

#ifdef CLIENT

#include "Demo.h"

struct CubesInternal;
struct CubesSettings;
struct SnapshotInternal;

class SnapshotDemo : public Demo
{
public:

    SnapshotDemo( core::Allocator & allocator );

    ~SnapshotDemo();

    virtual bool Initialize() override;

    virtual void Shutdown() override;

    virtual void Update() override;

    virtual bool Clear() override;

    virtual void Render() override;

    virtual bool KeyEvent( int key, int scancode, int action, int mods ) override;

    virtual bool CharEvent( unsigned int code ) override;

    virtual const char * GetName() const override { return "snapshot"; }

    virtual int GetNumModes() const override;

    virtual const char * GetModeDescription( int mode ) const override;

private:

    core::Allocator * m_allocator;

    CubesInternal * m_internal;

    CubesSettings * m_settings;

    SnapshotInternal * m_snapshot;
};

#endif // #ifdef CLIENT

#endif // #ifndef SNAPSHOT_DEMO_H
