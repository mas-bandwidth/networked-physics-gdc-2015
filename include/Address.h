/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_ADDRESS_H
#define PROTOCOL_ADDRESS_H

#include "Common.h"

namespace protocol
{
    enum class AddressType : uint8_t
    {
        Undefined,
        IPv4,
        IPv6
    };

    class Address
    {
    public:

        Address()
        {
            Clear();
        }

        Address( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t _port = 0 )
        {
            type = AddressType::IPv4;
            address4 = uint32_t(a) | (uint32_t(b)<<8) | (uint32_t(c)<<16) | (uint32_t(d)<<24);
            port = _port;
        }

        explicit Address( uint32_t address, int16_t _port = 0 )
        {
            type = AddressType::IPv4;
            address4 = htonl( address );        // IMPORTANT: stored in network byte order. eg. big endian!
            port = _port;
        }

        explicit Address( uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                          uint16_t e, uint16_t f, uint16_t g, uint16_t h,
                          uint16_t _port = 0 )
        {
            type = AddressType::IPv6;
            address6[0] = htons( a );
            address6[1] = htons( b );
            address6[2] = htons( c );
            address6[3] = htons( d );
            address6[4] = htons( e );
            address6[5] = htons( f );
            address6[6] = htons( g );
            address6[7] = htons( h );
            port = _port;
        }

        explicit Address( const uint16_t _address[], uint16_t _port = 0 )
        {
            type = AddressType::IPv6;
            for ( int i = 0; i < 8; ++i )
                address6[i] = htons( _address[i] );
            port = _port;
        }

        explicit Address( const sockaddr_storage & addr )
        {
            if ( addr.ss_family == AF_INET )
            {
                const sockaddr_in & addr_ipv4 = reinterpret_cast<const sockaddr_in&>( addr );
                type = AddressType::IPv4;
                address4 = addr_ipv4.sin_addr.s_addr;
                port = ntohs( addr_ipv4.sin_port );
            }
            else if ( addr.ss_family == AF_INET6 )
            {
                const sockaddr_in6 & addr_ipv6 = reinterpret_cast<const sockaddr_in6&>( addr );
                type = AddressType::IPv6;
                memcpy( address6, &addr_ipv6.sin6_addr, 16 );
                port = ntohs( addr_ipv6.sin6_port );
            }
            else
            {
                assert( false );
            }
        }

        explicit Address( const sockaddr_in6 & addr_ipv6 )
        {
            type = AddressType::IPv6;
            memcpy( address6, &addr_ipv6.sin6_addr, 16 );
            port = ntohs( addr_ipv6.sin6_port );
        }

        explicit Address( addrinfo * p )
        {
            port = 0;
            if ( p->ai_family == AF_INET )
            { 
                type = AddressType::IPv4;
                struct sockaddr_in * ipv4 = (struct sockaddr_in *)p->ai_addr;
                address4 = ipv4->sin_addr.s_addr;
                port = ntohs( ipv4->sin_port );
            } 
            else if ( p->ai_family == AF_INET6 )
            { 
                type = AddressType::IPv6;
                struct sockaddr_in6 * ipv6 = (struct sockaddr_in6 *)p->ai_addr;
                memcpy( address6, &ipv6->sin6_addr, 16 );
                port = ntohs( ipv6->sin6_port );
            }
            else
            {
                Clear();
            }
        }

        Address( string address )
        {
            // first try to parse as an IPv6 address:
            // 1. if the first character is '[' then it's probably an ipv6 in form "[addr6]:portnum"
            // 2. otherwise try to parse as raw IPv6 address, parse using inet_pton

            port = 0;
            if ( address[0] == '[' )
            {
                const int base_index = address.size() - 1;
                for ( int i = 0; i < 6; ++i )   // note: no need to search past 6 characters as ":65535" is longest port value
                {
                    const int index = base_index - i;
                    if ( address[index] == ':' )
                    {
                        const char * port_string = address.substr( index + 1, 6 ).c_str();
                        port = atoi( port_string );
                        address = address.substr( 1, index - 2 );
                    }
                }
            }
            struct in6_addr sockaddr6;
            if ( inet_pton( AF_INET6, address.c_str(), &sockaddr6 ) == 1 )
            {
                memcpy( address6, &sockaddr6, 16 );
                type = AddressType::IPv6;
                return;
            }

            // otherwise it's probably an IPv4 address:
            // 1. look for ":portnum", if found save the portnum and strip it out
            // 2. parse remaining ipv4 address via inet_pton

            const int base_index = address.size() - 1;
            for ( int i = 0; i < 6; ++i )   // note: no need to search past 6 characters as ":65535" is longest port value
            {
                const int index = base_index - i;
                if ( address[index] == ':' )
                {
                    const char * port_string = address.substr( index + 1, 6 ).c_str();
                    port = atoi( port_string );
                    address = address.substr( 0, index );
                }
            }

            struct sockaddr_in sockaddr4;
            if ( inet_pton( AF_INET, address.c_str(), &sockaddr4.sin_addr ) == 1 )
            {
                type = AddressType::IPv4;
                address4 = sockaddr4.sin_addr.s_addr;
            }
            else
            {
                // nope: it's not an IPv4 address. maybe it's a hostname? set address as undefined.
                Clear();
            }
        }

        void Clear()
        {
            type = AddressType::Undefined;
            memset( address6, 0, sizeof( address6 ) );
            port = 0;
        }

        const uint32_t GetAddress4() const
        {
            assert( type == AddressType::IPv4 );
            return address4;
        }

        const uint16_t * GetAddress6() const
        {
            assert( type == AddressType::IPv6 );
            return address6;
        }

        void SetPort( uint64_t _port )
        {
            port = _port;
        }

        const uint16_t GetPort() const 
        {
            return port;
        }

        AddressType GetType() const
        {
            return type;
        }

        // todo: bring this back once we have a std::string replacement
        /*
        string ToString() const
        {
            if ( type == AddressType::IPv4 )
            {
                const uint8_t a = address4 & 0xff;
                const uint8_t b = (address4>>8) & 0xff;
                const uint8_t c = (address4>>16) & 0xff;
                const uint8_t d = (address4>>24) & 0xff;
                if ( port != 0 )
                    return format_string( "%d.%d.%d.%d:%d", a, b, c, d, port );
                else
                    return format_string( "%d.%d.%d.%d", a, b, c, d );
            }
            else if ( type == AddressType::IPv6 )
            {
                char addressString[INET6_ADDRSTRLEN];
                inet_ntop( AF_INET6, &address6, addressString, INET6_ADDRSTRLEN );
                if ( port != 0 )
                    return format_string( "[%s]:%d", addressString, port );
                else
                    return addressString;
            }
            else
            {
                return "undefined";
            }
        }
        */

        bool IsValid() const
        {
            return type != AddressType::Undefined;
        }

        bool operator ==( const Address & other ) const
        {
            if ( type != other.type )
                return false;
            if ( port != other.port )
                return false;
            if ( type == AddressType::IPv4 && address4 == other.address4 )
                return true;
            else if ( type == AddressType::IPv6 && memcmp( address6, other.address6, sizeof( address6 ) ) == 0 )
                return true;
            else
                return false;
        }

        bool operator !=( const Address & other ) const
        {
            return !( *this == other );
        }

    private:

        AddressType type;

        union
        {
            uint32_t address4;
            uint16_t address6[8];
        };

        uint16_t port;    
    };
}

#endif
