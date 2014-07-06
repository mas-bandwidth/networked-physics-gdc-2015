#include "TestPackets.h"

using namespace protocol;

void test_packet_factory()
{
    printf( "test_packet_factory\n" );

    memory::initialize();
    {
        TestPacketFactory packetFactory( memory::default_allocator() );

        auto connectPacket = packetFactory.Create( PACKET_CONNECT );
        auto updatePacket = packetFactory.Create( PACKET_UPDATE );
        auto disconnectPacket = packetFactory.Create( PACKET_DISCONNECT );

        check( connectPacket->GetType() == PACKET_CONNECT );
        check( updatePacket->GetType() == PACKET_UPDATE );
        check( disconnectPacket->GetType() == PACKET_DISCONNECT );

        packetFactory.Destroy( connectPacket );
        packetFactory.Destroy( updatePacket );
        packetFactory.Destroy( disconnectPacket );
    }
    memory::shutdown();
}
