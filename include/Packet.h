/*
    Network Protocol Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_PACKET_H
#define PROTOCOL_PACKET_H

#include "Common.h"
#include "Address.h"

namespace protocol
{
    class PacketFactory;

    class Packet : public Object
    {
        Address address;
        int type;

    public:
        
        Packet( int _type ) : type(_type) {}

        int GetType() const { return type; }

        void SetAddress( const Address & _address ) { address = _address; }

        const Address & GetAddress() const { return address; }

    protected:

        virtual ~Packet() {}

        friend class PacketFactory;

    private:

        Packet( const Packet & other );
        Packet & operator = ( const Packet & other );
    };
}

#endif
