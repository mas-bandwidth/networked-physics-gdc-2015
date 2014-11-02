#ifdef CLIENT

#include "StoneManager.h"
#include "Global.h"
#include "Console.h"
#include "core/Core.h"
#include "core/File.h"
#include "core/Hash.h"
#include "core/Memory.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

static StoneData * LoadStoneData( core::Allocator & allocator, const char * filename )
{
    CORE_ASSERT( filename );

//    printf( "%.3f: Loading stone data: \"%s\"\n", global.timeBase.time, filename );

    FILE * file = fopen( filename, "rb" );
    if ( !file )
    {
        printf( "%.3f: error: failed to load stone file \"%s\"\n", global.timeBase.time, filename );
        return nullptr;
    }

    char header[5];
    if ( fread( header, 5, 1, file ) != 1 || header[0] != 'S' || header[1] != 'T' || header[2] != 'O' || header[3] != 'N' || header[4] != 'E' )
    {
        printf( "%.3f: error: not a valid stone file: \"%s\"\n", global.timeBase.time, filename );
        fclose( file );
        return nullptr;
    }

    StoneData * stoneData = CORE_NEW( allocator, StoneData );

    if ( fread( stoneData, sizeof( StoneData ), 1, file ) != 1 )
    {
        fclose( file );
        CORE_DELETE( allocator, StoneData, stoneData );
        return nullptr;
    }

    fclose( file );

    return stoneData;
}

StoneManager::StoneManager( core::Allocator & allocator )
    : m_stones( allocator )
{
    m_allocator = &allocator;
    core::hash::reserve( m_stones, 256 );
    Load();
}

StoneManager::~StoneManager()
{
    Unload();
}

const StoneData * StoneManager::GetStoneData( const char * name ) const
{
    const uint64_t key = core::hash_string( name );
    
    return core::hash::get( m_stones, key, (StoneData*)nullptr );
}

void StoneManager::Reload()
{
    printf( "%.3f: Reloading stones\n", global.timeBase.time );
    Unload();
    Load();
}

void StoneManager::Load()
{
    const char * stoneDirectory = "data/stones";
    
    DIR * dir = opendir( stoneDirectory );
    
    while ( dirent * entry = readdir( dir ) )
    {
        const char * filename = entry->d_name;

        const int len = strlen( filename );

        if ( len > 6 && 
             filename[len-6] == '.' &&
             filename[len-5] == 's' &&
             filename[len-4] == 't' &&
             filename[len-3] == 'o' &&
             filename[len-2] == 'n' &&
             filename[len-1] == 'e'
           )
        {
            const int MaxPath = 2048;

            char filename_without_extension[MaxPath];
            strcpy( filename_without_extension, entry->d_name );
            filename_without_extension[len-6] = '\0';

            char stone_path[MaxPath];
            sprintf( stone_path, "%s/%s.stone", stoneDirectory, filename_without_extension );

            StoneData * stoneData = LoadStoneData( *m_allocator, stone_path );
            if ( !stoneData )
                continue;

            uint32_t key = core::hash_string( filename_without_extension );

            core::hash::set( m_stones, key, stoneData );
        }
    }
    
    closedir( dir ); 
}

void StoneManager::Unload()
{
    for ( auto itor = core::hash::begin( m_stones ); itor != core::hash::end( m_stones ); ++itor )
    {
        StoneData * stoneData = itor->value;
//        printf( "%.3f: Delete stone %p\n", global.timeBase.time, stoneData );
        CORE_DELETE( *m_allocator, StoneData, stoneData );
    }
 
    core::hash::clear( m_stones );
}

CONSOLE_FUNCTION( reload_stones )
{
    CORE_ASSERT( global.stoneManager );

    global.stoneManager->Reload();
}

#endif // #ifdef CLIENT
