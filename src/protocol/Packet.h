// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_PACKET_H
#define PROTOCOL_PACKET_H

#include "core/Core.h"
#include "network/Address.h"
#include "protocol/Object.h"

namespace protocol
{
    class PacketFactory;

    class Packet : public Object
    {
        network::Address address;
        int type;

    public:
        
        Packet( int _type ) : type(_type) {}

        int GetType() const { return type; }

        void SetAddress( const network::Address & _address ) { address = _address; }

        const network::Address & GetAddress() const { return address; }

    protected:

        virtual ~Packet() {}

        friend class PacketFactory;

    private:

        Packet( const Packet & other );
        Packet & operator = ( const Packet & other );
    };
}

#endif
