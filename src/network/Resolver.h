// Network Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef NETWORK_RESOLVER_H
#define NETWORK_RESOLVER_H

#include "network/Config.h"

#if NETWORK_USE_RESOLVER

#include "core/Core.h"
#include "network/Address.h"
#include <future>
#include <string>

namespace network
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
        std::future<ResolveResult> future;
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

#endif
