#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#ifdef CLIENT

#include "core/Types.h"

class ShaderManager
{
public:

    ShaderManager( core::Allocator & allocator );
    
    ~ShaderManager();

    void Reload();

    unsigned int GetShader( const char * name );

private:

    void Load();
    void Unload();

    core::Hash<uint32_t> m_shaders;
};

#endif

#endif
