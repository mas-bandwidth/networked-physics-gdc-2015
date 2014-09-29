#ifndef GLOBAL_H
#define GLOBAL_H

#include "Common.h"

class FontManager;
class ShaderManager;
class TextureManager;
class MeshManager;

struct Global
{
    protocol::TimeBase timeBase;
    
    FontManager * fontManager = nullptr;
    ShaderManager * shaderManager = nullptr;
    TextureManager * textureManager = nullptr;
    MeshManager * meshManager = nullptr;
};

extern Global global;

#endif
