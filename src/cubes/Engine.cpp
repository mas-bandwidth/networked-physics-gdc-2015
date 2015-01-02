/*
	Networked Physics Demo
	Copyright © 2008-2015 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#include "Engine.h"

namespace cubes
{
	// -------------------------------------------------------------
	
	// helper functions for compression
	
 	void CompressPosition( const math::Vector & position, uint64_t & compressed_position )
	{
		float x = position.x + 512.0f;
		float y = position.y + 512.0f;
		float z = position.z + 512.0f;

		uint64_t integer_x = math::floor( x * 1024 + 0.5f );
		uint64_t integer_y = math::floor( y * 1024 + 0.5f );
		uint64_t integer_z = math::floor( z * 1024 + 0.5f );

		integer_x &= ( 1 << 20 ) - 1;
		integer_y &= ( 1 << 20 ) - 1;
		integer_z &= ( 1 << 20 ) - 1;

		compressed_position = ( integer_x << 40 ) | ( integer_y << 20 ) | integer_z;
	}

	void DecompressPosition( uint64_t compressed_position, math::Vector & position )
	{
		uint64_t integer_x = ( compressed_position >> 40 ) & ( (1<<20) - 1 );
		uint64_t integer_y = ( compressed_position >> 20 ) & ( (1<<20) - 1 ); 
		uint64_t integer_z = ( compressed_position       ) & ( (1<<20) - 1 );

		position.x = integer_x / 1024.0f - 512.0f;
		position.y = integer_y / 1024.0f - 512.0f;
		position.z = integer_z / 1024.0f - 512.0f;
	}

 	void CompressOrientation( const math::Quaternion & orientation, uint32_t & compressed_orientation )
	{
 		uint32_t largest = 0;
		float a,b,c;
		a = 0;
		b = 0;
		c = 0;

		const float w = orientation.w;
		const float x = orientation.x;
		const float y = orientation.y;
		const float z = orientation.z;

		#ifdef DEBUG
		const float epsilon = 0.0001f;
		const float length_squared = w*w + x*x + y*y + z*z;
		assert( length_squared >= 1.0f - epsilon && length_squared <= 1.0f + epsilon );
		#endif

		const float abs_w = math::abs( w );
		const float abs_x = math::abs( x );
		const float abs_y = math::abs( y );
		const float abs_z = math::abs( z );

		float largest_value = abs_x;

		if ( abs_y > largest_value )
		{
			largest = 1;
			largest_value = abs_y;
		}

		if ( abs_z > largest_value )
		{
			largest = 2;
			largest_value = abs_z;
		}

		if ( abs_w > largest_value )
		{
			largest = 3;
			largest_value = abs_w;
		}

		switch ( largest )
		{
			case 0:
				if ( x >= 0 )
				{
					a = y;
					b = z;
					c = w;
				}
				else
				{
					a = -y;
					b = -z;
					c = -w;
				}
				break;

			case 1:
				if ( y >= 0 )
				{
					a = x;
					b = z;
					c = w;
				}
				else
				{
					a = -x;
					b = -z;
					c = -w;
				}
				break;

			case 2:
				if ( z >= 0 )
				{
					a = x;
					b = y;
					c = w;
				}
				else
				{
					a = -x;
					b = -y;
					c = -w;
				}
				break;

			case 3:
				if ( w >= 0 )
				{
					a = x;
					b = y;
					c = z;
				}
				else
				{
					a = -x;
					b = -y;
					c = -z;
				}
				break;

			default:
				assert( false );
		}

//		printf( "float: a = %f, b = %f, c = %f\n", a, b, c );

		const float minimum = - 1.0f / 1.414214f;		// note: 1.0f / sqrt(2)
		const float maximum = + 1.0f / 1.414214f;

		const float normal_a = ( a - minimum ) / ( maximum - minimum ); 
		const float normal_b = ( b - minimum ) / ( maximum - minimum );
		const float normal_c = ( c - minimum ) / ( maximum - minimum );

 		uint32_t integer_a = math::floor( normal_a * 1024.0f + 0.5f );
 		uint32_t integer_b = math::floor( normal_b * 1024.0f + 0.5f );
 		uint32_t integer_c = math::floor( normal_c * 1024.0f + 0.5f );

//		printf( "integer: a = %d, b = %d, c = %d, largest = %d\n", 
//			integer_a, integer_b, integer_c, largest );

		compressed_orientation = ( largest << 30 ) | ( integer_a << 20 ) | ( integer_b << 10 ) | integer_c;
	}

	void DecompressOrientation( uint32_t compressed_orientation, math::Quaternion & orientation )
	{
		uint32_t largest = compressed_orientation >> 30;
		uint32_t integer_a = ( compressed_orientation >> 20 ) & ( (1<<10) - 1 );
		uint32_t integer_b = ( compressed_orientation >> 10 ) & ( (1<<10) - 1 );
		uint32_t integer_c = ( compressed_orientation ) & ( (1<<10) - 1 );

//		printf( "---------\n" );

//		printf( "integer: a = %d, b = %d, c = %d, largest = %d\n", 
//			integer_a, integer_b, integer_c, largest );

		const float minimum = - 1.0f / 1.414214f;		// note: 1.0f / sqrt(2)
		const float maximum = + 1.0f / 1.414214f;

		const float a = integer_a / 1024.0f * ( maximum - minimum ) + minimum;
		const float b = integer_b / 1024.0f * ( maximum - minimum ) + minimum;
		const float c = integer_c / 1024.0f * ( maximum - minimum ) + minimum;

//		printf( "float: a = %f, b = %f, c = %f\n", a, b, c );

		switch ( largest )
		{
			case 0:
			{
				// (?) y z w

				orientation.y = a;
				orientation.z = b;
				orientation.w = c;
				orientation.x = math::sqrt( 1 - orientation.y*orientation.y 
				                              - orientation.z*orientation.z 
				                              - orientation.w*orientation.w );
			}
			break;

			case 1:
			{
				// x (?) z w

				orientation.x = a;
				orientation.z = b;
				orientation.w = c;
				orientation.y = math::sqrt( 1 - orientation.x*orientation.x 
				                              - orientation.z*orientation.z 
				                              - orientation.w*orientation.w );
			}
			break;

			case 2:
			{
				// x y (?) w

				orientation.x = a;
				orientation.y = b;
				orientation.w = c;
				orientation.z = math::sqrt( 1 - orientation.x*orientation.x 
				                              - orientation.y*orientation.y 
				                              - orientation.w*orientation.w );
			}
			break;

			case 3:
			{
				// x y z (?)

				orientation.x = a;
				orientation.y = b;
				orientation.z = c;
				orientation.w = math::sqrt( 1 - orientation.x*orientation.x 
				                              - orientation.y*orientation.y 
				                              - orientation.z*orientation.z );
			}
			break;

			default:
			{
				assert( false );
				orientation.w = 1.0f;
				orientation.x = 0.0f;
				orientation.y = 0.0f;
				orientation.z = 0.0f;
			}
		}

		orientation.normalize();
	}

	void QuantizeVector( const math::Vector & vector, int32_t & x, int32_t & y, int32_t & z, float res )
	{
 		x = math::floor( vector.x * res + 0.5f );
 		y = math::floor( vector.y * res + 0.5f );
 		z = math::floor( vector.z * res + 0.5f );
	}
	
	void DequantizeVector( const int32_t & x, const int32_t & y, const int32_t & z, math::Vector & vector, float res )
	{
		vector.x = x / res;
		vector.y = y / res;
		vector.z = z / res;
	}
}
