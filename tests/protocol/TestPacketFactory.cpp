#include "TestPackets.h"

void test_packet_factory()
{
    printf( "test_packet_factory\n" );

    core::memory::initialize();
    {
        TestPacketFactory packetFactory( core::memory::default_allocator() );

        auto connectPacket = packetFactory.Create( PACKET_CONNECT );
        auto updatePacket = packetFactory.Create( PACKET_UPDATE );
        auto disconnectPacket = packetFactory.Create( PACKET_DISCONNECT );

        CORE_CHECK( connectPacket->GetType() == PACKET_CONNECT );
        CORE_CHECK( updatePacket->GetType() == PACKET_UPDATE );
        CORE_CHECK( disconnectPacket->GetType() == PACKET_DISCONNECT );

        packetFactory.Destroy( connectPacket );
        packetFactory.Destroy( updatePacket );
        packetFactory.Destroy( disconnectPacket );
    }
    core::memory::shutdown();
}
