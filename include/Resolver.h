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

    typedef function< void( const string & name, shared_ptr<ResolveResult> result ) > ResolveCallback;

    struct ResolveEntry
    {
        ResolveStatus status;
        shared_ptr<ResolveResult> result;
        future<shared_ptr<ResolveResult>> future;
        vector<ResolveCallback> callbacks;
    };

    class Resolver
    {
    public:

        virtual void Resolve( const string & name, ResolveCallback cb = nullptr ) = 0;

        virtual void Update( const TimeBase & timeBase ) = 0;

        virtual void Clear() = 0;

        virtual shared_ptr<ResolveEntry> GetEntry( const string & name ) = 0;
    };
}

#endif
