// Core Library - Copyright (c) 2008-2015, Glenn Fiedler

#include "core/File.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace core
{
    bool file_exists( const char * filename )
    {
        struct stat sb;
        return stat( filename, &sb ) == 0;
    }

    uint64_t file_time( const char * filename )
    {
        struct stat sb;
        if ( stat( filename, &sb ) != 0 )
            return (uint64_t) 0;   
        else
            return (uint64_t) sb.st_mtime;
    }

    void make_path( const char * dir )
    {
        char tmp[1024];
        char * p = NULL;
        int len;
        snprintf( tmp, sizeof(tmp), "%s", dir );
        len = strlen( tmp );
        if ( tmp[len-1] == '/' )
            tmp[len-1] = 0;
        for ( p = tmp + 1; *p; p++ )
        {
            if ( *p == '/' ) 
            {
                *p = 0;
                mkdir( tmp, S_IRWXU );
                *p = '/';
            }
        }
        mkdir( tmp, S_IRWXU );
    }

    void split_path_file( char ** p, char ** f, const char * pf ) 
    {
        const char * slash = pf, *next;
        while ( ( next = strpbrk( slash + 1, "\\/" ) ) ) slash = next;
        if ( pf != slash ) slash++;
        *p = strndup( pf, slash - pf );
        *f = strdup( slash );
    }
}
