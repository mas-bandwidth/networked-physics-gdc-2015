/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_MESSAGE_H
#define PROTOCOL_MESSAGE_H

#include "Common.h"
#include "Stream.h"

namespace protocol
{
    class MessageFactory;

    class Message : public Object
    {
    public:

        Message( int type ) : m_refCount(1), m_id(0), m_type(type)
        {
            PROTOCOL_ASSERT( m_magic == 0x12345 );
        }

        int GetId() const { PROTOCOL_ASSERT( m_magic == 0x12345 ); return m_id; }
        int GetType() const { PROTOCOL_ASSERT( m_magic == 0x12345 ); return m_type; }

        void SetId( uint16_t id ) { PROTOCOL_ASSERT( m_magic == 0x12345 ); m_id = id; }

        bool IsBlock() const { PROTOCOL_ASSERT( m_magic == 0x12345 ); return m_type == BlockMessageType; }

        int GetRefCount() { PROTOCOL_ASSERT( m_magic == 0x12345 ); return m_refCount; }

    protected:

        void AddRef() { m_refCount++; }
        void Release() { PROTOCOL_ASSERT( m_magic == 0x12345 ); PROTOCOL_ASSERT( m_refCount > 0 ); m_refCount--; }

        ~Message() 
        { 
            PROTOCOL_ASSERT( m_magic == 0x12345 );
            PROTOCOL_ASSERT( m_refCount == 0 );
            #ifndef NDEBUG
            m_magic = 0;
            #endif
        }

    private:

        friend class MessageFactory;
      
        Message( const Message & other );
        const Message & operator = ( const Message & other );

        #ifndef NDEBUG
        uint32_t m_magic = 0x12345;
        #endif
        int m_refCount;
        uint32_t m_id : 16;
        uint32_t m_type : 16;       
    };
}

#endif
