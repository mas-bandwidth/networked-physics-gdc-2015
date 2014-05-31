/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_CLIENT_SERVER_PACKETS_H
#define PROTOCOL_CLIENT_SERVER_PACKETS_H

#include "Packet.h"

namespace protocol
{
    // todo: design packets

    enum ClientServerPackets 
    { 
        PACKET_ConnectionRequest,
        PACKET_ChallengeResponse,
        PACKET_Connection

        // etc....
    };

    class ClientServerPacketFactory : public Factory<Packet>
    {
    public:

        ClientServerPacketFactory()
        {
            Register( PACKET_Connection, [this] { return make_shared<ConnectionPacket>( PACKET_Connection, m_interface ); } );
        }

        void SetInterface( shared_ptr<ConnectionInterface> interface )
        {
            m_interface = interface;
        }

    private:

        shared_ptr<ConnectionInterface> m_interface;
    };
}

#endif
