// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#include "protocol/Connection.h"
#include "core/Memory.h"

namespace protocol
{
    Connection::Connection( const ConnectionConfig & config ) : m_config( config )
    {
        CORE_ASSERT( config.packetFactory );
        CORE_ASSERT( config.channelStructure );

        m_allocator = config.allocator ? config.allocator : &core::memory::default_allocator();

        m_sentPackets = CORE_NEW( *m_allocator, SentPackets, *m_allocator, m_config.slidingWindowSize );
        
        m_receivedPackets = CORE_NEW( *m_allocator, ReceivedPackets, *m_allocator, m_config.slidingWindowSize );

        m_numChannels = config.channelStructure->GetNumChannels();
        for ( int i = 0; i < m_numChannels; ++i )
        {
            m_channels[i] = config.channelStructure->CreateChannel( i );
            m_channels[i]->SetContext( config.context );
            CORE_ASSERT( m_channels[i] );
        }

        Reset();
    }

    Connection::~Connection()
    {
        CORE_ASSERT( m_sentPackets );
        CORE_ASSERT( m_receivedPackets );
        CORE_ASSERT( m_channels );

        for ( int i = 0; i < m_numChannels; ++i )
        {
            CORE_ASSERT( m_channels[i] );
            m_config.channelStructure->DestroyChannel( m_channels[i] );
        }

        CORE_DELETE( *m_allocator, SentPackets, m_sentPackets );
        CORE_DELETE( *m_allocator, ReceivedPackets, m_receivedPackets );

        m_sentPackets = nullptr;
        m_receivedPackets = nullptr;
    }

    Channel * Connection::GetChannel( int index )
    {
        CORE_ASSERT( index >= 0 );
        CORE_ASSERT( index < m_numChannels );
        return m_channels[index];
    }

    void Connection::Reset()
    {
        m_error = CONNECTION_ERROR_NONE;

        m_timeBase = core::TimeBase();

        m_sentPackets->Reset();
        m_receivedPackets->Reset();

        for ( int i = 0; i < m_numChannels; ++i )
            m_channels[i]->Reset();

        memset( m_counters, 0, sizeof( m_counters ) );
    }

    void Connection::Update( const core::TimeBase & timeBase )
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return;

        m_timeBase = timeBase;

        for ( int i = 0; i < m_numChannels; ++i )
        {
            m_channels[i]->Update( timeBase );

            if ( m_channels[i]->GetError() != 0 )
            {
                m_error = CONNECTION_ERROR_CHANNEL;
                return;
            }
        }
    }

    ConnectionError Connection::GetError() const
    {
        return m_error;
    }

    int Connection::GetChannelError( int channelIndex ) const
    {
        CORE_ASSERT( channelIndex >= 0 );
        CORE_ASSERT( channelIndex < m_numChannels );
        return m_channels[channelIndex]->GetError();
    }

    const core::TimeBase & Connection::GetTimeBase() const
    {
        return m_timeBase;
    }

    ConnectionPacket * Connection::WritePacket()
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return nullptr;

        auto packet = static_cast<ConnectionPacket*>( m_config.packetFactory->Create( CONNECTION_PACKET ) );

        packet->sequence = m_sentPackets->GetSequence();

        GenerateAckBits( *m_receivedPackets, packet->ack, packet->ack_bits );

        for ( int i = 0; i < m_numChannels; ++i )
            packet->channelData[i] = m_channels[i]->GetData( packet->sequence );

        auto entry = m_sentPackets->Insert( packet->sequence );
        CORE_ASSERT( entry );
        entry->acked = 0;

        m_counters[CONNECTION_COUNTER_PACKETS_WRITTEN]++;

        return packet;
    }

    bool Connection::ReadPacket( ConnectionPacket * packet )
    {
        if ( m_error != CONNECTION_ERROR_NONE )
            return false;

        CORE_ASSERT( packet );
        CORE_ASSERT( packet->GetType() == CONNECTION_PACKET );

//            printf( "read packet %d\n", (int) packet->sequence );

        ProcessAcks( packet->ack, packet->ack_bits );

        m_counters[CONNECTION_COUNTER_PACKETS_READ]++;

        bool discardPacket = false;

        for ( int i = 0; i < m_numChannels; ++i )
        {
            if ( !packet->channelData[i] )
                continue;

            auto result = m_channels[i]->ProcessData( packet->sequence, packet->channelData[i] );

            if ( !result )
                discardPacket = true;
        }

        if ( discardPacket || !m_receivedPackets->Insert( packet->sequence ) )
        {
            m_counters[CONNECTION_COUNTER_PACKETS_DISCARDED]++;
            return false;            
        }

        return true;
    }

    uint64_t Connection::GetCounter( int index ) const
    {
        CORE_ASSERT( index >= 0 );
        CORE_ASSERT( index < CONNECTION_COUNTER_NUM_COUNTERS );
        return m_counters[index];
    }

    void Connection::ProcessAcks( uint16_t ack, uint32_t ack_bits )
    {
//            printf( "process acks: %d - %x\n", (int)ack, ack_bits );

        for ( int i = 0; i < 32; ++i )
        {
            if ( ack_bits & 1 )
            {                    
                const uint16_t sequence = ack - i;
                SentPacketData * packetData = m_sentPackets->Find( sequence );
                if ( packetData && !packetData->acked )
                {
                    PacketAcked( sequence );
                    packetData->acked = 1;
                }
            }
            ack_bits >>= 1;
        }
    }

    void Connection::PacketAcked( uint16_t sequence )
    {
//            printf( "packet %d acked\n", (int) sequence );

        m_counters[CONNECTION_COUNTER_PACKETS_ACKED]++;

        for ( int i = 0; i < m_numChannels; ++i )
            m_channels[i]->ProcessAck( sequence );
    }
}
