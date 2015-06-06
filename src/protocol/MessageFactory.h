// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_MESSAGE_FACTORY_H
#define PROTOCOL_MESSAGE_FACTORY_H

#include "protocol/Message.h"
#include "core/Memory.h"

namespace protocol
{
    class MessageFactory
    {        
        #if PROTOCOL_DEBUG_MEMORY_LEAKS
        std::map<void*,int> allocated_messages;
        #endif

        core::Allocator * m_allocator;

        int num_allocated_messages = 0;

        int m_numTypes;

    public:

        MessageFactory( core::Allocator & allocator, int numTypes )
        {
            m_allocator = &allocator;
            m_numTypes = numTypes;
        }

        ~MessageFactory()
        {
            CORE_ASSERT( m_allocator );
            m_allocator = nullptr;

            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            if ( allocated_messages.size() )
            {
                printf( "you leaked messages!\n" );
                printf( "%d messages leaked\n", (int) allocated_messages.size() );
                for ( auto itor : allocated_messages )
                {
                    auto p = itor.first;
                    printf( "leaked message %p (%d)\n", p, ((Message*)p)->GetRefCount() );
                }
                exit(1);
            }
            #endif
            if ( num_allocated_messages != 0 )
            {
                printf( "you leaked messages!\n" );
                printf( "%d messages leaked\n", num_allocated_messages );
                exit(1);
            }
            CORE_ASSERT( num_allocated_messages == 0 );
        }

        Message * Create( int type )
        {
            CORE_ASSERT( type >= 0 );
            CORE_ASSERT( type < m_numTypes );

            Message * message = CreateInternal( type );

            CORE_ASSERT( message );

            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "create message %p\n", message );
            allocated_messages[message] = 1;
            auto itor = allocated_messages.find( message );
            CORE_ASSERT( itor != allocated_messages.end() );
            #endif

            num_allocated_messages++;

            return message;
        }

        void AddRef( Message * message )
        {
            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "addref message %p (%d->%d)\n", message, message->GetRefCount(), message->GetRefCount()+1 );
            #endif
            
            CORE_ASSERT( message );
            
            message->AddRef();
        }

        void Release( Message * message )
        {
            CORE_ASSERT( message );

            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "release message %p (%d->%d)\n", message, message->GetRefCount(), message->GetRefCount()-1 );
            #endif

            CORE_ASSERT( message );
            
            message->Release();
            
            if ( message->GetRefCount() == 0 )
            {
                #if PROTOCOL_DEBUG_MEMORY_LEAKS
                printf( "destroy message %p\n", message );
                auto itor = allocated_messages.find( message );
                CORE_ASSERT( itor != allocated_messages.end() );
                allocated_messages.erase( message );
                #endif
            
                num_allocated_messages--;

                CORE_ASSERT( m_allocator );

                CORE_DELETE( *m_allocator, Message, message );
            }
        }

        int GetNumTypes() const
        {
            return m_numTypes;
        }

    protected:

        virtual Message * CreateInternal( int type ) = 0;
    };
}

#endif
