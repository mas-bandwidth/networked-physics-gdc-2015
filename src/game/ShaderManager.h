#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#ifdef CLIENT

#include "Types.h"

class ShaderManager
{
public:

    ShaderManager( protocol::Allocator & allocator );
    
    ~ShaderManager();

    void Reload();

    unsigned int GetShader( const char * name );

private:

    void Load();
    void Unload();

    protocol::Hash<uint32_t> m_shaders;
};

#endif

#endif
