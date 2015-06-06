// Core Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

//#define CORE_DEBUG_MEMORY_LEAKS 1

#if CORE_DEBUG_MEMORY_LEAKS
#include <map>
#include <algorithm>
#endif

#define CORE_USE_SCRATCH_ALLOCATOR 1

#define CORE_PLATFORM_WINDOWS  1
#define CORE_PLATFORM_MAC      2
#define CORE_PLATFORM_UNIX     3

#if defined(_WIN32)
#define CORE_PLATFORM CORE_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define CORE_PLATFORM CORE_PLATFORM_MAC
#else
#define CORE_PLATFORM CORE_PLATFORM_UNIX
#endif

#define CORE_LITTLE_ENDIAN 1
#define CORE_BIG_ENDIAN 2

#if    defined(__386__) || defined(i386)    || defined(__i386__)  \
    || defined(__X86)   || defined(_M_IX86)                       \
    || defined(_M_X64)  || defined(__x86_64__)                    \
    || defined(alpha)   || defined(__alpha) || defined(__alpha__) \
    || defined(_M_ALPHA)                                          \
    || defined(ARM)     || defined(_ARM)    || defined(__arm__)   \
    || defined(WIN32)   || defined(_WIN32)  || defined(__WIN32__) \
    || defined(_WIN32_WCE) || defined(__NT__)                     \
    || defined(__MIPSEL__)
  #define CORE_ENDIAN CORE_LITTLE_ENDIAN
#else
  #define CORE_ENDIAN CORE_BIG_ENDIAN
#endif

#endif
