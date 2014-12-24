// Network Library - Copyright (c) 2008-2015, The Network Protocol Company, Inc.

#ifndef NETWORK_DNS_RESOLVER_H
#define NETWORK_DNS_RESOLVER_H

#include "network/Config.h"

#if NETWORK_USE_RESOLVER

#include "network/Resolver.h"

struct addrinfo;

namespace network
{
    // todo: ewwwwww!
    ResolveResult DNSResolve_Blocking( std::string name, bool ipv6 );

    // todo: ewwwwww!
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
