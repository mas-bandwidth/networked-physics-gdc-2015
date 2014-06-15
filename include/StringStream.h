#ifndef PROTOCOL_STRING_STREAM_H
#define PROTOCOL_STRING_STREAM_H

#include "Array.h"

#include <string.h>
#include <stdio.h>

#if defined(_MSC_VER)

	#include <stdarg.h>
	#include <stdio.h>

	inline int vsnprintf_compat(char* buffer, size_t size, const char* format, va_list args)
	{
		int result = -1;
		if (size > 0)
			result = _vsnprintf_s(buffer, size, _TRUNCATE, format, args);
		if (result == -1)
			return _vscprintf(format, args);
	
		return result;	
	}

	inline int snprintf_compat(char* buffer, size_t size, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		int result = vsnprintf_compat(buffer, size, format, args);
		va_end(args);
		return result;
	}

	#define snprintf snprintf_compat
	#define vsnprintf vsnprintf_compat
	
#endif

namespace protocol
{
	/// Functions for operating on an Array<char> as a stream of characters,
	/// useful for string formatting, etc.
	namespace string_stream
	{
		typedef Array<char> Buffer;

		/// Dumps the item to the stream using a default formatting.
		Buffer & operator<<(Buffer &b, char c);
		Buffer & operator<<(Buffer &b, const char *s);
		Buffer & operator<<(Buffer &b, float f);
		Buffer & operator<<(Buffer &b, int32_t i);
		Buffer & operator<<(Buffer &b, uint32_t i);
		Buffer & operator<<(Buffer &b, uint64_t i);

		/// Uses printf to print formatted data to the stream.
		Buffer & printf(Buffer &b, const char *format, ...);

		/// Pushes the raw data to the stream.
		Buffer & push(Buffer &b, const char *data, uint32_t n);

		/// Pads the stream with spaces until it is aligned at the specified column.
		/// Can be used to column align data. (Assumes each char is 1 space wide,
		/// i.e. does not work with UTF-8 data.)
		Buffer & tab(Buffer &b, uint32_t column);

		/// Adds the specified number of c to the stream.
		Buffer & repeat(Buffer &b, uint32_t count, char c);

		/// Returns the stream as a C-string. There will always be a \0 character
		/// at the end of the returned string. You don't have to explicitly add it
		/// to the buffer.
		const char *c_str(Buffer &b);
	}

	namespace string_stream_internal
	{
		using namespace string_stream;

		template <typename T>
		inline Buffer &printf_small(Buffer &b, const char *fmt, const T &t)
		{
			char s[32];
			snprintf(s, 32, fmt, t);
			return (b << s);
		}
	}

	namespace string_stream
	{
		inline Buffer & operator<<(Buffer &b, char c)
		{
			array::push_back(b, c);
			return b;
		}

		inline Buffer & operator<<(Buffer &b, const char *s)
		{
			return push(b, s, strlen(s));
		}

		inline Buffer & operator<<(Buffer &b, float f)
		{
			return string_stream_internal::printf_small(b, "%g", f);
		}

		inline Buffer & operator<<(Buffer &b, int32_t i)
		{
			return string_stream_internal::printf_small(b, "%d", i);
		}

		inline Buffer & operator<<(Buffer &b, uint32_t i)
		{
			return string_stream_internal::printf_small(b, "%u", i);
		}

		inline Buffer & operator<<(Buffer &b, uint64_t i)
		{
			return string_stream_internal::printf_small(b, "%01llx", i);
		}

		inline Buffer & push(Buffer &b, const char *data, uint32_t n)
		{
			unsigned int end = array::size(b);
			array::resize(b, end + n);
			memcpy(array::begin(b) + end, data, n);
			return b;
		}

		inline const char *c_str(Buffer &b)
		{
			// Ensure there is a \0 at the end of the buffer.
			array::push_back(b, '\0');
			array::pop_back(b);
			return array::begin(b);
		}
	}

	namespace string_stream
	{
		Buffer & printf(Buffer &b, const char *format, ...)
		{
			va_list args;
			
			va_start(args, format);
			int n = vsnprintf( nullptr, 0, format, args );
			va_end(args);

			uint32_t end = array::size(b);
			array::resize(b, end + n + 1);
			
			va_start(args, format);
			vsnprintf(array::begin(b) + end, n + 1, format, args);
			va_end(args);
			
			array::resize(b, end + n);

			return b;
		}

		Buffer & tab(Buffer &b, uint32_t column)
		{
			uint32_t current_column = 0;
			uint32_t i = array::size(b) - 1;
			while (i != 0xffffffffu && b[i] != '\n' && b[i] != '\r') {
				++current_column;
				--i;
			}
			if (current_column < column)
				repeat(b, column - current_column, ' ');
			return b;
		}

		Buffer & repeat(Buffer &b, uint32_t count, char c)
		{
			for (uint32_t i=0; i<count; ++i)
				array::push_back(b, c);
			return b;
		}
	}
}

#endif
