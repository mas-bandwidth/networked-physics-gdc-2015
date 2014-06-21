/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_MESSAGE_H
#define PROTOCOL_MESSAGE_H

#include "Common.h"
#include "Stream.h"

namespace protocol
{
    class Message : public Object
    {
    public:

        Message( int type ) : m_refCount(0), m_id(0), m_type(type)
        {
            assert( m_magic == 0x12345 );             
        }

        ~Message() 
        { 
            assert( m_magic == 0x12345 );
            assert( m_refCount == 0 ); 
            m_magic = 0;
        }

        int GetId() const { assert( m_magic == 0x12345 ); return m_id; }
        int GetType() const { assert( m_magic == 0x12345 ); return m_type; }

        void SetId( uint16_t id ) { assert( m_magic == 0x12345 ); m_id = id; }

        bool IsBlock() const { assert( m_magic == 0x12345 ); return m_type == BlockMessageType; }

        void AddRef() { m_refCount++; }
        void Release() { assert( m_magic == 0x12345 ); assert( m_refCount > 0 ); m_refCount--; }
        int GetRefCount() { assert( m_magic == 0x12345 ); return m_refCount; }

    private:
      
        Message( const Message & other );
        const Message & operator = ( const Message & other );

        #ifndef NDEBUG
        uint32_t m_magic = 0x12345;
        #endif
        int m_refCount;
        uint32_t m_id : 16;
        uint32_t m_type : 16;       
    };

    class BlockMessage : public Message
    {
    public:

        BlockMessage() : Message( BlockMessageType ) {}

        BlockMessage( Block * block ) : Message( BlockMessageType ) 
        { 
            assert( block ); 
            m_block = block; 
        }

        ~BlockMessage()
        {
            assert( m_block );
            delete m_block;
            m_block = nullptr;
        }

        template <typename Stream> void Serialize( Stream & stream )
        { 
            serialize_block( stream, m_block, MaxSmallBlockSize );
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

        Block * GetBlock() { return m_block; }

    private:

        Block * m_block = nullptr;

        BlockMessage( const BlockMessage & other );
        const BlockMessage & operator = ( const BlockMessage & other );
    };
}

#endif
