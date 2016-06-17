/*
    Networked Physics Demo

    Copyright © 2008 - 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
