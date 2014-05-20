/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_STREAM_H
#define PROTOCOL_STREAM_H

#include "Common.h"

namespace protocol
{
    struct StreamValue
    {
        StreamValue( int64_t _value, int64_t _min, int64_t _max )
            : value( _value ),
              min( _min ),
              max( _max ) {}

        int64_t value;         // the actual value to be serialized
        int64_t min;           // the minimum value
        int64_t max;           // the maximum value
    };

    enum StreamMode
    {
        STREAM_Read,
        STREAM_Write
    };

    class Stream
    {
    public:

        Stream( StreamMode mode )
        {
            SetMode( mode );
        }

        void SetMode( StreamMode mode )
        {
            m_mode = mode;
            m_readIndex = 0;
        }

        bool IsReading() const 
        {
            return m_mode == STREAM_Read;
        }

        bool IsWriting() const
        {
            return m_mode == STREAM_Write;
        }

        void SerializeValue( int64_t & value, int64_t min, int64_t max )
        {
            // note: this is a dummy stream implementation to be replaced with a bitpacker, range encoder or arithmetic encoder in future

            if ( m_mode == STREAM_Write )
            {
                assert( value >= min );
                assert( value <= max );
                m_values.push_back( StreamValue( value, min, max ) );
            }
            else
            {
                if ( m_readIndex >= m_values.size() )
                    throw runtime_error( "read past end of stream" );

                StreamValue & streamValue = m_values[m_readIndex++];

                if ( streamValue.min != min || streamValue.max != max )
                    throw runtime_error( format_string( "min/max stream mismatch: [%lld,%lld] vs. [%lld,%lld]", streamValue.min, streamValue.max, min, max ) );

                if ( streamValue.value < min || streamValue.value > max )
                    throw runtime_error( format_string( "value %lld read from stream is outside min/max range [%lld,%lld]", streamValue.value, min, max ) );

                value = streamValue.value;
            }
        }

        const uint8_t * GetWriteBuffer( size_t & bufferSize )
        {
            bufferSize = sizeof( StreamValue ) * m_values.size();
            return reinterpret_cast<uint8_t*>( &m_values[0] );
        }

        void SetReadBuffer( uint8_t * readBuffer, size_t bufferSize )
        {
            int numValues = bufferSize / sizeof( StreamValue );
//            cout << "numValues = " << numValues << endl;
            StreamValue * streamValues = reinterpret_cast<StreamValue*>( readBuffer );
            m_values = vector<StreamValue>( streamValues, streamValues + numValues );
            m_readIndex = 0;
            /*
            cout << "-----------------------------" << endl;
            cout << "read values:" << endl;
            for ( int i = 0; i < m_values.size(); ++i )
            {
                cout << " + value = " << m_values[i].value << " min = " << m_values[i].min << " max = " << m_values[i].max << endl;
            }
            cout << "-----------------------------" << endl;
            */
        }

    private:

        StreamMode m_mode;
        size_t m_readIndex;
        vector<StreamValue> m_values;
    };

    void serialize_object( Stream & stream, Object & object )
    {                        
        object.Serialize( stream );
    }

    template<typename T> void serialize_int( Stream & stream, T & value, int64_t min, int64_t max )
    {                        
        int64_t int64_value = (int64_t) value;
        stream.SerializeValue( int64_value, min, max );
        value = (T) int64_value;
    }

    void serialize_block( Stream & stream, shared_ptr<Block> & block_ptr, int maxBytes )
    { 
        int numBytesMinusOne;

        if ( stream.IsWriting() )
        {
            assert( block_ptr );
            numBytesMinusOne = int( block_ptr->size() ) - 1;
            assert( numBytesMinusOne >= 0 );
            assert( numBytesMinusOne <= maxBytes - 1 );
        }

        serialize_int( stream, numBytesMinusOne, 0, maxBytes - 1 );
        
        const int numBytes = numBytesMinusOne + 1;

        if ( stream.IsReading() )
            block_ptr = make_shared<Block>( numBytes );
        
        Block & block = *block_ptr;

        const int numWords = numBytes / 4;

        if ( stream.IsWriting() )
        {
            for ( int i = 0; i < numWords; ++i )
            {
                uint32_t value =             block[i*4]               |
                                 ( uint32_t( block[i*4+1] ) << 8 )    |
                                 ( uint32_t( block[i*4+2] ) << 16 )   |
                                 ( uint32_t( block[i*4+3] ) << 24 );

                serialize_int( stream, value, 0, 0xFFFFFFFF );
            }
        }
        else
        {
            for ( int i = 0; i < numWords; ++i )
            {
                uint32_t value;

                serialize_int( stream, value, 0, 0xFFFFFFFF );

                block[i*4] = value & 0xFF;
                block[i*4+1] = ( value >> 8 ) & 0xFF;
                block[i*4+2] = ( value >> 16 ) & 0xFF;
                block[i*4+3] = ( value >> 24 ) & 0xFF;
            }
        }

        const int tailIndex = numWords * 4;
        const int tailBytes = numBytes - numWords * 4;

        for ( int i = 0; i < tailBytes; ++i )
            serialize_int( stream, block[tailIndex+i], 0, 255 );
    }
}   //

#endif
