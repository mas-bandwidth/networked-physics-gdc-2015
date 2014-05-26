#include "Connection.h"
#include "BSDSocketsInterface.h"
#include "ReliableMessageChannel.h"

using namespace std;
using namespace protocol;

enum 
{ 
    PACKET_Connection = 0,
    PACKET_Dummy
};

class PacketFactory : public Factory<Packet>
{
public:
    PacketFactory()
    {
        Register( PACKET_Connection, [this] { return make_shared<ConnectionPacket>( PACKET_Connection, m_interface ); } );
        Register( PACKET_Dummy, [this] { return nullptr; } );
    }

    void SetInterface( shared_ptr<ConnectionInterface> interface )
    {
        assert( interface );
        m_interface = interface;
    }

private:

    shared_ptr<ConnectionInterface> m_interface;
};

enum MessageType
{
    MESSAGE_Block = 0,           // IMPORTANT: 0 is reserved for block messages
    MESSAGE_Test
};

static int messageBitsArray[] = { 1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512, 1000, 5000, 5, 3, 7, 800 };

int GetNumBitsForMessage( uint16_t sequence )
{
    const int modulus = sizeof( messageBitsArray ) / sizeof( int );
    const int index = sequence % modulus;
    return messageBitsArray[index];
}

struct TestMessage : public Message
{
    TestMessage() : Message( MESSAGE_Test )
    {
        sequence = 0;
    }

    void Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );
        int numBits = GetNumBitsForMessage( sequence );
        int numWords = numBits / 32;
        for ( int i = 0; i < numWords; ++i )
        {
            int dummy = 0;
            serialize_bits( stream, dummy, 32 );
        }
        int numRemainderBits = numBits - numWords * 32;
        if ( numRemainderBits > 0 )
        {
            int dummy = 0;
            serialize_bits( stream, dummy, numRemainderBits );
        }
    }

    // todo: add some pattern for the bits, eg. 0,1,0,1 etc. so i can verify
    // connect contents read back on the receive side of message

    uint16_t sequence;
};

class MessageFactory : public Factory<Message>
{
public:
    MessageFactory()
    {
        Register( MESSAGE_Block, [] { return make_shared<BlockMessage>(); } );
        Register( MESSAGE_Test, [] { return make_shared<TestMessage>(); } );
    }
};

