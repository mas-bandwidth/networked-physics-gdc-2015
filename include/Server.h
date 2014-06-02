/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_SERVER_H
#define PROTOCOL_SERVER_H

#include "Connection.h"
#include "NetworkInterface.h"
#include "ClientServerPackets.h"

namespace protocol
{
    enum ConnectionRequestDenyReason
    {
        CONNECTION_REQUEST_DENIED_ServerClosed,                 // server is closed. all connection requests are denied.
    };

    struct ServerConfig
    {
        uint64_t protocolId = 42;                               // the protocol id. must be the same for client and server to talk.

        shared_ptr<NetworkInterface> networkInterface;          // network interface used to send and receive packets

        shared_ptr<ChannelStructure> channelStructure;          // defines the connection channel structure
    };

    class Server
    {
        ServerConfig m_config;
        TimeBase m_timeBase;
        bool m_open = true;

    public:

        Server( const ServerConfig & config )
            : m_config( config )
        {
            assert( m_config.networkInterface );
            assert( m_config.channelStructure );

            // todo: 
        }

        void Update( const TimeBase & timeBase )
        {
            m_timeBase = timeBase;

            UpdateClients();

            UpdateNetworkInterface();

            UpdateReceivePackets();
        }

        void Open()
        {
            m_open = true;
        }

        void Close()
        {
            m_open = false;
        }

        bool IsOpen() const
        {
            return m_open;
        }

    protected:

        void UpdateClients()
        {
            // todo: iterate across each client state machine and update, send packets etc.
        }

        void UpdateNetworkInterface()
        {
            assert( m_config.networkInterface );
            m_config.networkInterface->Update( m_timeBase );
        }

        void UpdateReceivePackets()
        {
            while ( true )
            {
                auto packet = m_config.networkInterface->ReceivePacket();
                if ( !packet )
                    break;

                switch ( packet->GetType() )
                {
                    case PACKET_ConnectionRequest:
                    {
                        auto connectionRequestPacket = static_pointer_cast<ConnectionChallengePacket>( packet );

                        cout << "server received connection request packet" << endl;

                        if ( !m_open )
                        {
                            cout << "server is closed. denying connection request" << endl;

                            auto connectionDeniedPacket = make_shared<ConnectionDeniedPacket>();
                            connectionDeniedPacket->protocolId = m_config.protocolId;
                            connectionDeniedPacket->clientGuid = connectionRequestPacket->clientGuid;
                            connectionDeniedPacket->reason = CONNECTION_REQUEST_DENIED_ServerClosed;

                            m_config.networkInterface->SendPacket( packet->GetAddress(), connectionDeniedPacket );
                        }
                    }
                    break;

                    default:
                        break;
                }
            }
        }
    };
}

#endif
