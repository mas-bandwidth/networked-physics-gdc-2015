#ifndef TEST_MESSAGES_H
#define TEST_MESSAGES_H

#include "Factory.h"
#include "Message.h"
#include "BlockMessage.h"
#include "MessageFactory.h"

using namespace protocol;

enum MessageType
{
    MESSAGE_BLOCK = BlockMessageType,
    MESSAGE_TEST
};

inline int GetNumBitsForMessage( uint16_t sequence )
{
    static int messageBitsArray[] = { 1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512, 5, 3, 7, 50 };
    const int modulus = sizeof( messageBitsArray ) / sizeof( int );
    const int index = sequence % modulus;
    return messageBitsArray[index];
}

struct TestMessage : public Message
{
    TestMessage() : Message( MESSAGE_TEST )
    {
        sequence = 0;
        dummy = 0;
    }

    template <typename Stream> void Serialize( Stream & stream )
    {        
        serialize_bits( stream, sequence, 16 );

        int numBits = GetNumBitsForMessage( sequence );
        int numWords = numBits / 32;
        for ( int i = 0; i < numWords; ++i )
            serialize_bits( stream, dummy, 32 );
        int numRemainderBits = numBits - numWords * 32;
        if ( numRemainderBits > 0 )
            serialize_bits( stream, dummy, numRemainderBits );

        serialize_check( stream, 0xDEADBEEF );
    }

    void SerializeRead( ReadStream & stream )
    {
        Serialize( stream );
    }

    void SerializeWrite( WriteStream & stream )
    {
        Serialize( stream );
    }

    void SerializeMeasure( MeasureStream & stream )
    {
        Serialize( stream );
    }

    uint16_t sequence;
    uint32_t dummy;
};

class TestMessageFactory : public MessageFactory
{
public:

    TestMessageFactory()
    {
        // todo: convert to custom allocator
        Register( MESSAGE_BLOCK, [] { return new BlockMessage(); } );
        Register( MESSAGE_TEST,  [] { return new TestMessage();  } );
    }
};

#endif
