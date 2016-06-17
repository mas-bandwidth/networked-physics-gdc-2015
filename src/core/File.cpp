/*
    Networked Physics Example

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "core/File.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TOOLS

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
        len = (int) strlen( tmp );
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
        *f = _strdup( slash );
    }
}

#endif // #if TOOLS
