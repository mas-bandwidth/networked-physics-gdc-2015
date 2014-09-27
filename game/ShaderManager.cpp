#include "ShaderManager.h"
#include "Render.h"

#ifdef CLIENT

#include "Common.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

ShaderManager::ShaderManager()
{
    Reload();
}

ShaderManager::~ShaderManager()
{
    // ...
}

void ShaderManager::Reload()
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
            printf( "%s/%s\n", shaderDirectory, entry->d_name );

            // todo: load the frag and vert files together
        }
    }
    
    closedir( dir );
}

unsigned int ShaderManager::GetShader( const char * name )
{
    return 0;
}

#endif
