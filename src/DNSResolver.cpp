/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#include "DNSResolver.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

namespace protocol
{
    ResolveResult DNSResolve_Blocking( std::string name, int family, int socktype )
    {
        struct addrinfo hints, *res, *p;
        memset( &hints, 0, sizeof hints );
        hints.ai_family = family;
        hints.ai_socktype = socktype;

        uint16_t port = 0;
        char * hostname = const_cast<char*>( name.c_str() );
        for ( int i = 0; i < name.size(); ++i )
        {
            if ( hostname[i] == ':' )
            {
                hostname[i] = '\0';
                port = atoi( &hostname[i+1] );
                break;
            }
        }

        if ( getaddrinfo( hostname, nullptr, &hints, &res ) != 0 )
            return ResolveResult();

        ResolveResult result;
        for ( p = res; p != nullptr; p = p->ai_next )
        {
            if ( result.numAddresses >= MaxResolveAddresses - 1 )
                break;
            auto address = Address( p );
            if ( address.IsValid() )
            {
                if ( port != 0 )
                    address.SetPort( port );
                result.address[result.numAddresses++] = address;
            }
        }

        freeaddrinfo( res );

        return result;
    }

    DNSResolver::DNSResolver( int family, int socktype )
    {
        m_family = family;
        m_socktype = socktype;
    }

    DNSResolver::~DNSResolver()
    {
        // todo: this class owns the entry pointers so it is responsible for deleting them
    }

    void DNSResolver::Resolve( const std::string & name )
    {
        auto itor = map.find( name );
        if ( itor != map.end() )
            return;

        auto entry = new ResolveEntry();           // todo: instead of allocating here have some "max resolves" or something in a pool
        entry->status = RESOLVE_IN_PROGRESS;
        const int family = m_family;
        const int socktype = m_socktype;

        entry->future = async( std::launch::async, [name, family, socktype, entry] () -> ResolveResult
        { 
            return DNSResolve_Blocking( name, family, socktype );
        } );

        map[name] = entry;

        in_progress[name] = entry;
    }

    void DNSResolver::Update( const TimeBase & timeBase )
    {
        for ( auto itor = in_progress.begin(); itor != in_progress.end(); )
        {
            auto name = itor->first;
            auto entry = itor->second;

            if ( entry->future.wait_for( std::chrono::seconds(0) ) == std::future_status::ready )
            {
                entry->result = entry->future.get();
                entry->status = entry->result.numAddresses ? RESOLVE_SUCCEEDED : RESOLVE_FAILED;
                in_progress.erase( itor++ );
            }
            else
                ++itor;
        }
    }

    void DNSResolver::Clear()
    {
        map.clear();
    }

    ResolveEntry * DNSResolver::GetEntry( const std::string & name )
    {
        auto itor = map.find( name );
        if ( itor != map.end() )
            return itor->second;
        else
            return nullptr;
    }
}
