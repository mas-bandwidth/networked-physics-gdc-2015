/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_DNS_RESOLVER_H
#define PROTOCOL_DNS_RESOLVER_H

#include "Resolver.h"

namespace protocol
{
    static ResolveResult * DNSResolve_Blocking( string name, int family = AF_UNSPEC, int socktype = SOCK_DGRAM )
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
            return nullptr;

        auto result = new ResolveResult();

        for ( p = res; p != nullptr; p = p->ai_next )
        {
            auto address = Address( p );
            if ( address.IsValid() )
            {
                if ( port != 0 )
                    address.SetPort( port );
                result->addresses.push_back( address );
            }
        }

        freeaddrinfo( res );

        if ( result->addresses.size() == 0 )
            return nullptr;

        return result;
    }

    typedef map<string,ResolveEntry*> ResolveMap;

    class DNSResolver : public Resolver
    {
    public:

        DNSResolver( int family = AF_INET6, int socktype = SOCK_DGRAM )
        {
            m_family = family;
            m_socktype = socktype;
        }

        ~DNSResolver()
        {
            // todo: this class owns the entry pointers so it is responsible for deleting them
        }

        virtual void Resolve( const string & name, ResolveCallback callback )
        {
            auto itor = map.find( name );
            if ( itor != map.end() )
            {
                auto name = itor->first;
                auto entry = itor->second;
                switch ( entry->status )
                {
                    case ResolveStatus::InProgress:
                        if ( callback )
                            entry->callbacks.push_back( callback );
                        break;

                    case ResolveStatus::Succeeded:
                    case ResolveStatus::Failed:             // note: result is nullptr if resolve failed
                        if ( callback )
                            callback( name, entry->result );      
                        break;
                }
                return;
            }

            auto entry = new ResolveEntry();
            entry->status = ResolveStatus::InProgress;
            if ( callback != nullptr )
                entry->callbacks.push_back( callback );
            const int family = m_family;
            const int socktype = m_socktype;

            // todo: probably want to do the alloc of the result *here*
            // while we are on main thread, before the future runs async
            // my allocators will *not* be threadsafe by design, eg. only
            // to be used on one thread.

            entry->future = async( launch::async, [name, family, socktype] () -> ResolveResult*
            { 
                return DNSResolve_Blocking( name, family, socktype );
            } );

            map[name] = entry;

            in_progress[name] = entry;
        }

        virtual void Update( const TimeBase & timeBase )
        {
            for ( auto itor = in_progress.begin(); itor != in_progress.end(); )
            {
                auto name = itor->first;
                auto entry = itor->second;

                if ( entry->future.wait_for( chrono::seconds(0) ) == future_status::ready )
                {
                    entry->result = entry->future.get();
                    entry->status = entry->result ? ResolveStatus::Succeeded : ResolveStatus::Failed;
                    for ( auto callback : entry->callbacks )
                        callback( name, entry->result );
                    in_progress.erase( itor++ );
                }
                else
                    ++itor;
            }
        }

        virtual void Clear()
        {
            map.clear();
        }

        virtual ResolveEntry * GetEntry( const string & name )
        {
            auto itor = map.find( name );
            if ( itor != map.end() )
                return itor->second;
            else
                return nullptr;
        }

    private:

        int m_family;
        int m_socktype;
        ResolveMap map;
        ResolveMap in_progress;
    };
}

#endif
