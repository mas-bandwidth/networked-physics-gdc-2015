/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef RESOLVE_WRAPPER_H
#define RESOLVE_WRAPPER_H

#include "NetworkInterface.h"
#include "Resolver.h"

namespace protocol
{
    struct ResolveWrapperConfig
    {
        Resolver * resolver = nullptr;                      // resolver eg: DNS
        NetworkInterface * networkInterface = nullptr;      // the network interface we are wrapping
    };

    class ResolveWrapper : public NetworkInterface
    {
        const ResolveWrapperConfig m_config;

        std::map<std::string,PacketQueue*> m_resolve_send_queues;

    public:

        ResolveWrapper( const ResolveWrapperConfig & config )
            : m_config( config )
        {
            assert( m_config.resolver );
            assert( m_config.networkInterface );
        }

        ~ResolveWrapper()
        {
            // todo: a *lot* of work needs to be done in here
            // also, we should try to cancel any async resolves
            // in progress right now.
        }

        void SendPacket( const Address & address, Packet * packet )
        {
            m_config.networkInterface->SendPacket( address, packet );
        }

        void SendPacket( const std::string & hostname, Packet * packet )
        {
            // Try to extract port from hostname string, eg: "localhost:10000"

            SendPacket( hostname, 0, packet );
        }

        void SendPacket( const std::string & hostname, uint16_t port, Packet * packet )
        {
            assert( packet );

            // Will use the port specified in port argument

            Address address;
            address.SetPort( port );
            packet->SetAddress( address );

            auto resolveEntry = m_config.resolver->GetEntry( hostname );

            if ( resolveEntry )
            {
                switch ( resolveEntry->status )
                {
                    case RESOLVE_SUCCEEDED:
                    {
                        assert( resolveEntry->result->addresses.size() >= 1 );
                        Address address = resolveEntry->result->addresses[0];
                        if ( port != 0 )
                            address.SetPort( port );
                        SendPacket( address, packet );
                        //printf( "resolve succeeded: sending packet to %s\n", address.ToString().c_str() );
                    }
                    break;

                    case RESOLVE_IN_PROGRESS:
                    {
                        //printf( "resolve in progress: buffering packet\n" );
                        auto resolve_send_queue = m_resolve_send_queues[hostname];
                        assert( resolve_send_queue );
                        resolve_send_queue->push( packet );
                    }
                    break;

                    case RESOLVE_FAILED:
                    {
                        //printf( "resolve failed: discarding packet for \"%s\"\n", hostname.c_str() );
                    }
                    break;
                }
            }
            else
            {
                //printf( "resolving \"%s\": buffering packet\n", hostname.c_str() );
                m_config.resolver->Resolve( hostname );
                auto resolve_send_queue = new PacketQueue();
                resolve_send_queue->push( packet );
                m_resolve_send_queues[hostname] = resolve_send_queue;
            }
        }

        Packet * ReceivePacket()
        {
            return m_config.networkInterface->ReceivePacket();
        }

        void Update( const TimeBase & timeBase )
        {
            m_config.networkInterface->Update( timeBase );

            m_config.resolver->Update( TimeBase() );

            for ( auto itor = m_resolve_send_queues.begin(); itor != m_resolve_send_queues.end(); )
            {
                auto hostname = itor->first;
                auto resolve_send_queue = itor->second;

                auto entry = m_config.resolver->GetEntry( hostname );
                assert( entry );
                if ( entry->status != RESOLVE_IN_PROGRESS )
                {
                    if ( entry->status == RESOLVE_SUCCEEDED )
                    {
                        assert( entry->result->addresses.size() > 0 );
                        auto address = entry->result->addresses[0];
                        //printf( "resolved \"%s\" to %s\n", hostname.c_str(), address.ToString().c_str() );

                        while ( !resolve_send_queue->empty() )
                        {
                            auto packet = resolve_send_queue->front();
                            resolve_send_queue->pop();
                            const uint16_t port = packet->GetAddress().GetPort();
                            if ( port )
                                address.SetPort( port );
                            //printf( "sent buffered packet to %s\n", address.ToString() );
                            SendPacket( address, packet );
                        }
                        m_resolve_send_queues.erase( itor++ );
                    }
                    else if ( entry->status == RESOLVE_FAILED )
                    {
                        //printf( "failed to resolve \"%s\". discarding %d packets\n", hostname.c_str(), resolve_send_queue->size() );
                        m_resolve_send_queues.erase( itor++ );
                    }
                }
                else
                    ++itor;
            }
        }

        uint32_t GetMaxPacketSize() const
        {
            return m_config.networkInterface->GetMaxPacketSize();
        }
    };
}

#endif
