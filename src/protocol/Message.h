// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_MESSAGE_H
#define PROTOCOL_MESSAGE_H

#include "core/Core.h"
#include "protocol/Object.h"
#include "protocol/Stream.h"

namespace protocol
{
    class MessageFactory;

    class Message : public Object
    {
    public:

        Message( int type ) : m_refCount(1), m_id(0), m_type(type)
        {
            CORE_ASSERT( m_magic == 0x12345 );
        }

        int GetId() const { CORE_ASSERT( m_magic == 0x12345 ); return m_id; }
        int GetType() const { CORE_ASSERT( m_magic == 0x12345 ); return m_type; }

        void SetId( uint16_t id ) { CORE_ASSERT( m_magic == 0x12345 ); m_id = id; }

        bool IsBlock() const { CORE_ASSERT( m_magic == 0x12345 ); return m_type == BlockMessageType; }

        int GetRefCount() { CORE_ASSERT( m_magic == 0x12345 ); return m_refCount; }

    protected:

        void AddRef() { m_refCount++; }
        void Release() { CORE_ASSERT( m_magic == 0x12345 ); CORE_ASSERT( m_refCount > 0 ); m_refCount--; }

        ~Message() 
        { 
            CORE_ASSERT( m_magic == 0x12345 );
            CORE_ASSERT( m_refCount == 0 );
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
