// Protocol Library - Copyright (c) 2008-2015, Glenn Fiedler

#ifndef PROTOCOL_STREAM_H
#define PROTOCOL_STREAM_H

#include <math.h>
#include "core/Core.h"
#include "core/Allocator.h"
#include "protocol/ProtocolConstants.h"
#include "protocol/ProtocolEnums.h"
#include "protocol/Block.h"
#include "protocol/BitPacker.h"

namespace protocol
{
    class WriteStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        WriteStream( uint8_t * buffer, int bytes ) : m_writer( buffer, bytes ), m_context( nullptr ), m_aborted( false ) {}

        void SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            CORE_ASSERT( min < max );
            CORE_ASSERT( value >= min );
            CORE_ASSERT( value <= max );
            const int bits = core::bits_required( min, max );
            uint32_t unsigned_value = value - min;
            m_writer.WriteBits( unsigned_value, bits );
        }

        void SerializeBits( uint32_t value, int bits )
        {
            CORE_ASSERT( bits > 0 );
            CORE_ASSERT( bits <= 32 );
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

        int GetBytesProcessed() const
        {
            return m_writer.GetBytesWritten();
        }

        int GetBitsProcessed() const
        {
            return m_writer.GetBitsWritten();
        }

        int GetBitsRemaining() const
        {
            return GetTotalBits() - GetBitsProcessed();
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

        void SetContext( const void ** context )
        {
            m_context = context;
        }

        const void * GetContext( int index ) const
        {
            CORE_ASSERT( index >= 0 );
            CORE_ASSERT( index < protocol::MaxContexts );
            return m_context ? m_context[index] : nullptr;
        }

        void Abort()
        {
            m_aborted = true;
        }

        bool Aborted() const
        {
            return m_aborted;
        }

    private:

        BitWriter m_writer;
        const void ** m_context;
        bool m_aborted;
    };

    class ReadStream
    {
    public:

        enum { IsWriting = 0 };
        enum { IsReading = 1 };

        ReadStream( uint8_t * buffer, int bytes ) : m_bitsRead(0), m_reader( buffer, bytes ), m_context( nullptr ), m_aborted( false ) {}

        void SerializeInteger( int32_t & value, int32_t min, int32_t max )
        {
            CORE_ASSERT( min < max );
            const int bits = core::bits_required( min, max );
            uint32_t unsigned_value = m_reader.ReadBits( bits );
            value = (int32_t) unsigned_value + min;
            m_bitsRead += bits;
        }

        void SerializeBits( uint32_t & value, int bits )
        {
            CORE_ASSERT( bits > 0 );
            CORE_ASSERT( bits <= 32 );
            uint32_t read_value = m_reader.ReadBits( bits );
            value = read_value;
            m_bitsRead += bits;
        }

        void SerializeBytes( uint8_t * data, int bytes )
        {
            Align();
            m_reader.ReadBytes( data, bytes );
            m_bitsRead += bytes * 8;
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
            CORE_ASSERT( value == magic );
            return value == magic;
        }

        int GetBitsProcessed() const
        {
            return m_bitsRead;
        }

        int GetBytesProcessed() const
        {
            return m_bitsRead / 8 + ( m_bitsRead % 8 ? 1 : 0 );
        }

        bool IsOverflow() const
        {
            return m_reader.IsOverflow();
        }

        void SetContext( const void ** context )
        {
            m_context = context;
        }

        const void * GetContext( int index ) const
        {
            CORE_ASSERT( index >= 0 );
            CORE_ASSERT( index < MaxContexts );
            return m_context ? m_context[index] : nullptr;
        }

        void Abort()
        {
            m_aborted = true;
        }

        bool Aborted() const
        {
            return m_aborted;
        }

        int GetBytesRead() const
        {
            return m_reader.GetBytesRead();
        }

    private:

        int m_bitsRead;
        BitReader m_reader;
        const void ** m_context;
        bool m_aborted;
    };

    class MeasureStream
    {
    public:

        enum { IsWriting = 1 };
        enum { IsReading = 0 };

        MeasureStream( int bytes ) : m_totalBytes( bytes ), m_bitsWritten(0), m_context( nullptr ), m_aborted( false ) {}

        void SerializeInteger( int32_t value, int32_t min, int32_t max )
        {
            CORE_ASSERT( min < max );
            CORE_ASSERT( value >= min );
            CORE_ASSERT( value <= max );
            const int bits = core::bits_required( min, max );
            m_bitsWritten += bits;
        }

        void SerializeBits( uint32_t value, int bits )
        {
            CORE_ASSERT( bits > 0 );
            CORE_ASSERT( bits <= 32 );
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

        int GetBitsProcessed() const
        {
            return m_bitsWritten;
        }

        int GetBytesProcessed() const
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

        void SetContext( const void ** context )
        {
            m_context = context;
        }

        const void * GetContext( int index ) const
        {
            CORE_ASSERT( index >= 0 );
            CORE_ASSERT( index < MaxContexts );
            return m_context ? m_context[index] : nullptr;
        }

        void Abort()
        {
            m_aborted = true;
        }

        bool Aborted() const
        {
            return m_aborted;
        }

    private:

        int m_totalBytes;
        int m_bitsWritten;
        const void ** m_context;
        bool m_aborted;
    };
}

template <typename T> void serialize_object( protocol::ReadStream & stream, T & object )
{                        
    object.SerializeRead( stream );
}

template <typename T> void serialize_object( protocol::WriteStream & stream, T & object )
{                        
    object.SerializeWrite( stream );
}

template <typename T> void serialize_object( protocol::MeasureStream & stream, T & object )
{                        
    object.SerializeMeasure( stream );
}

#define serialize_int( stream, value, min, max )            \
    do                                                      \
    {                                                       \
        CORE_ASSERT( min < max );                           \
        int32_t int32_value;                                \
        if ( Stream::IsWriting )                            \
        {                                                   \
            CORE_ASSERT( value >= min );                    \
            CORE_ASSERT( value <= max );                    \
            int32_value = (int32_t) value;                  \
        }                                                   \
        stream.SerializeInteger( int32_value, min, max );   \
        if ( Stream::IsReading )                            \
        {                                                   \
            value = (decltype(value)) int32_value;          \
            CORE_ASSERT( value >= min );                    \
            CORE_ASSERT( value <= max );                    \
        }                                                   \
    } while (0)

#define serialize_bits( stream, value, bits )               \
    do                                                      \
    {                                                       \
        CORE_ASSERT( bits > 0 );                            \
        CORE_ASSERT( bits <= 32 );                          \
        uint32_t uint32_value;                              \
        if ( Stream::IsWriting )                            \
            uint32_value = (uint32_t) value;                \
        stream.SerializeBits( uint32_value, bits );         \
        if ( Stream::IsReading )                            \
            value = (decltype(value)) uint32_value;         \
    } while (0)

#define serialize_bool( stream, value ) serialize_bits( stream, value, 1 )

template <typename Stream> void serialize_uint16( Stream & stream, uint16_t & value )
{
    serialize_bits( stream, value, 16 );
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

template <typename Stream> void serialize_int16( Stream & stream, int16_t & value )
{
    serialize_bits( stream, value, 16 );
}

template <typename Stream> void serialize_int32( Stream & stream, int32_t & value )
{
    serialize_bits( stream, value, 32 );
}

template <typename Stream> void serialize_int64( Stream & stream, int64_t & value )
{
    uint32_t hi,lo;
    if ( Stream::IsWriting )
    {
        lo = uint64_t(value) & 0xFFFFFFFF;
        hi = uint64_t(value) >> 32;
    }
    serialize_bits( stream, lo, 32 );
    serialize_bits( stream, hi, 32 );
    if ( Stream::IsReading )
        value = ( int64_t(hi) << 32 ) | lo;
}

template <typename Stream> void serialize_float( Stream & stream, float & value )
{
    union FloatInt
    {
        float float_value;
        uint32_t int_value;
    };

    FloatInt tmp;
    if ( Stream::IsWriting )
        tmp.float_value = value;

    serialize_uint32( stream, tmp.int_value );

    if ( Stream::IsReading )
        value = tmp.float_value;
}

template <typename Stream> inline void internal_serialize_float( Stream & stream, float & value, float min, float max, float res )
{
    const float delta = max - min;
    const float values = delta / res;
    const uint32_t maxIntegerValue = (uint32_t) ceil( values );
    const int bits = core::bits_required( 0, maxIntegerValue );
    
    uint32_t integerValue = 0;
    
    if ( Stream::IsWriting )
    {
        float normalizedValue = core::clamp( ( value - min ) / delta, 0.0f, 1.0f );
        integerValue = (uint32_t) floor( normalizedValue * maxIntegerValue + 0.5f );
    }
    
    stream.SerializeBits( integerValue, bits );

    if ( Stream::IsReading )
    {
        const float normalizedValue = integerValue / float( maxIntegerValue );
        value = normalizedValue * delta + min;
    }
}

#define serialize_compressed_float( stream, value, min, max, res )                                        \
do                                                                                                        \
{                                                                                                         \
    internal_serialize_float( stream, value, min, max, res );                                             \
}                                                                                                         \
while(0)


template <typename Stream> void serialize_double( Stream & stream, double & value )
{
    union DoubleInt
    {
        double double_value;
        uint64_t int_value;
    };

    DoubleInt tmp;
    if ( Stream::IsWriting )
        tmp.double_value = value;

    serialize_uint64( stream, tmp.int_value );

    if ( Stream::IsReading )
        value = tmp.double_value;
}

template <typename Stream> void serialize_bytes( Stream & stream, uint8_t * data, int bytes )
{
    stream.SerializeBytes( data, bytes );        
}

template <typename Stream> void serialize_string( Stream & stream, char * string, int buffer_size )
{
    uint32_t length;
    if ( Stream::IsWriting )
        length = strlen( string );
    stream.Align();
    stream.SerializeBits( length, 32 );
    CORE_ASSERT( length < buffer_size - 1 );
    stream.SerializeBytes( (uint8_t*)string, length );
    if ( Stream::IsReading )
        string[length] = '\0';
}

template <typename Stream> void serialize_block( Stream & stream, protocol::Block & block, int maxBytes )
{ 
    stream.Align();

    int numBytes;
    if ( Stream::IsWriting )
    {
        CORE_ASSERT( block.IsValid() );
        numBytes = (int) block.GetSize();
    }

    serialize_int( stream, numBytes, 1, maxBytes );

    stream.Align();
    
    if ( Stream::IsReading )
    {
        CORE_ASSERT( numBytes > 0 );
        CORE_ASSERT( numBytes <= maxBytes );
        core::Allocator * allocator = block.GetAllocator();
        CORE_ASSERT( allocator );
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
        CORE_ASSERT( previous < current );
        difference = current - previous;
        CORE_ASSERT( difference >= 0 );
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

#endif
