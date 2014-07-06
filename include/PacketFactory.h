#ifndef PROTOCOL_PACKET_FACTORY_H
#define PROTOCOL_PACKET_FACTORY_H

#include "Packet.h"
#include "Factory.h"
#include "Memory.h"

namespace protocol
{
    class PacketFactory : public Factory<Packet>
    {        
        #if PROTOCOL_DEBUG_MEMORY_LEAKS
        std::map<void*,int> allocated_packets;
        #endif

        int num_allocated_packets = 0;

        Allocator * m_allocator;

    public:

        PacketFactory( Allocator & allocator )
        {
            m_allocator = &allocator;
        }

        ~PacketFactory()
        {
            #if PROTOCOL_DEBUG_MEMORY_LEAKS
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
                exit( 1 );
            }
            assert( num_allocated_packets == 0 );
        }

        Packet * Create( int type )
        {
            Packet * packet = Factory<Packet>::Create( type );
            
            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "create packet %p\n", packet );
            allocated_packets[packet] = 1;
            auto itor = allocated_packets.find( packet );
            assert( itor != allocated_packets.end() );
            #endif
            
            num_allocated_packets++;

            return packet;
        }

        void Destroy( Packet * packet )
        {
            assert( packet );

            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "destroy packet %p\n", packet );
            auto itor = allocated_packets.find( packet );
            assert( itor != allocated_packets.end() );
            allocated_packets.erase( packet );
            #endif

            assert( num_allocated_packets > 0 );
            num_allocated_packets--;

            assert( m_allocator );

            PROTOCOL_DELETE( *m_allocator, Packet, packet );
        }
    };
}

#endif
