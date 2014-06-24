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
}

#endif
