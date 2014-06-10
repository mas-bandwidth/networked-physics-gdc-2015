/*
    Network Protocol Library
    Copyright (c) 2013-2014 Glenn Fiedler <glenn.fiedler@gmail.com>
*/

#ifndef PROTOCOL_STREAM_H
#define PROTOCOL_STREAM_H

#include "Common.h"
#include "BitPacker.h"

namespace protocol
{
    enum StreamMode
    {
        STREAM_Read,
        STREAM_Write
    };

    class Stream
    {
    public:

        Stream( StreamMode mode, uint8_t * buffer, int bytes )
            : m_mode( mode ),
              m_writer( buffer, bytes ), 
              m_reader( buffer, bytes )
        {
            // ...
        }

        bool IsReading() const 
        {
            return m_mode == STREAM_Read;
        }

        bool IsWriting() const
        {
            return m_mode == STREAM_Write;
        }

        void SerializeInteger( int32_t & value, int32_t min, int32_t max )
        {
            assert( min < max );

            const int bits = bits_required( min, max );

            if ( IsWriting() )
            {
                uint32_t unsigned_value = value - min;
                m_writer.WriteBits( unsigned_value, bits );
            }
            else
            {
                uint32_t unsigned_value = m_reader.ReadBits( bits );
                if ( m_reader.InOverflow() )
                    throw runtime_error( "read stream overflow" );
                value = (int32_t) unsigned_value + min;
            }
        }

        void SerializeBits( uint32_t & value, int bits )
        {
            assert( bits > 0 );
            assert( bits <= 32 );

            if ( IsWriting() )
            {
                m_writer.WriteBits( value, bits );
            }
            else
            {
                uint32_t read_value = m_reader.ReadBits( bits );
                if ( m_reader.InOverflow() )
                    throw runtime_error( "read stream overflow" );
                value = read_value;
            }
        }

        void SerializeBytes( uint8_t * data, int bytes )
        {
            Align();
            if ( IsWriting() )
                m_writer.WriteBytes( data, bytes );
            else
                m_reader.ReadBytes( data, bytes );
        }

        void Align()
        {
            if ( IsWriting() )
                m_writer.WriteAlign();
            else
                m_reader.ReadAlign();
        }

        int GetAlignBits() const
        {
            return IsWriting() ? m_writer.GetAlignBits() : m_reader.GetAlignBits();
        }

        void Check( uint32_t magic )
        {
            uint32_t value = 0;
            if ( IsWriting() )
                value = magic;
            SerializeBits( value, 32 );
            assert( value == magic );
        }

        void Flush()
        {
            if ( IsWriting() )
                m_writer.FlushBits();
        }

        const uint8_t * GetData() const
        {
            return m_writer.GetData();          // note: same data shared between reader and writer
        }

        int GetBits() const
        {
            if ( IsWriting() )
                return m_writer.GetBitsWritten();
            else
                return 0;
        }

        int GetBytes() const
        {
            if ( IsWriting() )
                return m_writer.GetBytes();
            else
                return 0;
        }

    private:

        StreamMode m_mode;
        BitWriter m_writer;
        BitReader m_reader;
    };

    void serialize_object( Stream & stream, Object & object )
    {                        
        object.Serialize( stream );
    }

    #define serialize_int( stream, value, min, max )            \
        do                                                      \
        {                                                       \
            int32_t int32_value = (int32_t) value;              \
            stream.SerializeInteger( int32_value, min, max );   \
            value = (decltype(value)) int32_value;              \
        } while (0)

    #define serialize_bits( stream, value, bits )               \
        do                                                      \
        {                                                       \
            assert( bits > 0 );                                 \
            assert( bits <= 32 );                               \
            uint32_t uint32_value = (uint32_t) value;           \
            stream.SerializeBits( uint32_value, bits );         \
            value = (decltype(value)) uint32_value;             \
        } while (0)

    void serialize_bool( Stream & stream, bool & value )
    {
        serialize_bits( stream, value, 1 );
    }

    void serialize_uint32( Stream & stream, uint32_t & value )
    {
        serialize_bits( stream, value, 32 );
    }

    void serialize_uint64( Stream & stream, uint64_t & value )
    {
        uint32_t hi = 0;
        uint32_t lo = 0;
        if ( stream.IsWriting() )
        {
            lo = value & 0xFFFFFFFF;
            hi = value >> 32;
        }
        serialize_bits( stream, lo, 32 );
        serialize_bits( stream, hi, 32 );
        if ( stream.IsReading() )
            value = ( uint64_t(hi) << 32 ) | lo;
    }

    void serialize_block( Stream & stream, shared_ptr<Block> & block_ptr, int maxBytes )
    { 
        int numBytes = 0;

        if ( stream.IsWriting() )
        {
            assert( block_ptr );
            numBytes = (int) block_ptr->size();
        }

        stream.Align();

        serialize_int( stream, numBytes, 1, maxBytes - 1 );

        stream.Align();
        
        if ( stream.IsReading() )
            block_ptr = make_shared<Block>( numBytes );
        
        Block & block = *block_ptr;

        stream.SerializeBytes( &block[0], numBytes );
    }
}  //

#endif
