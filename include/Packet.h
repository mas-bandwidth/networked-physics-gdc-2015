/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_PACKET_H
#define PROTOCOL_PACKET_H

#include "Common.h"
#include "Address.h"

namespace protocol
{

    class Packet : public Object
    {
        Address address;
        int type;

    public:
        
        Packet( int _type ) :type(_type) {}

        int GetType() const { return type; }

        void SetAddress( const Address & _address ) { address = _address; }

        const Address & GetAddress() const { return address; }
    };

    typedef queue<shared_ptr<Packet>> PacketQueue;
}

#endif
