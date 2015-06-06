// Core Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef CORE_FILE_H
#define CORE_FILE_H

#include "core/Core.h"
#include <stdio.h>
#include <stdlib.h>

namespace core
{
    bool file_exists( const char * filename );

    uint64_t file_time( const char * filename );

    void make_path( const char * dir );

    void split_path_file( char ** p, char ** f, const char * pf );

    template <class T> void WriteObject( FILE * file, const T & object )
    {
        if ( fwrite( (const char*) &object, sizeof(object), 1, file ) != 1 )
        {
            printf( "error: failed to write data to file\n" );
            exit(1);
        }
    }
}

#endif
