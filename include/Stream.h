/*
    Network Protocol Foundation Library.
    Copyright (c) 2014, The Network Protocol Company, Inc.
*/

#ifndef PROTOCOL_STREAM_H
#define PROTOCOL_STREAM_H

#include "Common.h"
#include "Block.h"
#include "Allocator.h"
#include "BitPacker.h"

namespace protocol
{
    class WriteStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        WriteStream( uint8_t * buffer, int bytes ) : m_writer( buffer, bytes ) {}

        void SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            PROTOCOL_ASSERT( min < max );
            PROTOCOL_ASSERT( value >= min );
            PROTOCOL_ASSERT( value <= max );
            const int bits = bits_required( min, max );
            uint32_t unsigned_value = value - min;
            m_writer.WriteBits( unsigned_value, bits );
        }

        void SerializeBits( uint32_t value, int bits )
        {
            PROTOCOL_ASSERT( bits > 0 );
            PROTOCOL_ASSERT( bits <= 32 );
            m_writer.WriteBits( value, bits );
        }

        void SerializeBytes( const uint8_t * data, int bytes )
        {
            Align();
            m_writer.WriteBytes( data, bytes );
        }

        void Align()
        {
            m_writer.WriteAlign();
        }

        int GetAlignBits() const
        {
            return m_writer.GetAlignBits();
        }

        bool Check( uint32_t magic )
        {
            Align();
            SerializeBits( magic, 32 );
            return true;
        }

        void Flush()
        {
            m_writer.FlushBits();
        }

        const uint8_t * GetData() const
        {
            return m_writer.GetData();
        }

        int GetBytesWritten() const
        {
            return m_writer.GetBytesWritten();
        }

        int GetBitsWritten() const
        {
            return m_writer.GetBitsWritten();
        }

        int GetBitsRemaining() const
        {
            return GetTotalBits() - GetBitsWritten();
        }

        int GetTotalBits() const
        {
            return m_writer.GetTotalBytes() * 8;
        }

        int GetTotalBytes() const
        {
            return m_writer.GetTotalBytes();
        }

        bool IsOverflow() const
        {
            return m_writer.IsOverflow();
        }

    private:

        BitWriter m_writer;
    };

    class ReadStream
    {
    public:

        enum { IsWriting = 0 };
        enum { IsReading = 1 };

        ReadStream( uint8_t * buffer, int bytes ) : m_reader( buffer, bytes ) {}

        void SerializeInteger( int32_t & value, int32_t min, int32_t max )
        {
            PROTOCOL_ASSERT( min < max );
            const int bits = bits_required( min, max );
            uint32_t unsigned_value = m_reader.ReadBits( bits );
            value = (int32_t) unsigned_value + min;
        }

        void SerializeBits( uint32_t & value, int bits )
        {
            PROTOCOL_ASSERT( bits > 0 );
            PROTOCOL_ASSERT( bits <= 32 );
            uint32_t read_value = m_reader.ReadBits( bits );
            value = read_value;
        }

        void SerializeBytes( uint8_t * data, int bytes )
        {
            Align();
            m_reader.ReadBytes( data, bytes );
        }

        void Align()
        {
            m_reader.ReadAlign();
        }

        int GetAlignBits() const
        {
            return m_reader.GetAlignBits();
        }

        bool Check( uint32_t magic )
        {
            Align();
            uint32_t value = 0;
            SerializeBits( value, 32 );
            PROTOCOL_ASSERT( value == magic );
            return value == magic;
        }

        bool IsOverflow() const
        {
            return m_reader.IsOverflow();
        }

    private:

        BitReader m_reader;
    };

    class MeasureStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        MeasureStream( int bytes ) : m_totalBytes( bytes ), m_bitsWritten(0) {}

        void SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            PROTOCOL_ASSERT( min < max );
            PROTOCOL_ASSERT( value >= min );
            PROTOCOL_ASSERT( value <= max );
            const int bits = bits_required( min, max );
            m_bitsWritten += bits;
        }

        void SerializeBits( uint32_t value, int bits )
        {
            PROTOCOL_ASSERT( bits > 0 );
            PROTOCOL_ASSERT( bits <= 32 );
            m_bitsWritten += bits;
        }

        void SerializeBytes( const uint8_t * data, int bytes )
        {
            Align();
            m_bitsWritten += bytes * 8;
        }

        void Align()
        {
            const int alignBits = GetAlignBits();
            m_bitsWritten += alignBits;
        }

        int GetAlignBits() const
        {
            return 7;       // worst case
        }

        bool Check( uint32_t magic )
        {
            Align();
            m_bitsWritten += 32;
            return true;
        }

        int GetBitsWritten() const
        {
            return m_bitsWritten;
        }

        int GetBytesWritten() const
        {
            return m_bitsWritten / 8 + ( m_bitsWritten % 8 ? 1 : 0 );
        }

        int GetTotalBytes() const
        {
            return m_totalBytes;
        }

        int GetTotalBits() const
        {
            return m_totalBytes * 8;
        }

        bool IsOverflow() const
        {
            return m_bitsWritten > m_totalBytes * 8;
        }

    private:

        int m_totalBytes;
        int m_bitsWritten;
    };

    template <typename T> void serialize_object( ReadStream & stream, T & object )
    {                        
        object.SerializeRead( stream );
    }

    template <typename T> void serialize_object( WriteStream & stream, T & object )
    {                        
        object.SerializeWrite( stream );
    }

    template <typename T> void serialize_object( MeasureStream & stream, T & object )
    {                        
        object.SerializeMeasure( stream );
    }
    
    #define serialize_int( stream, value, min, max )            \
        do                                                      \
        {                                                       \
            PROTOCOL_ASSERT( min < max );                                \
            int32_t int32_value;                                \
            if ( Stream::IsWriting )                            \
            {                                                   \
                PROTOCOL_ASSERT( value >= min );                         \
                PROTOCOL_ASSERT( value <= max );                         \
                int32_value = (int32_t) value;                  \
            }                                                   \
            stream.SerializeInteger( int32_value, min, max );   \
            if ( Stream::IsReading )                            \
            {                                                   \
                value = (decltype(value)) int32_value;          \
                PROTOCOL_ASSERT( value >= min );                         \
                PROTOCOL_ASSERT( value <= max );                         \
            }                                                   \
        } while (0)

    #define serialize_bits( stream, value, bits )               \
        do                                                      \
        {                                                       \
            PROTOCOL_ASSERT( bits > 0 );                                 \
            PROTOCOL_ASSERT( bits <= 32 );                               \
            uint32_t uint32_value;                              \
            if ( Stream::IsWriting )                            \
                uint32_value = (uint32_t) value;                \
            stream.SerializeBits( uint32_value, bits );         \
            if ( Stream::IsReading )                            \
                value = (decltype(value)) uint32_value;         \
        } while (0)

    template <typename Stream> void serialize_bool( Stream & stream, bool & value )
    {
        serialize_bits( stream, value, 1 );
    }

    template <typename Stream> void serialize_uint32( Stream & stream, uint32_t & value )
    {
        serialize_bits( stream, value, 32 );
    }

    template <typename Stream> void serialize_uint64( Stream & stream, uint64_t & value )
    {
        uint32_t hi,lo;
        if ( Stream::IsWriting )
        {
            lo = value & 0xFFFFFFFF;
            hi = value >> 32;
        }
        serialize_bits( stream, lo, 32 );
        serialize_bits( stream, hi, 32 );
        if ( Stream::IsReading )
            value = ( uint64_t(hi) << 32 ) | lo;
    }

    template <typename Stream> void serialize_bytes( Stream & stream, uint8_t * data, int bytes )
    {
        stream.SerializeBytes( data, bytes );        
    }

    template <typename Stream> void serialize_block( Stream & stream, Block & block, int maxBytes )
    { 
        stream.Align();

        int numBytes;
        if ( Stream::IsWriting )
        {
            PROTOCOL_ASSERT( block.IsValid() );
            numBytes = (int) block.GetSize();
        }

        serialize_int( stream, numBytes, 1, maxBytes );

        stream.Align();
        
        if ( Stream::IsReading )
        {
            PROTOCOL_ASSERT( numBytes > 0 );
            PROTOCOL_ASSERT( numBytes <= maxBytes );
            Allocator * allocator = block.GetAllocator();
            PROTOCOL_ASSERT( allocator );
            uint8_t * data = (uint8_t*) allocator->Allocate( numBytes );
            block.Connect( *allocator, data, numBytes );
        }
        
        stream.SerializeBytes( block.GetData(), numBytes );
    }

    template <typename Stream, typename T> void serialize_int_relative( Stream & stream, T previous, T & current )
    {
        uint32_t difference;
        if ( Stream::IsWriting )
        {
            PROTOCOL_ASSERT( previous < current );
            difference = current - previous;
            PROTOCOL_ASSERT( difference >= 0 );
        }

        bool oneBit;
        if ( Stream::IsWriting )
            oneBit = difference == 1;
        serialize_bool( stream, oneBit );
        if ( oneBit )
        {
            if ( Stream::IsReading )
                current = previous + 1;
            return;
        }

        bool twoBits;
        if ( Stream::IsWriting )
            twoBits = difference == difference <= 4;
        serialize_bool( stream, twoBits );
        if ( twoBits )
        {
            serialize_int( stream, difference, 1, 4 );
            if ( Stream::IsReading )
                current = previous + difference;
            return;
        }

        bool fourBits;
        if ( Stream::IsWriting )
            fourBits = difference == difference <= 16;
        serialize_bool( stream, fourBits );
        if ( fourBits )
        {
            serialize_int( stream, difference, 1, 16 );
            if ( Stream::IsReading )
                current = previous + difference;
            return;
        }

        bool eightBits;
        if ( Stream::IsWriting )
            eightBits = difference == difference <= 256;
        serialize_bool( stream, eightBits );
        if ( eightBits )
        {
            serialize_int( stream, difference, 1, 256 );
            if ( Stream::IsReading )
                current = previous + difference;
            return;
        }

        bool twelveBits;
        if ( Stream::IsWriting )
            twelveBits = difference <= 4096;
        serialize_bool( stream, twelveBits );
        if ( twelveBits )
        {
            serialize_int( stream, difference, 1, 4096 );
            if ( Stream::IsReading )
                current = previous + difference;
            return;
        }

        bool sixteenBits;
        if ( Stream::IsWriting ) 
            sixteenBits = difference <= 65535;
        serialize_bool( stream, sixteenBits );
        if ( sixteenBits )
        {
            serialize_int( stream, difference, 1, 65536 );
            if ( Stream::IsReading )
                current = previous + difference;
            return;
        }

        uint32_t value = current;
        serialize_uint32( stream, value );
        if ( Stream::IsReading )
            current = (decltype(current)) value;
    }

    template <typename Stream> bool serialize_check( Stream & stream, uint32_t magic )
    {
        return stream.Check( magic );
    }
}  //

#endif
