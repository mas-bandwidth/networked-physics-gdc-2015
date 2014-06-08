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
        shared_ptr<Resolver> resolver;                      // resolver eg: DNS
        shared_ptr<NetworkInterface> networkInterface;      // the network interface we are wrapping
    };

    class ResolveWrapper : public NetworkInterface
    {
        ResolveWrapperConfig m_config;

        map<string,shared_ptr<PacketQueue>> m_resolve_send_queues;

    public:

        ResolveWrapper( const ResolveWrapperConfig & config )
            : m_config( config )
        {
            assert( m_config.resolver );
            assert( m_config.networkInterface );
        }

        void SendPacket( const Address & address, shared_ptr<Packet> packet )
        {
            m_config.networkInterface->SendPacket( address, packet );
        }

        void SendPacket( const string & hostname, uint16_t port, shared_ptr<Packet> packet )
        {
            Address address;
            address.SetPort( port );
            packet->SetAddress( address );

            auto resolveEntry = m_config.resolver->GetEntry( hostname );

            if ( resolveEntry )
            {
                switch ( resolveEntry->status )
                {
                    case ResolveStatus::Succeeded:
                    {
                        assert( resolveEntry->result->addresses.size() >= 1 );
                        Address address = resolveEntry->result->addresses[0];
                        address.SetPort( port );
                        SendPacket( address, packet );
                        cout << "resolve succeeded: sending packet to " << address.ToString() << endl;
                    }
                    break;

                    case ResolveStatus::InProgress:
                    {
                        cout << "resolve in progress: buffering packet" << endl;
                        auto resolve_send_queue = m_resolve_send_queues[hostname];
                        assert( resolve_send_queue );
                        resolve_send_queue->push( packet );
                    }
                    break;

                    case ResolveStatus::Failed:
                    {
                        cout << "resolve failed: discarding packet for \"" << hostname << "\"" << endl;
                    }
                    break;
                }
            }
            else
            {
                cout << "resolving \"" << hostname << "\": buffering packet" << endl;
                m_config.resolver->Resolve( hostname );
                auto resolve_send_queue = make_shared<PacketQueue>();
                resolve_send_queue->push( packet );
                m_resolve_send_queues[hostname] = resolve_send_queue;
            }
        }

        shared_ptr<Packet> ReceivePacket()
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
                if ( entry->status != ResolveStatus::InProgress )
                {
                    if ( entry->status == ResolveStatus::Succeeded )
                    {
                        assert( entry->result->addresses.size() > 0 );
                        auto address = entry->result->addresses[0];
                        cout << format_string( "resolved \"%s\" to %s", hostname.c_str(), address.ToString().c_str() ) << endl;

                        while ( !resolve_send_queue->empty() )
                        {
                            auto packet = resolve_send_queue->front();
                            resolve_send_queue->pop();
                            const uint16_t port = packet->GetAddress().GetPort();
                            address.SetPort( port );
                            cout << "sent buffered packet to " << address.ToString() << endl;
                            SendPacket( address, packet );
                        }
                        m_resolve_send_queues.erase( itor++ );
                    }
                    else if ( entry->status == ResolveStatus::Failed )
                    {
                        cout << "failed to resolve \"" << hostname << "\". discarding " << resolve_send_queue->size() << " packets" << endl;
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
