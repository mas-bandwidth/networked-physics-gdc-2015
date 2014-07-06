/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_CONFIG_H
#define PROTOCOL_CONFIG_H

//#define PROTOCOL_DEBUG_MEMORY_LEAKS 1

#if PROTOCOL_DEBUG_MEMORY_LEAKS
#include <map>
#include <algorithm>
#endif

#define PROTOCOL_USE_SCRATCH_ALLOCATOR 1

#define PROTOCOL_PLATFORM_WINDOWS  1
#define PROTOCOL_PLATFORM_MAC      2
#define PROTOCOL_PLATFORM_UNIX     3

#if defined(_WIN32)
#define PROTOCOL_PLATFORM PROTOCOL_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PROTOCOL_PLATFORM PROTOCOL_PLATFORM_MAC
#else
#define PROTOCOL_PLATFORM PROTOCOL_PLATFORM_UNIX
#endif

#endif
