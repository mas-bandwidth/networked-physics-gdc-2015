/*
	Simple Network Library from "Networking for Game Programmers"
	http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#ifndef NET_STREAM_H
#define NET_STREAM_H

#include <assert.h>
#include <string.h>
#include <algorithm>

namespace net
{
	// bitpacker class
	//  + read and write non-8 multiples of bits efficiently
	
	class BitPacker
	{
	public:
		
		enum Mode
		{
			Read,
			Write
		};
		
		BitPacker( Mode mode, void * buffer, int bytes )
		{
			assert( bytes >= 0 );
			this->mode = mode;
			this->buffer = (unsigned char*) buffer;
			this->ptr = (unsigned char*) buffer;
			this->bytes = bytes;
			bit_index = 0;
			if ( mode == Write )
				memset( buffer, 0, bytes );
		}
		
		void WriteBits( unsigned int value, int bits = 32 )
		{
			assert( ptr );
			assert( buffer );
			assert( bits > 0 );
			assert( bits <= 32 );
			assert( mode == Write );
			if ( bits < 32 )
			{
				const unsigned int mask = ( 1 << bits ) - 1;
				value &= mask;
			}
			do
			{
				assert( ptr - buffer < bytes );
				*ptr |= (unsigned char) ( value << bit_index );
				assert( bit_index < 8 );
				const int bits_written = std::min( bits, 8 - bit_index );
				assert( bits_written > 0 );
				assert( bits_written <= 8 );
				bit_index += bits_written;
				if ( bit_index >= 8 )
				{
					ptr++;
					bit_index = 0;
					value >>= bits_written;
				}
				bits -= bits_written;
				assert( bits >= 0 );
				assert( bits <= 32 );
			}
			while ( bits > 0 );
		}
		
 		void ReadBits( unsigned int & value, int bits = 32 )
		{
			assert( ptr );
			assert( buffer );
			assert( bits > 0 );
			assert( bits <= 32 );
			assert( mode == Read );
			int original_bits = bits;
			int value_index = 0;
			value = 0;
			do
			{
				assert( ptr - buffer < bytes );
				assert( bits >= 0 );
				assert( bits <= 32 );
				int bits_to_read = std::min( 8 - bit_index, bits );
				assert( bits_to_read > 0 );
				assert( bits_to_read <= 8 );
				value |= ( *ptr >> bit_index ) << value_index;
				bits -= bits_to_read;
				bit_index += bits_to_read;
				value_index += bits_to_read;
				assert( value_index >= 0 );
				assert( value_index <= 32 );
				if ( bit_index >= 8 )
				{
					ptr++;
					bit_index = 0;
				}
			}
			while ( bits > 0 );
			if ( original_bits < 32 )
			{
				const unsigned int mask = ( 1 << original_bits ) - 1;
				value &= mask;
			}
		}
		
		void * GetData()
		{
			return buffer;
		}
		
		int GetBits() const
		{
			return ( ptr - buffer ) * 8 + bit_index;
		}
		
		int GetBytes() const
		{
			return (int) ( ptr - buffer ) + ( bit_index > 0 ? 1 : 0 );
		}
		
		int BitsRemaining() const
		{
			return bytes * 8 - ( ( ptr - buffer ) * 8 + bit_index );
		}
		
		Mode GetMode() const
		{
			return mode;
		}
		
		bool IsValid() const
		{
			return buffer != NULL;
		}
		
	private:
		
		int bit_index;
		unsigned char * ptr;
		unsigned char * buffer;
		int bytes;
		Mode mode;
	};
	
	// arithmetic coder
	//  + todo: implement arithmetic coder based on jon blow's game developer article
	
	class ArithmeticCoder
	{
	public:
		
		enum Mode
		{
			Read,
			Write
		};
		
		ArithmeticCoder( Mode mode, void * buffer, unsigned int size )
		{
			assert( buffer );
			assert( size >= 0 );
			this->mode = mode;
			this->buffer = (unsigned char*) buffer;
			this->size = size;
		}

		bool WriteInteger( unsigned int value, unsigned int minimum = 0, unsigned int maximum = 0xFFFFFFFF )
		{
			// ...
			return false;
		}
		
		bool ReadInteger( unsigned int value, unsigned int minimum = 0, unsigned int maximum = 0xFFFFFFFF )
		{
			// ...
			return false;
		}
		
	private:
		
		unsigned char * buffer;
		int size;
		Mode mode;
	};
	
	// stream class
	//  + unifies read and write into a serialize operation
	//  + provides attribution of stream for debugging purposes
	
	class Stream
	{
	public:
		
		enum Mode
		{
			Read,
			Write
		};
		
		Stream( Mode mode, void * buffer, int bytes, void * journal_buffer = NULL, int journal_bytes = 0 )
			: bitpacker( mode == Write ? BitPacker::Write : BitPacker::Read, buffer, bytes ), 
			  journal( mode == Write ? BitPacker::Write : BitPacker::Read, journal_buffer, journal_bytes )
		{
		}
		
		bool SerializeBoolean( bool & value )
		{
			unsigned int tmp = (unsigned int) value;
			bool result = SerializeBits( tmp, 1 );
			value = (bool) tmp;
			return result;
		}

		bool SerializeByte( char & value, char min = -127, char max = +128 )
		{
			// wtf: why do I have to do this!?
			unsigned int tmp = (unsigned int) ( value + 127 );
			bool result = SerializeInteger( tmp, (unsigned int ) ( min + 127 ), ( max + 127 ) );
			value = ( (char) tmp ) - 127;
			return result;
		}
		
		bool SerializeByte( signed char & value, signed char min = -127, signed char max = +128 )
		{
			unsigned int tmp = (unsigned int) ( value + 127 );
			bool result = SerializeInteger( tmp, (unsigned int ) ( min + 127 ), ( max + 127 ) );
			value = ( (signed char) tmp ) - 127;
			return result;
		}

		bool SerializeByte( unsigned char & value, unsigned char min = 0, unsigned char max = 0xFF )
		{
			unsigned int tmp = (unsigned int) value;
			bool result = SerializeInteger( tmp, min, max );
			value = (unsigned char) tmp;
			return result;
		}

		bool SerializeShort( signed short & value, signed short min = -32767, signed short max = +32768 )
		{
			unsigned int tmp = (unsigned int) ( value + 32767 );
			bool result = SerializeInteger( tmp, (unsigned int ) ( min + 32767 ), ( max + 32767 ) );
			value = ( (signed short) tmp ) - 32767;
			return result;
		}

		bool SerializeShort( unsigned short & value, unsigned short min = 0, unsigned short max = 0xFFFF )
		{
			unsigned int tmp = (unsigned int) value;
			bool result = SerializeInteger( tmp, min, max );
			value = (unsigned short) tmp;
			return result;
		}
		
		bool SerializeInteger( signed int & value, signed int min = -2147483646, signed int max = +2147483647 )
		{
			unsigned int tmp = (unsigned int) ( value + 2147483646 );
			bool result = SerializeInteger( tmp, (unsigned int ) ( min + 2147483646 ), ( max + 2147483646 ) );
			value = ( (signed int) tmp ) - 2147483646;
			return result;
		}

		bool SerializeInteger( unsigned int & value, unsigned int min = 0, unsigned int max = 0xFFFFFFFF )
		{
			assert( min < max );
			if ( IsWriting() )
			{
				assert( value >= min );
				assert( value <= max );
			}
			const int bits_required = BitsRequired( min, max );
			unsigned int bits = value - min;
			bool result = SerializeBits( bits, bits_required );
			if ( IsReading() )
			{
				value = bits + min;
				assert( value >= min );
				assert( value <= max );
			}
			return result;
		}
		
		bool SerializeFloat( float & value )
		{
			union FloatInt
			{
				unsigned int i;
				float f;
			};
			if ( IsReading() )
			{
				FloatInt floatInt;
				if ( !SerializeBits( floatInt.i, 32 ) )
					return false;
				value = floatInt.f;
				return true;
			}
			else
			{
				FloatInt floatInt;
				floatInt.f = value;
				return SerializeBits( floatInt.i, 32 );
			}
		}

		bool SerializeBits( unsigned int & value, int bits )
		{
			assert( bits >= 1 );
			assert( bits <= 32 );
			if ( bitpacker.BitsRemaining() < bits )
				return false;
			if ( journal.IsValid() )
			{
				unsigned int token = 2 + bits;		// note: 0 = end, 1 = checkpoint, [2,34] = n - 2 bits written
				if ( IsWriting() )
				{
					journal.WriteBits( token, 6 );
				}
				else
				{
					journal.ReadBits( token, 6 );
					int bits_written = token - 2;
					if ( bits != bits_written )
					{
						printf( "desync read/write: attempting to read %d bits when %d bits were written\n", bits, bits_written );
						return false;
					}
				}
			}
			if ( IsReading() )
				bitpacker.ReadBits( value, bits );
			else
				bitpacker.WriteBits( value, bits );
			return true;
		}
		
		bool Checkpoint()
		{
			if ( journal.IsValid() )
			{
				unsigned int token = 1;		// note: 0 = end, 1 = checkpoint, [2,34] = n - 2 bits written
				if ( IsWriting() )
				{
					journal.WriteBits( token, 6 );
				}
				else
				{
					journal.ReadBits( token, 6 );
					if ( token != 1 )
					{
						printf( "desync read/write: checkpoint not present in journal\n" );
						return false;
					}
				}
			}
			unsigned int magic = 0x12345678;
			unsigned int value = magic;
			if ( bitpacker.BitsRemaining() < 32 )
			{
				printf( "not enough bits remaining for checkpoint\n" );
				return false;
			}
			if ( IsWriting() )
				bitpacker.WriteBits( value, 32 );
			else
				bitpacker.ReadBits( value, 32 );
			if ( value != magic )
			{
				printf( "checkpoint failed!\n" );
				return false;
			}
			return true;
		}
		
		bool IsReading() const
		{
			return bitpacker.GetMode() == BitPacker::Read;
		}
		
		bool IsWriting() const
		{
			return bitpacker.GetMode() == BitPacker::Write;
		}
		
		int GetBitsProcessed() const
		{
			return bitpacker.GetBits();
		}
		
		int GetBitsRemaining() const
		{
			return bitpacker.BitsRemaining();
		}
		
		static int BitsRequired( unsigned int minimum, unsigned int maximum )
		{
			assert( maximum > minimum );
			assert( maximum >= 1 );
			if ( maximum - minimum >= 0x7FFFFFF )
				return 32;
			return BitsRequired( maximum - minimum + 1 );
		}
		
		static int BitsRequired( unsigned int distinctValues )
		{
			assert( distinctValues > 1 );
			unsigned int maximumValue = distinctValues - 1;
			for ( int index = 0; index < 32; ++index )
			{
				if ( ( maximumValue & ~1 ) == 0 )
					return index + 1;
				maximumValue >>= 1;
			}
			return 32;
		}
		
		int GetDataBytes() const
		{
			return bitpacker.GetBytes();
		}
		
		int GetJournalBytes() const
		{
			return journal.GetBytes();
		}
		
		void DumpJournal()
		{
			if ( journal.IsValid() )
			{
				printf( "-----------------------------\n" );
				printf( "dump journal:\n" );
				BitPacker reader( BitPacker::Read, journal.GetData(), journal.GetBytes() );
				while ( reader.BitsRemaining() > 6 )
				{
					unsigned int token = 0;
					reader.ReadBits( token, 6 );
					if ( token == 0 )
						break;
					if ( token == 1 )
						printf( " (checkpoint)\n" );
					else
						printf( " + %d bits\n", token - 2 );
				}
				printf( "-----------------------------\n" );
			}
			else
				printf( "no journal exists!\n" );
		}
		
	private:
		
		BitPacker bitpacker;
		BitPacker journal;
	};
}

#endif
