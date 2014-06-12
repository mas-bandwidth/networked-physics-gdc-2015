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
        vector<Address> addresses;
    };

    enum class ResolveStatus
    {
        InProgress,
        Succeeded,
        Failed
    };

    typedef function< void( const string & name, ResolveResult * result ) > ResolveCallback;

    struct ResolveEntry
    {
        ResolveStatus status;
        ResolveResult * result;
        future<ResolveResult*> future;
        vector<ResolveCallback> callbacks;

        // todo: still need to determine ownership here. this class should become 
        // non-POD and will need to be stored only via pointer.
    };

    class Resolver
    {
    public:

        virtual void Resolve( const string & name, ResolveCallback cb = nullptr ) = 0;

        virtual void Update( const TimeBase & timeBase ) = 0;

        virtual void Clear() = 0;

        virtual ResolveEntry * GetEntry( const string & name ) = 0;
    };
}

#endif
