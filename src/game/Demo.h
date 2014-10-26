#ifndef DEMO_H
#define DEMO_H

#ifdef CLIENT

namespace core { class Allocator; }

class Demo
{
public:

    virtual ~Demo() {}

    virtual bool Initialize() = 0;

    virtual void Update() = 0;

    virtual void Render() = 0;

    virtual bool KeyEvent( int key, int scancode, int action, int mods ) = 0;

    virtual bool CharEvent( unsigned int code ) = 0;

    // todo: reload notification
};

#endif // #ifdef CLIENT

#endif // #ifndef DEMO_H
