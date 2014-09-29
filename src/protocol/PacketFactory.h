#ifndef PROTOCOL_PACKET_FACTORY_H
#define PROTOCOL_PACKET_FACTORY_H

#include "Packet.h"
#include "Memory.h"

namespace protocol
{
    class PacketFactory
    {        
        #if PROTOCOL_DEBUG_MEMORY_LEAKS
        std::map<void*,int> allocated_packets;
        #endif

        int num_allocated_packets = 0;

        Allocator * m_allocator;

        int m_numTypes;

    public:

        PacketFactory( Allocator & allocator, int numTypes )
        {
            m_allocator = &allocator;
            m_numTypes = numTypes;
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
                PROTOCOL_ASSERT( !"leaked packets" );
            }
            PROTOCOL_ASSERT( num_allocated_packets == 0 );
        }

        Packet * Create( int type )
        {
            PROTOCOL_ASSERT( type >= 0 );
            PROTOCOL_ASSERT( type < m_numTypes );

            Packet * packet = CreateInternal( type );
            
            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "create packet %p\n", packet );
            allocated_packets[packet] = 1;
            auto itor = allocated_packets.find( packet );
            PROTOCOL_ASSERT( itor != allocated_packets.end() );
            #endif
            
            num_allocated_packets++;

            return packet;
        }

        void Destroy( Packet * packet )
        {
            PROTOCOL_ASSERT( packet );

            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "destroy packet %p\n", packet );
            auto itor = allocated_packets.find( packet );
            PROTOCOL_ASSERT( itor != allocated_packets.end() );
            allocated_packets.erase( packet );
            #endif

            PROTOCOL_ASSERT( num_allocated_packets > 0 );
            num_allocated_packets--;

            PROTOCOL_ASSERT( m_allocator );

            PROTOCOL_DELETE( *m_allocator, Packet, packet );
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
