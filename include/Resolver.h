/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_RESOLVER_H
#define PROTOCOL_RESOLVER_H

#include "Common.h"
#include "Address.h"

namespace protocol
{
    struct ResolveResult
    {
        std::vector<Address> addresses;
    };

    enum ResolveStatus
    {
        RESOLVE_IN_PROGRESS,
        RESOLVE_SUCCEEDED,
        RESOLVE_FAILED
    };

    typedef std::function< void( const std::string & name, ResolveResult * result ) > ResolveCallback;

    struct ResolveEntry
    {
        ResolveStatus status;
        ResolveResult * result;
        std::future<ResolveResult*> future;
        std::vector<ResolveCallback> callbacks;

        // todo: still need to determine ownership here. this class should become 
        // non-POD and will need to be stored only via pointer.
    };

    class Resolver
    {
    public:

        virtual void Resolve( const std::string & name, ResolveCallback cb = nullptr ) = 0;

        virtual void Update( const TimeBase & timeBase ) = 0;

        virtual void Clear() = 0;

        virtual ResolveEntry * GetEntry( const std::string & name ) = 0;
    };
}

#endif
