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

    uint32_t GetShader( const char * name );

private:

    void Load();
    void Unload();

    core::Hash<uint32_t> m_shaders;
};

#endif // #ifdef CLIENT

#endif // #ifndef SHADER_MANAGER_H