void soak_test()
{
    cout << "[soak test]" << endl;

    auto packetFactory = make_shared<PacketFactory>();
    auto messageFactory = make_shared<MessageFactory>();

    const int MaxPacketSize = 4096;

    BSDSocketsInterfaceConfig interfaceConfig;
    interfaceConfig.port = 10000;
    interfaceConfig.family = AF_INET6;
    interfaceConfig.maxPacketSize = MaxPacketSize;
    interfaceConfig.packetFactory = static_pointer_cast<Factory<Packet>>( packetFactory );
    
    BSDSocketsInterface interface( interfaceConfig );

    Address address( "::1" );
    address.SetPort( interfaceConfig.port );

    ConnectionConfig connectionConfig;
    connectionConfig.packetType = PACKET_Connection;
    connectionConfig.maxPacketSize = MaxPacketSize;
    connectionConfig.packetFactory = packetFactory;

    Connection connection( connectionConfig );

    packetFactory->SetInterface( connection.GetInterface() );

    ReliableMessageChannelConfig messageChannelConfig;
    messageChannelConfig.maxMessagesPerPacket = 256;
    messageChannelConfig.sendQueueSize = 2048;
    messageChannelConfig.receiveQueueSize = 512;
    messageChannelConfig.packetBudget = 4000;
    messageChannelConfig.maxMessageSize = 1024;
    messageChannelConfig.blockFragmentSize = 3900;
    messageChannelConfig.maxLargeBlockSize = 32 * 1024 * 1024;
    messageChannelConfig.messageFactory = static_pointer_cast<Factory<Message>>( messageFactory );
    
    auto messageChannel = make_shared<ReliableMessageChannel>( messageChannelConfig );
    
    connection.AddChannel( messageChannel );

    double dt = 1.0 / 100;
    chrono::milliseconds ms( 1 );

    uint16_t sendMessageId = 0;
    uint64_t numMessagesSent = 0;
    uint64_t numMessagesReceived = 0;

    TimeBase timeBase;
    timeBase.time = 0.0;
    timeBase.deltaTime = dt;

    while ( true )
    {
        const int maxMessagesToSend = 1 + rand() % 32;

        for ( int i = 0; i < maxMessagesToSend; ++i )
        {
            if ( !messageChannel->CanSendMessage() )
                break;

            int value = rand() % 10000;

            if ( value < 5000 )
            {
                // bitpacked message

//              cout << format_string( "%09.2f - sent message %d", timeBase.time, message->GetId() ) << endl;

                auto message = make_shared<TestMessage>();
                message->sequence = sendMessageId;
                messageChannel->SendMessage( message );
            }
            else if ( value < 9999 )
            {
                // small block

//                cout << format_string( "%09.2f - sent small block %d", timeBase.time, message->GetId() ) << endl;

                int index = sendMessageId % 32;
                auto block = make_shared<Block>( index + 1, index );
                messageChannel->SendBlock( block );
            }
            else
            {
                // large block

//                  cout << format_string( "%09.2f - sent large block %d", timeBase.time, message->GetId() ) << endl;

                    int index = sendMessageId % 4;
                    auto block = make_shared<Block>( (index+1) * 1024 * 1000 + index, index );
                    messageChannel->SendBlock( block );
            }

            sendMessageId++;
            numMessagesSent++;
        }

        auto packet = connection.WritePacket();

//        cout << format_string( "%09.2f - sent packet %d", timeBase.time, packet->sequence ) << endl;

        interface.Update();

        interface.SendPacket( address, packet );

        connection.Update( timeBase );

        this_thread::sleep_for( ms );

        while ( true )
        {
            auto packet = interface.ReceivePacket();
            if ( !packet )
                break;

            if ( ( rand() % 20 ) == 0 )
                continue;

            assert( packet->GetAddress() == address );
            assert( packet->GetType() == PACKET_Connection );

            auto connectionPacket = static_pointer_cast<ConnectionPacket>( packet );

//            cout << format_string( "%09.2f - received packet %d", timeBase.time, connectionPacket->sequence ) << endl;

            connection.ReadPacket( connectionPacket );
        }

        while ( true )
        {
            auto message = messageChannel->ReceiveMessage();

            if ( !message )
                break;

            assert( message->GetId() == numMessagesReceived % 65536 );
            assert( message->GetType() == MESSAGE_Block || message->GetType() == MESSAGE_Test );

            if ( message->GetType() == MESSAGE_Test )
            {
                cout << format_string( "%09.2f - received message %d - test message", timeBase.time, message->GetId() ) << endl;
            }
            else
            {
                auto blockMessage = static_pointer_cast<BlockMessage>( message );
                auto block = blockMessage->GetBlock();
                assert( block );
                
                if ( block->size() <= messageChannelConfig.maxSmallBlockSize )
                {
                    const int index = numMessagesReceived % 32;
                    assert( block->size() == index + 1 );
                    for ( auto c : *block )
                        assert( c == index );
                    cout << format_string( "%09.2f - received message %d - small block", timeBase.time, message->GetId() ) << endl;
                }
                else
                {
                    const int index = numMessagesReceived % 4;
                    assert( block->size() == (index + 1 ) * 1024 * 1000 + index );
                    for ( auto c : *block )
                        assert( c == index );
                    cout << format_string( "%09.2f - received message %d - large block", timeBase.time, message->GetId() ) << endl;
                }
            }

            numMessagesReceived++;
        }

        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesSent ) == numMessagesSent );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesReceived ) == numMessagesReceived );
        assert( messageChannel->GetCounter( ReliableMessageChannel::MessagesDiscardedEarly ) == 0 );

        timeBase.time += dt;
    }
}

int main()
{
    if ( !InitializeSockets() )
    {
        cerr << "failed to initialize sockets" << endl;
        return 1;
    }

    try
    {
        soak_test();
    }
    catch ( runtime_error & e )
    {
        cerr << string( "error: " ) + e.what() << endl;
    }

    ShutdownSockets();

    return 0;
}
