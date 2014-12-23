#ifndef GAME_MESSAGES_H
#define GAME_MESSAGES_H

#include "protocol/Message.h"
#include "protocol/BlockMessage.h"
#include "protocol/MessageFactory.h"
#include "clientServer/ClientServerEnums.h"
#include "GameContext.h"

enum MessageType
{
    MESSAGE_BLOCK = protocol::BlockMessageType,
    MESSAGE_TEST,
    NUM_MESSAGE_TYPES
};

inline int GetNumBitsForMessage( uint16_t sequence )
{
    static int messageBitsArray[] = { 1, 320, 120, 4, 256, 45, 11, 13, 101, 100, 84, 95, 203, 2, 3, 8, 512, 5, 3, 7, 50 };
    const int modulus = sizeof( messageBitsArray ) / sizeof( int );
    const int index = sequence % modulus;
    return messageBitsArray[index];
}

struct TestMessage : public protocol::Message
{
    TestMessage() : Message( MESSAGE_TEST )
    {
        sequence = 0;
        value = 0;
    }

    PROTOCOL_SERIALIZE_OBJECT( stream )
    {
        serialize_bits( stream, sequence, 16 );

        int numBits = GetNumBitsForMessage( sequence ) / 2;
        int numWords = numBits / 32;
        uint32_t dummy = 0;
        for ( int i = 0; i < numWords; ++i )
            serialize_bits( stream, dummy, 32 );
        int numRemainderBits = numBits - numWords * 32;
        if ( numRemainderBits > 0 )
            serialize_bits( stream, dummy, numRemainderBits );

        auto gameContext = (const GameContext*) stream.GetContext( clientServer::CONTEXT_USER );
        CORE_ASSERT( gameContext );
        serialize_int( stream, value, gameContext->value_min, gameContext->value_max );

        CORE_CHECK( serialize_check( stream, 0xDEADBEEF ) );
    }

    uint16_t sequence;
    int value;
};

class GameMessageFactory : public protocol::MessageFactory
{
    core::Allocator * m_allocator;

public:

    GameMessageFactory( core::Allocator & allocator )
        : MessageFactory( allocator, NUM_MESSAGE_TYPES )
    {
        m_allocator = &allocator;
    }

protected:

    protocol::Message * CreateInternal( int type )
    {
        switch ( type )
        {
            case MESSAGE_BLOCK:     return CORE_NEW( *m_allocator, protocol::BlockMessage );
            case MESSAGE_TEST:      return CORE_NEW( *m_allocator, TestMessage );
            default:
                return nullptr;
        }
    }
};

#endif // #ifndef GAME_MESSAGES_H
