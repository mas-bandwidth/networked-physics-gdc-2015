#ifndef STONE_DEMO_H
#define STONE_DEMO_H

#ifdef CLIENT

#include "Demo.h"

struct StoneInternal;

class StoneDemo : public Demo
{
public:

    StoneDemo( core::Allocator & allocator );

    ~StoneDemo();

    virtual bool Initialize() override;

    virtual void Shutdown() override;

    virtual void Update() override;

    virtual bool Clear() override;

    virtual void Render() override;

    virtual bool KeyEvent( int key, int scancode, int action, int mods ) override;

    virtual bool CharEvent( unsigned int code ) override;

    virtual const char * GetName() const override { return "stone"; }

    virtual int GetNumModes() const override;

    virtual const char * GetModeDescription( int mode ) const override;

private:

    core::Allocator * m_allocator;

    StoneInternal * m_internal;
};

#endif // #ifdef CLIENT

#endif // #ifndef STONE_DEMO_H
