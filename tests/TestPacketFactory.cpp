#include "TestPackets.h"

using namespace protocol;

void test_packet_factory()
{
    printf( "test_packet_factory\n" );

    memory::initialize();
    {
        TestPacketFactory packetFactory;

        auto connectPacket = packetFactory.Create( PACKET_CONNECT );
        auto updatePacket = packetFactory.Create( PACKET_UPDATE );
        auto disconnectPacket = packetFactory.Create( PACKET_DISCONNECT );

        assert( connectPacket->GetType() == PACKET_CONNECT );
        assert( updatePacket->GetType() == PACKET_UPDATE );
        assert( disconnectPacket->GetType() == PACKET_DISCONNECT );

        packetFactory.Destroy( connectPacket );
        packetFactory.Destroy( updatePacket );
        packetFactory.Destroy( disconnectPacket );
    }
    memory::shutdown();
}
