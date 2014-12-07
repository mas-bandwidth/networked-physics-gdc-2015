#ifndef GLOBAL_H
#define GLOBAL_H

#include "core/Core.h"

static const int MaxClients = 16;
static const float TickRate = 60;
static const int ServerPort = 10000;

#ifdef CLIENT
class GameClient;
class FontManager;
class ShaderManager;
class TextureManager;
class MeshManager;
class StoneManager;
class InputManager;
class DemoManager;
class ReplayManager;
#endif // #ifdef CLIENT

class Console;

struct Global
{
    core::TimeBase timeBase;
    
    bool quit = false;

    #ifdef CLIENT

    int displayWidth;
    int displayHeight;

    GameClient * client = nullptr;

    FontManager * fontManager = nullptr;
    ShaderManager * shaderManager = nullptr;
    TextureManager * textureManager = nullptr;
    MeshManager * meshManager = nullptr;
    InputManager * inputManager = nullptr;
    DemoManager * demoManager = nullptr;
    StoneManager * stoneManager = nullptr;
    ReplayManager * replayManager = nullptr;

    #endif // #ifdef CLIENT

    Console * console = nullptr;
};

extern Global global;

#endif // #ifndef GLOBAL_H
