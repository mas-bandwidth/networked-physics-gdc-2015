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

#ifndef PROTOCOL_PACKET_FACTORY_H
#define PROTOCOL_PACKET_FACTORY_H

#include "core/Memory.h"
#include "protocol/Packet.h"

namespace protocol
{
    class PacketFactory
    {        
        #if CORE_DEBUG_MEMORY_LEAKS
        std::map<void*,int> allocated_packets;
        #endif

        int num_allocated_packets = 0;

        core::Allocator * m_allocator;

        int m_numTypes;

    public:

        PacketFactory( core::Allocator & allocator, int numTypes )
        {
            m_allocator = &allocator;
            m_numTypes = numTypes;
        }

        ~PacketFactory()
        {
            #if CORE_DEBUG_MEMORY_LEAKS
            if ( allocated_packets.size() )
            {
                printf( "you leaked packets!\n" );
                printf( "%d packets leaked\n", (int) allocated_packets.size() );
                for ( auto itor : allocated_packets )
                {
                    auto p = itor.first;
                    printf( "leaked packet %p\n", p );
                }
                exit(1);
            }
            #endif
            if ( num_allocated_packets != 0 )
            {
                printf( "you leaked packets!\n" );
                printf( "%d packets leaked\n", num_allocated_packets );
                CORE_ASSERT( !"leaked packets" );
            }
            CORE_ASSERT( num_allocated_packets == 0 );
        }

        Packet * Create( int type )
        {
            CORE_ASSERT( type >= 0 );
            CORE_ASSERT( type < m_numTypes );

            Packet * packet = CreateInternal( type );
            
            #if CORE_DEBUG_MEMORY_LEAKS
            printf( "create packet %p\n", packet );
            allocated_packets[packet] = 1;
            auto itor = allocated_packets.find( packet );
            CORE_ASSERT( itor != allocated_packets.end() );
            #endif
            
            num_allocated_packets++;

            return packet;
        }

        void Destroy( Packet * packet )
        {
            CORE_ASSERT( packet );

            #if CORE_DEBUG_MEMORY_LEAKS
            printf( "destroy packet %p\n", packet );
            auto itor = allocated_packets.find( packet );
            CORE_ASSERT( itor != allocated_packets.end() );
            allocated_packets.erase( packet );
            #endif

            CORE_ASSERT( num_allocated_packets > 0 );
            num_allocated_packets--;

            CORE_ASSERT( m_allocator );

            CORE_DELETE( *m_allocator, Packet, packet );
        }

        int GetNumTypes() const
        {
            return m_numTypes;
        }

    protected:

        virtual Packet * CreateInternal( int type ) = 0;     // IMPORTANT: override this to create your own types!
    };
}

#endif
