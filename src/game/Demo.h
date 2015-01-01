#ifndef DEMO_H
#define DEMO_H

#ifdef CLIENT

namespace core { class Allocator; }

class Demo
{
public:

    virtual ~Demo() {}

    virtual bool Initialize() = 0;

    virtual void Shutdown() = 0;

    virtual bool Clear() = 0;

    virtual void Update() = 0;

    virtual void Render() = 0;

    virtual bool KeyEvent( int key, int scancode, int action, int mods ) = 0;

    virtual bool CharEvent( unsigned int code ) = 0;

    virtual const char * GetName() const = 0;

    virtual int GetNumModes() const = 0;

    virtual const char * GetModeDescription( int mode ) const = 0;

    void SetMode( int mode ) { m_mode = mode; }

    int GetMode() const { return m_mode; }

private:

    int m_mode = 0;
};

#endif // #ifdef CLIENT

#endif // #ifndef DEMO_H
