#include "ShaderManager.h"

#ifdef CLIENT

#include "Render.h"
#include "Global.h"
#include "Console.h"
#include "core/Core.h"
#include "core/Hash.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

ShaderManager::ShaderManager( core::Allocator & allocator )
    : m_shaders( allocator )
{
    core::hash::reserve( m_shaders, 256 );
    Load();
}

ShaderManager::~ShaderManager()
{
    Unload();
}

void ShaderManager::Reload()
{
    printf( "%.3f: Reloading shaders\n", global.timeBase.time );
    Unload();
    Load();
}

unsigned int ShaderManager::GetShader( const char * name )
{
    const uint64_t key = core::hash_string( name );
    
    uint32_t shader = core::hash::get( m_shaders, key, uint32_t(0) );
    
    if ( shader )
        return shader;

    const uint64_t default_key = core::hash_string( "default" );
    
    return core::hash::get( m_shaders, default_key, uint32_t(0) );
}

void ShaderManager::Load()
{
    const char * shaderDirectory = "data/shaders";
    
    DIR * dir = opendir( shaderDirectory );
    
    while ( dirent * entry = readdir( dir ) )
    {
        const char * filename = entry->d_name;

        const int len = strlen( filename );

        if ( len > 5 && 
             filename[len-5] == '.' &&
             filename[len-4] == 'v' &&
             filename[len-3] == 'e' &&
             filename[len-2] == 'r' &&
             filename[len-1] == 't'
           )
        {
            const int MaxPath = 2048;

            char filename_without_extension[MaxPath];
            strcpy( filename_without_extension, entry->d_name );
            filename_without_extension[len-5] = '\0';

            char vertex_shader_path[MaxPath];
            char fragment_shader_path[MaxPath];

            sprintf( vertex_shader_path, "%s/%s.vert", shaderDirectory, filename_without_extension );
            sprintf( fragment_shader_path, "%s/%s.frag", shaderDirectory, filename_without_extension );

            unsigned int shader = load_shader( vertex_shader_path, fragment_shader_path );

            if ( shader == 0 )
                continue;

            uint32_t key = core::hash_string( filename_without_extension );

            core::hash::set( m_shaders, key, shader );
        }
    }
    
    closedir( dir );    
}

void ShaderManager::Unload()
{
    for ( auto itor = core::hash::begin( m_shaders ); itor != core::hash::end( m_shaders ); ++itor )
    {
        const uint32_t shader = itor->value;
        delete_shader( shader );
    }
 
    core::hash::clear( m_shaders );
}

CONSOLE_FUNCTION( reload_shaders )
{
    CORE_ASSERT( global.fontManager );

    global.shaderManager->Reload();
}

#endif // #ifdef CLIENT
