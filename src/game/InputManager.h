#ifdef CLIENT

#ifndef GAME_INPUT_MANAGER_H
#define GAME_INPUT_MANAGER_H

namespace core { class Allocator; }

class InputManager
{
public:

    InputManager( core::Allocator & allocator );

    ~InputManager();

    void KeyEvent( int key, int scancode, int action, int mods );

    void CharEvent( unsigned int code );

private:

    core::Allocator * m_allocator;
};

#endif // #ifndef GAME_INPUT_MANAGER_H

#endif // #ifdef CLIENT