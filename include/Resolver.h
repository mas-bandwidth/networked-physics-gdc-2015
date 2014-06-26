/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_RESOLVER_H
#define PROTOCOL_RESOLVER_H

#include "Common.h"
#include "Address.h"

// todo: find a way to move this into the implementation cpp file
#include <future>

// todo: kill it with fire!
#include <string>

namespace protocol
{
    struct ResolveResult
    {
        int numAddresses = 0;
        Address address[MaxResolveAddresses];
    };

    enum ResolveStatus
    {
        RESOLVE_IN_PROGRESS,
        RESOLVE_SUCCEEDED,
        RESOLVE_FAILED
    };

    struct ResolveEntry
    {
        ResolveStatus status;
        ResolveResult result;
        std::future<ResolveResult> future;          // todo: move the future into the C++ so it does not pollute headers
    };

    class Resolver
    {
    public:

        virtual void Resolve( const std::string & name ) = 0;

        virtual void Update( const TimeBase & timeBase ) = 0;

        virtual void Clear() = 0;

        virtual ResolveEntry * GetEntry( const std::string & name ) = 0;
    };
}

#endif
