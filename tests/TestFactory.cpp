#include "TestPackets.h"

using namespace protocol;

void test_factory()
{
    TestPacketFactory packetFactory;

    auto connectPacket = packetFactory.Create( PACKET_CONNECT );
    auto updatePacket = packetFactory.Create( PACKET_UPDATE );
    auto disconnectPacket = packetFactory.Create( PACKET_DISCONNECT );

    assert( connectPacket->GetType() == PACKET_CONNECT );
    assert( updatePacket->GetType() == PACKET_UPDATE );
    assert( disconnectPacket->GetType() == PACKET_DISCONNECT );

    delete connectPacket;
    delete updatePacket;
    delete disconnectPacket;
}
