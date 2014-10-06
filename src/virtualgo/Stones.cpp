#include "virtualgo/Stones.h"
#include "core/File.h"
#include "core/Memory.h"

template <class T> bool ReadObject( FILE * file, const T & object )
{
    return fread( (char*) &object, sizeof(object), 1, file ) == 1;
}

namespace virtualgo
{
    StoneManager::StoneManager( core::Allocator & allocator )
    {
        m_numStones = 0;
        m_stoneData = nullptr;
        m_allocator = &allocator;
        Load();
    }

    StoneManager::~StoneManager()
    {
        if ( m_stoneData )
            CORE_DELETE_ARRAY( *m_allocator, m_stoneData, m_numStones );
        m_stoneData = nullptr;
        m_numStones = 0;        
    }

    const StoneData & StoneManager::GetStoneData( StoneSize size, StoneColor color ) const
    {
        CORE_ASSERT( m_stoneData );

        const int index = (color+1) * m_numStones + size;

        CORE_ASSERT( index >= 0 );
        CORE_ASSERT( index < m_numStones );

        return m_stoneData[index];
    }

    bool StoneManager::Load()
    {
        printf( "Loading stone data\n" );

        const char * filename = "data/stones/Stones.bin";

        FILE * file = fopen( filename, "rb" );
        if ( !file )
            return false;

        char header[6];
        memset( header, 0, sizeof( header ) );
        fread( header, 6, 1, file );
        if ( header[0] != 'S' ||
             header[1] != 'T' ||
             header[2] != 'O' ||
             header[3] != 'N' ||
             header[4] != 'E' ||
             header[5] != 'S' )
            return false;

        ReadObject( file, m_numStones );

        printf( "Loading %d stones\n" );

        m_stoneData = CORE_NEW_ARRAY( *m_allocator, StoneData, m_numStones );

        if ( fread( m_stoneData, sizeof( StoneData ) * m_numStones, 1, file ) != 1 )
        {
            fclose( file );
            return false;
        }

        fclose( file );

        return true;
    }
}
