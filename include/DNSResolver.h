/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_DNS_RESOLVER_H
#define PROTOCOL_DNS_RESOLVER_H

#include "Resolver.h"

struct addrinfo;

namespace protocol
{
    ResolveResult DNSResolve_Blocking( std::string name, int family = AF_INET6, int socktype = SOCK_DGRAM );

    typedef std::map<std::string,ResolveEntry*> ResolveMap;

    class DNSResolver : public Resolver
    {
    public:

        DNSResolver( int family = AF_INET6, int socktype = SOCK_DGRAM );

        ~DNSResolver();

        void Resolve( const std::string & name );

        virtual void Update( const TimeBase & timeBase );

        virtual void Clear();

        virtual ResolveEntry * GetEntry( const std::string & name );

    private:

        int m_family;
        int m_socktype;
        ResolveMap map;
        ResolveMap in_progress;
    };
}

#endif
