// Core Library - Copyright (c) 2014, The Network Protocol Company, Inc.

#ifndef CORE_FILE_H
#define CORE_FILE_H

#include "core/Core.h"

namespace core
{
    bool file_exists( const char * filename );

    uint64_t file_time( const char * filename );

    void make_path( const char * dir );

    void split_path_file( char ** p, char ** f, const char * pf );
}

#endif
