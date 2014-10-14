#include "FontManager.h"
#include "Font.h"

#ifdef CLIENT

#include "Global.h"
#include "Console.h"
#include "core/Core.h"
#include "core/Hash.h"
#include "core/Memory.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

extern FontAtlas * LoadFontAtlas( core::Allocator & allocator, const char * filename );

FontManager::FontManager( core::Allocator & allocator )
    : m_fonts( allocator )
{
    m_allocator = &allocator;
    core::hash::reserve( m_fonts, 256 );
    Load();
}

FontManager::~FontManager()
{
    Unload();
}

void FontManager::Reload()
{
    printf( "%.3f: Reloading fonts\n", global.timeBase.time );
    Unload();
    Load();
}

Font * FontManager::GetFont( const char * name )
{
    const uint64_t key = core::hash_string( name );
    
    return core::hash::get( m_fonts, key, (Font*)nullptr );
}

void FontManager::Load()
{
    const char * fontDirectory = "data/fonts";
    
    DIR * dir = opendir( fontDirectory );
    
    while ( dirent * entry = readdir( dir ) )
    {
        const char * filename = entry->d_name;

        const int len = strlen( filename );

        if ( len > 5 && 
             filename[len-5] == '.' &&
             filename[len-4] == 'f' &&
             filename[len-3] == 'o' &&
             filename[len-2] == 'n' &&
             filename[len-1] == 't'
           )
        {
            const int MaxPath = 2048;

            char filename_without_extension[MaxPath];
            strcpy( filename_without_extension, entry->d_name );
            filename_without_extension[len-5] = '\0';

            char font_path[MaxPath];
            sprintf( font_path, "%s/%s.font", fontDirectory, filename_without_extension );

            FontAtlas * atlas = LoadFontAtlas( *m_allocator, font_path );
            if ( !atlas )
                continue;

            Font * font = CORE_NEW( *m_allocator, Font, *m_allocator, atlas );

            uint32_t key = core::hash_string( filename_without_extension );

            core::hash::set( m_fonts, key, font );
        }
    }
    
    closedir( dir );    
}

void FontManager::Unload()
{
    for ( auto itor = core::hash::begin( m_fonts ); itor != core::hash::end( m_fonts ); ++itor )
    {
        Font * font = itor->value;
//        printf( "%.3f: Delete font %p\n", global.timeBase.time, font );
        CORE_DELETE( *m_allocator, Font, font );
    }
 
    core::hash::clear( m_fonts );
}

CONSOLE_FUNCTION( reload_fonts )
{
    CORE_ASSERT( global.fontManager );

    global.fontManager->Reload();        
}

#endif // #ifdef CLIENT
