/*
    Networked Physics Demo

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "network/Address.h"
#include <stdio.h>

#if CORE_PLATFORM == CORE_PLATFORM_WINDOWS

    #define NOMINMAX
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <ws2ipdef.h>
    #pragma comment( lib, "WS2_32.lib" )

    #ifdef SetPort
    #undef SetPort
    #endif // #ifdef SetPort

#else // #if CORE_PLATFORM == CORE_PLATFORM_WINDOWS

	#include <netdb.h>
	#include <arpa/inet.h>

#endif // #if CORE_PLATFORM == CORE_PLATFORM_WINDOWS

#include <memory.h>
#include <string.h>

namespace network
{
    Address::Address()
    {
        Clear();
    }

    Address::Address( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port )
    {
        m_type = ADDRESS_IPV4;
        m_address4 = uint32_t(a) | (uint32_t(b)<<8) | (uint32_t(c)<<16) | (uint32_t(d)<<24);
        m_port = port;
    }

    Address::Address( uint32_t address, int16_t port )
    {
        m_type = ADDRESS_IPV4;
        m_address4 = htonl( address );        // IMPORTANT: stored in network byte order. eg. big endian!
        m_port = port;
    }

    Address::Address( uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                      uint16_t e, uint16_t f, uint16_t g, uint16_t h,
                      uint16_t port )
    {
        m_type = ADDRESS_IPV6;
        m_address6[0] = htons( a );
        m_address6[1] = htons( b );
        m_address6[2] = htons( c );
        m_address6[3] = htons( d );
        m_address6[4] = htons( e );
        m_address6[5] = htons( f );
        m_address6[6] = htons( g );
        m_address6[7] = htons( h );
        m_port = port;
    }

    Address::Address( const uint16_t address[], uint16_t port )
    {
        m_type = ADDRESS_IPV6;
        for ( int i = 0; i < 8; ++i )
            m_address6[i] = htons( address[i] );
        m_port = port;
    }

    Address::Address( const sockaddr_storage & addr )
    {
        if ( addr.ss_family == AF_INET )
        {
            const sockaddr_in & addr_ipv4 = reinterpret_cast<const sockaddr_in&>( addr );
            m_type = ADDRESS_IPV4;
            m_address4 = addr_ipv4.sin_addr.s_addr;
            m_port = ntohs( addr_ipv4.sin_port );
        }
        else if ( addr.ss_family == AF_INET6 )
        {
            const sockaddr_in6 & addr_ipv6 = reinterpret_cast<const sockaddr_in6&>( addr );
            m_type = ADDRESS_IPV6;
            memcpy( m_address6, &addr_ipv6.sin6_addr, 16 );
            m_port = ntohs( addr_ipv6.sin6_port );
        }
        else
        {
            CORE_ASSERT( false );
            Clear();
        }
    }

    Address::Address( const sockaddr_in6 & addr_ipv6 )
    {
        m_type = ADDRESS_IPV6;
        memcpy( m_address6, &addr_ipv6.sin6_addr, 16 );
        m_port = ntohs( addr_ipv6.sin6_port );
    }

    Address::Address( addrinfo * p )
    {
        m_port = 0;
        if ( p->ai_family == AF_INET )
        { 
            m_type = ADDRESS_IPV4;
            struct sockaddr_in * ipv4 = (struct sockaddr_in *)p->ai_addr;
            m_address4 = ipv4->sin_addr.s_addr;
            m_port = ntohs( ipv4->sin_port );
        } 
        else if ( p->ai_family == AF_INET6 )
        { 
            m_type = ADDRESS_IPV6;
            struct sockaddr_in6 * ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            memcpy( m_address6, &ipv6->sin6_addr, 16 );
            m_port = ntohs( ipv6->sin6_port );
        }
        else
        {
            Clear();
        }
    }

    Address::Address( const char * address )
    {
        Parse( address );
    }

    Address::Address( const char * address, uint16_t port )
    {
        Parse( address );
        m_port = port;
    }

    void Address::Parse( const char * address_in )
    {
        // first try to parse as an IPv6 address:
        // 1. if the first character is '[' then it's probably an ipv6 in form "[addr6]:portnum"
        // 2. otherwise try to parse as raw IPv6 address, parse using inet_pton

        CORE_ASSERT( address_in );

        char buffer[256];
        char * address = buffer;
        strncpy( address, address_in, 255 );
        address[255] = '\0';

        int addressLength = (int) strlen( address );
        m_port = 0;
        if ( address[0] == '[' )
        {
            const int base_index = addressLength - 1;
            for ( int i = 0; i < 6; ++i )   // note: no need to search past 6 characters as ":65535" is longest port value
            {
                const int index = base_index - i;
                if ( index < 3 )
                    break;
                if ( address[index] == ':' )
                {
                    m_port = (uint16_t) atoi( &address[index+1] );
                    address[index-1] = '\0';
                }
            }
            address += 1;
        }
        struct in6_addr sockaddr6;
        if ( inet_pton( AF_INET6, address, &sockaddr6 ) == 1 )
        {
            memcpy( m_address6, &sockaddr6, 16 );
            m_type = ADDRESS_IPV6;
            return;
        }

        // otherwise it's probably an IPv4 address:
        // 1. look for ":portnum", if found save the portnum and strip it out
        // 2. parse remaining ipv4 address via inet_pton

        addressLength = (int) strlen( address );
        const int base_index = addressLength - 1;
        for ( int i = 0; i < 6; ++i )   // note: no need to search past 6 characters as ":65535" is longest port value
        {
            const int index = base_index - i;
            if ( index < 0 )
                break;
            if ( address[index] == ':' )
            {
                m_port = (uint16_t) atoi( &address[index+1] );
                address[index] = '\0';
            }
        }

        struct sockaddr_in sockaddr4;
        if ( inet_pton( AF_INET, address, &sockaddr4.sin_addr ) == 1 )
        {
            m_type = ADDRESS_IPV4;
            m_address4 = sockaddr4.sin_addr.s_addr;
        }
        else
        {
            // nope: it's not an IPv4 address. maybe it's a hostname? set address as undefined.
            Clear();
        }
    }

    void Address::Clear()
    {
        m_type = ADDRESS_UNDEFINED;
        memset( m_address6, 0, sizeof( m_address6 ) );
        m_port = 0;
    }

    uint32_t Address::GetAddress4() const
    {
        CORE_ASSERT( m_type == ADDRESS_IPV4 );
        return m_address4;
    }

    const uint16_t * Address::GetAddress6() const
    {
        CORE_ASSERT( m_type == ADDRESS_IPV6 );
        return m_address6;
    }

    void Address::SetPort( uint16_t port )
    {
        m_port = port;
    }

    uint16_t Address::GetPort() const 
    {
        return m_port;
    }

    AddressType Address::GetType() const
    {
        return m_type;
    }

    const char * Address::ToString( char buffer[], int bufferSize ) const
    {
        if ( m_type == ADDRESS_IPV4 )
        {
            const uint8_t a =   m_address4 & 0xff;
            const uint8_t b = ( m_address4 >> 8  ) & 0xff;
            const uint8_t c = ( m_address4 >> 16 ) & 0xff;
            const uint8_t d = ( m_address4 >> 24 ) & 0xff;
            if ( m_port != 0 )
                snprintf( buffer, bufferSize, "%d.%d.%d.%d:%d", a, b, c, d, m_port );
            else
                snprintf( buffer, bufferSize, "%d.%d.%d.%d", a, b, c, d );
            return buffer;
        }
        else if ( m_type == ADDRESS_IPV6 )
        {
            if ( m_port == 0 )
            {
                inet_ntop( AF_INET6, (void*) &m_address6, buffer, bufferSize );
                return buffer;
            }
            else
            {
                char addressString[INET6_ADDRSTRLEN];
                inet_ntop( AF_INET6, (void*) &m_address6, addressString, INET6_ADDRSTRLEN );
                snprintf( buffer, bufferSize, "[%s]:%d", addressString, m_port );
                return buffer;
            }
        }
        else
        {
            return "undefined";
        }
    }

    bool Address::IsValid() const
    {
        return m_type != ADDRESS_UNDEFINED;
    }

    bool Address::operator ==( const Address & other ) const
    {
        if ( m_type != other.m_type )
            return false;
        if ( m_port != other.m_port )
            return false;
        if ( m_type == ADDRESS_IPV4 && m_address4 == other.m_address4 )
            return true;
        else if ( m_type == ADDRESS_IPV6 && memcmp( m_address6, other.m_address6, sizeof( m_address6 ) ) == 0 )
            return true;
        else
            return false;
    }

    bool Address::operator !=( const Address & other ) const
    {
        return !( *this == other );
    }
}
