/*
    Network Protocol Library.
    Copyright (c) 2014 The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_DNS_RESOLVER_H
#define PROTOCOL_DNS_RESOLVER_H

#include "Config.h"

#if PROTOCOL_USE_RESOLVER

#include "Resolver.h"

struct addrinfo;

namespace protocol
{
    ResolveResult DNSResolve_Blocking( std::string name, bool ipv6 );

    typedef std::map<std::string,ResolveEntry*> ResolveMap;

    class DNSResolver : public Resolver
    {
    public:

        DNSResolver( bool ipv6 = true );

        ~DNSResolver();

        void Resolve( const std::string & name );

        virtual void Update( const TimeBase & timeBase );

        virtual void Clear();

        virtual ResolveEntry * GetEntry( const std::string & name );

    private:

        bool m_ipv6;
        ResolveMap map;
        ResolveMap in_progress;
    };
}

#endif

#endif
