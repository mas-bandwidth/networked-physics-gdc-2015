#ifndef PROTOCOL_MESSAGE_FACTORY_H
#define PROTOCOL_MESSAGE_FACTORY_H

#include "Factory.h"
#include "Message.h"
#include "Memory.h"

namespace protocol
{
    class MessageFactory : public Factory<Message>
    {        
        #if PROTOCOL_DEBUG_MEMORY_LEAKS
        std::map<void*,int> allocated_messages;
        #endif

        Allocator * m_allocator;

        int num_allocated_messages = 0;

    public:

        MessageFactory( Allocator & allocator )
        {
            m_allocator = &allocator;
        }

        ~MessageFactory()
        {
            assert( m_allocator );
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
            assert( num_allocated_messages == 0 );
        }

        Message * Create( int type )
        {
            Message * message = Factory<Message>::Create( type );

            assert( message );

            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "create message %p\n", message );
            allocated_messages[message] = 1;
            auto itor = allocated_messages.find( message );
            assert( itor != allocated_messages.end() );
            #endif

            num_allocated_messages++;

            return message;
        }

        void AddRef( Message * message )
        {
            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "addref message %p (%d->%d)\n", message, message->GetRefCount(), message->GetRefCount()+1 );
            #endif
            
            assert( message );
            
            message->AddRef();
        }

        void Release( Message * message )
        {
            assert( message );

            #if PROTOCOL_DEBUG_MEMORY_LEAKS
            printf( "release message %p (%d->%d)\n", message, message->GetRefCount(), message->GetRefCount()-1 );
            #endif

            assert( message );
            
            message->Release();
            
            if ( message->GetRefCount() == 0 )
            {
                #if PROTOCOL_DEBUG_MEMORY_LEAKS
                printf( "destroy message %p\n", message );
                auto itor = allocated_messages.find( message );
                assert( itor != allocated_messages.end() );
                allocated_messages.erase( message );
                #endif
            
                num_allocated_messages--;

                assert( m_allocator );

                PROTOCOL_DELETE( *m_allocator, Message, message );
            }
        }
    };
}

#endif
