#ifndef GLOBAL_H
#define GLOBAL_H

#include "core/Core.h"

class FontManager;
class ShaderManager;
class TextureManager;
class MeshManager;

struct Global
{
    core::TimeBase timeBase;
    FontManager * fontManager = nullptr;
    ShaderManager * shaderManager = nullptr;
    TextureManager * textureManager = nullptr;
    MeshManager * meshManager = nullptr;
};

extern Global global;

#endif
