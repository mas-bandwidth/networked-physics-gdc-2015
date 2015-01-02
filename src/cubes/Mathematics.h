/*
	Networked Physics Demo
	Copyright © 2008-2015 Glenn Fiedler
	http://www.gafferongames.com/networking-for-game-programmers
*/

#ifndef MATHEMATICS_H
#define MATHEMATICS_H

#include "Config.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <assert.h>

namespace math
{
	const float pi = 3.1415926f;                            // pi at a reasonable precision for single precision floating point
	const float epsilon = 0.00001f;                         // floating point epsilon for single precision
	const float epsilonSquared = epsilon * epsilon;         // epsilon value squared

	// test for floating point equality within [-epsilon,+epsilon]

	inline bool equal(float a, float b)
	{
		const float d = a - b;
		if ( d < epsilon && d > -epsilon ) 
			return true;
		else 
			return false;
	}

	// clamp in range
	
	template <typename T> T min( T a, T b )
	{
		return a < b ? a : b;
	}
	
	inline unsigned int clamp( unsigned int value, unsigned int min, unsigned int max )
	{
		assert( max > min );
		if ( value < min )
			return min;
		else if ( value > max )
			return max;
		else
			return value;
	}
	
	inline int clamp( int value, int min, int max )
	{
		assert( max > min );
		if ( value < min )
			return min;
		else if ( value > max )
			return max;
		else
			return value;
	}
	
	inline float clamp( float value, float min, float max )
	{
		assert( max > min );
		if ( value < min )
			return min;
		else if ( value > max )
			return max;
		else
			return value;
	}

	// determine the minimum floating point value

	inline float minimum(float a, float b)
	{
		return ( a < b) ? a : b;
	}

	// determine the maximum floating point value

	inline float maximum(float a, float b)
	{
		return ( a > b ) ? a : b;
	}

	// calculate the square root of a floating point number.

	inline float sqrt(float value)
	{
		assert(value>=0);
		return sqrtf(value);
	}

	// calculate the sine of a floating point angle in radians.

	inline float sin(float radians)
	{
		return sinf(radians);
	}

	// calculate the cosine of a floating point angle in radians.

	inline float cos(float radians)
	{
		return cosf(radians);
	}

	// calculate the tangent of a floating point angle in radians.

	inline float tan(float radians)
	{
		return tanf(radians);
	}

	// calculate the arcsine of a floating point value. result is in radians.

	inline float asin(float value)
	{
		return asinf(value);
	}

	// calculate the arccosine of a floating point value. result is in radians.

	inline float acos(float value)
	{
		return acosf(value);
	}

	// calculate the arctangent of a floating point value y/x. result is in radians.

	inline float atan2(float y, float x)
	{
		return atan2f(y,x);
	}

	// calculate the floor of a floating point value.
	// the floor is the nearest integer strictly less than or equal to the floating point number.

	inline float floor(float value)
	{
		return floorf(value);
	}

	// calculate the ceiling of a floating point value.
	// the ceil is the nearest integer strictly greater than or equal to the floating point number.

	inline float ceiling(float value)
	{                     
		return ceilf(value);
	}

	// float/integer union to avoid aliasing float*/int*
	struct FloatInteger
	{
		unsigned int intValue;
		float floatValue;
	};

	// quickly determine the sign of a floating point number.

	inline unsigned int sign( float v )
	{	
		FloatInteger tmp;
		tmp.floatValue = v;
		return tmp.intValue & 0x80000000;
	}

	// fast floating point absolute value.

	inline float abs( float v )
	{
		/*
		FloatInteger tmp;
		tmp.floatValue = v;
		tmp.intValue &= 0x7fffffff;
		return tmp.floatValue;
		*/
		return fabsf( v );
	}

	// random numbers
	
	inline void init_random( int seed )
	{
		srand( seed );
	}

	inline int random( int maximum )
	{
		assert( maximum > 0 );
		int randomNumber = 0;
		randomNumber = rand() % maximum;
		return randomNumber;
	}

	inline float random_float( float min, float max )
	{
		assert( max > min );
		return random( 1000000 ) / 1000000.f * ( max - min ) + min;
	}

	inline bool chance( float probability )
	{
		assert( probability >= 0.0f );
		assert( probability <= 1.0f );
		const int percent = (int) ( probability * 100.0f );
		return random(100) <= percent;
	}

	// vector class

	class Vector
	{
	public:

	    // default constructor.

	    Vector() {}

	    // construct vector from x,y,z components.

	    Vector(float _x, float _y, float _z)
			: x( _x ), y( _y ), z( _z ) {}

	    // set vector to zero.

	    void zero()
	    {
	        x = 0;
	        y = 0;
	        z = 0;
	    }

	    // negate vector.

		void negate()
		{
			x = -x;
			y = -y;
			z = -z;
		}

	    // add another vector to this vector.

	    void add(const Vector &vector)
	    {
	        x += vector.x;
	        y += vector.y;
	        z += vector.z;
	    }

	    // subtract another vector from this vector.

	    void subtract(const Vector &vector)
	    {
	        x -= vector.x;
	        y -= vector.y;
	        z -= vector.z;
	    }

	    // multiply this vector by a scalar.

	    void multiply(float scalar)
	    {
	        x *= scalar;
	        y *= scalar;
	        z *= scalar;
	    }

	    // divide this vector by a scalar.

		void divide(float scalar)
		{
			assert(scalar!=0);
			const float inv = 1.0f / scalar;
			x *= inv;
			y *= inv;
			z *= inv;
		}

	    // calculate dot product of this vector with another vector.

	    float dot(const Vector &vector) const
	    {
	        return x * vector.x + y * vector.y + z * vector.z;
	    }

	    // calculate cross product of this vector with another vector.

		Vector cross(const Vector &vector) const
	    {
	        return Vector(y * vector.z - z * vector.y, z * vector.x - x * vector.z, x * vector.y - y * vector.x);
	    }

	    // calculate cross product of this vector with another vector, store result in parameter.

		void cross(const Vector &vector, Vector &result) const
	    {
	        result.x = y * vector.z - z * vector.y;
	        result.y = z * vector.x - x * vector.z;
	        result.z = x * vector.y - y * vector.x;
	    }

	    // calculate length of vector squared

	    float lengthSquared() const
	    {
	        return x*x + y*y + z*z;
	    }

	    // calculate length of vector.

	    float length() const
	    {
			return sqrt(x*x + y*y + z*z);
	    }

	    float length2d() const
	    {
			return sqrt(x*x + y*y);
	    }

	    // normalize vector and return reference to normalized self.

	    Vector& normalize()
	    {
	        const float magnitude = sqrt(x*x + y*y + z*z);
	        if (magnitude>epsilon)
	        {
	            const float scale = 1.0f / magnitude;
	            x *= scale;
	            y *= scale;
	            z *= scale;
	        }
			return *this;
	    }

	    // return unit length vector

	    Vector unit() const
	    {
	        Vector vector(*this);
	        vector.normalize();
	        return vector;
	    }

	    // test if vector is normalized.

		bool normalized() const
		{
			return equal(length(),1);
		}

	    // equals operator

		bool operator ==(const Vector &other) const
		{
			if (equal(x,other.x) && equal(y,other.y) && equal(z,other.z)) 
				return true;
			else 
				return false;
		}

	    // not equals operator

		bool operator !=(const Vector &other) const
		{
			return !(*this==other);
		}

	    // element access

	    float& operator [](int i)
	    {
	        assert(i>=0);
	        assert(i<=2);
	        return *(&x+i);
	    }

	    // element access (const)

	    const float& operator [](int i) const
	    {
	        assert(i>=0);
	        assert(i<=2);
	        return *(&x+i);
	    }

		friend inline Vector operator-(const Vector &a);
		friend inline Vector operator+(const Vector &a, const Vector &b);
		friend inline Vector operator-(const Vector &a, const Vector &b);
		friend inline Vector operator*(const Vector &a, const Vector &b);
		friend inline Vector& operator+=(Vector &a, const Vector &b);
		friend inline Vector& operator-=(Vector &a, const Vector &b);
		friend inline Vector& operator*=(Vector &a, const Vector &b);

		friend inline Vector operator*(const Vector &a, float s);
		friend inline Vector operator/(const Vector &a, float s);
		friend inline Vector& operator*=(Vector &a, float s);
		friend inline Vector& operator/=(Vector &a, float s);
		friend inline Vector operator*(float s, const Vector &a);
		friend inline Vector& operator*=(float s, Vector &a);

	    float x;        // x component of vector
	    float y;        // y component of vector
	    float z;        // z component of vector
	};

  	inline Vector operator-(const Vector &a)
	{
		return Vector(-a.x, -a.y, -a.z);
	}
  
	inline Vector operator+(const Vector &a, const Vector &b)
	{
		return Vector(a.x+b.x, a.y+b.y, a.z+b.z);
	}

	inline Vector operator-(const Vector &a, const Vector &b)
	{
		return Vector(a.x-b.x, a.y-b.y, a.z-b.z);
	}

	inline Vector operator*(const Vector &a, const Vector &b)
	{
		return Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
	}

	inline Vector& operator+=(Vector &a, const Vector &b)
	{
		a.x += b.x;
		a.y += b.y;
		a.z += b.z;
		return a;
	}

	inline Vector& operator-=(Vector &a, const Vector &b)
	{
		a.x -= b.x;
		a.y -= b.y;
		a.z -= b.z;
		return a;
	}

	inline Vector& operator*=(Vector &a, const Vector &b)
	{
		const float cx = a.y * b.z - a.z * b.y;
		const float cy = a.z * b.x - a.x * b.z;
		const float cz = a.x * b.y - a.y * b.x;
		a.x = cx;
		a.y = cy;
		a.z = cz;
		return a;
	}

	inline Vector operator*(const Vector &a, float s)
	{
		return Vector(a.x*s, a.y*s, a.z*s);
	}

	inline Vector operator/(const Vector &a, float s)
	{
		assert(s!=0);
		return Vector(a.x/s, a.y/s, a.z/s);
	}

	inline Vector& operator*=(Vector &a, float s)
	{
		a.x *= s;
		a.y *= s;
		a.z *= s;
		return a;
	}

	inline Vector& operator/=(Vector &a, float s)
	{
		assert(s!=0);
		a.x /= s;
		a.y /= s;
		a.z /= s;
		return a;
	}

	inline Vector operator*(float s, const Vector &a)
	{
		return Vector(a.x*s, a.y*s, a.z*s);
	}

	inline Vector& operator*=(float s, Vector &a)
	{
		a.x *= s;
		a.y *= s;
		a.z *= s;
		return a;
	}

/*
 	4x4 matrix class.

	The convention here is post-multiplication by a column vector.
	ie. x = Ab, where x and b are column vectors.

	Please note that in cases where a matrix is pre-multiplied by a 
	vector, we then assume that the vector is a row vector.
	This operation is then equivalent to post multiplying the column
	vector by the transpose of the actual matrix.

	If you wish to think of this matrix in terms of basis vectors,
	then by convention, the rows of this matrix form the set of
	basis vectors.

	When composing matrix transforms A * B * C * D, note that the
	actual order of operations as visible in the resultant matrix
	is D, C, B, A. Alternatively, you can view transforms as changing
	coordinate system, then the coordinate systems are changed 
	in order A, B, C, D.
*/

	class Matrix
	{
	public:

		// default constructor.

		Matrix() {};

		// construct a matrix from three basis vectors.
		// the x,y,z values from each of these basis vectors map to rows in the 3x3 sub matrix.
		// note: the rest of the matrix (row 4 and column 4 are set to identity)

		Matrix(const Vector &a, const Vector &b, const Vector &c)
		{
			// ax ay az 0
			// bx by bz 0
			// cx cy cz 0
			// 0  0  0  1

			m11 = a.x;
			m12 = a.y;
			m13 = a.z;
			m14 = 0;
			m21 = b.x;
			m22 = b.y;
			m23 = b.z;
			m24 = 0;
			m31 = c.x;
			m32 = c.y;
			m33 = c.z;
			m34 = 0;
			m41 = 0;
			m42 = 0;
			m43 = 0;
			m44 = 1;
		}

		// construct a matrix from explicit values for the 3x3 sub matrix.
		// note: the rest of the matrix (row 4 and column 4 are set to identity)

		Matrix(float m11, float m12, float m13,
			   float m21, float m22, float m23,
			   float m31, float m32, float m33)
		{
			this->m11 = m11;
			this->m12 = m12;
			this->m13 = m13;
			this->m14 = 0;
			this->m21 = m21;
			this->m22 = m22;
			this->m23 = m23;
			this->m24 = 0;
			this->m31 = m31;
			this->m32 = m32;
			this->m33 = m33;
			this->m34 = 0;
			this->m41 = 0;
			this->m42 = 0;
			this->m43 = 0;
			this->m44 = 1;
		}

		// construct a matrix from explicit entry values for the whole 4x4 matrix.

		Matrix(float m11, float m12, float m13, float m14,
			   float m21, float m22, float m23, float m24,
			   float m31, float m32, float m33, float m34,
			   float m41, float m42, float m43, float m44)
		{
			this->m11 = m11;
			this->m12 = m12;
			this->m13 = m13;
			this->m14 = m14;
			this->m21 = m21;
			this->m22 = m22;
			this->m23 = m23;
			this->m24 = m24;
			this->m31 = m31;
			this->m32 = m32;
			this->m33 = m33;
			this->m34 = m34;
			this->m41 = m41;
			this->m42 = m42;
			this->m43 = m43;
			this->m44 = m44;
		}

		// load matrix from raw float array.
		// data is assumed to be stored linearly in memory in row order, from left to right, top to bottom.

		Matrix(const float data[])
		{
			this->m11 = data[0];
			this->m12 = data[1];
			this->m13 = data[2];
			this->m14 = data[3];
			this->m21 = data[4];
			this->m22 = data[5];
			this->m23 = data[6];
			this->m24 = data[7];
			this->m31 = data[8];
			this->m32 = data[9];
			this->m33 = data[10];
			this->m34 = data[11];
			this->m41 = data[12];
			this->m42 = data[13];
			this->m43 = data[14];
			this->m44 = data[15];
		}

		// construct a matrix from explicit values for the 3x3 sub matrix.
		// note: the rest of the matrix (row 4 and column 4 are set to identity)

		void Initialize3x3( float m11, float m12, float m13,
						    float m21, float m22, float m23,
						    float m31, float m32, float m33 )
		{
			this->m11 = m11;
			this->m12 = m12;
			this->m13 = m13;
			this->m14 = 0;
			this->m21 = m21;
			this->m22 = m22;
			this->m23 = m23;
			this->m24 = 0;
			this->m31 = m31;
			this->m32 = m32;
			this->m33 = m33;
			this->m34 = 0;
			this->m41 = 0;
			this->m42 = 0;
			this->m43 = 0;
			this->m44 = 1;
		}
		
		// set all entries in matrix to zero.

		void zero()
		{
			m11 = 0;
			m12 = 0;
			m13 = 0;
			m14 = 0;
			m21 = 0;
			m22 = 0;
			m23 = 0;
			m24 = 0;
			m31 = 0;
			m32 = 0;
			m33 = 0;
			m34 = 0;
			m41 = 0;
			m42 = 0;
			m43 = 0;
			m44 = 0;
		}

		// set matrix to identity.

		void identity()
		{
			m11 = 1;
			m12 = 0;
			m13 = 0;
			m14 = 0;
			m21 = 0;
			m22 = 1;
			m23 = 0;
			m24 = 0;
			m31 = 0;
			m32 = 0;
			m33 = 1;
			m34 = 0;
			m41 = 0;
			m42 = 0;
			m43 = 0;
			m44 = 1;
		}

		// set to a translation matrix.

		void translate(float x, float y, float z)
		{
			m11 = 1;		  // 1 0 0 x 
			m12 = 0;		  // 0 1 0 y
			m13 = 0;		  // 0 0 1 z
			m14 = x;		  // 0 0 0 1
			m21 = 0;
			m22 = 1;
			m23 = 0;
			m24 = y;
			m31 = 0;
			m32 = 0;
			m33 = 1;
			m34 = z;
			m41 = 0;
			m42 = 0;
			m43 = 0;
			m44 = 1;
		}

		// set to a translation matrix.

		void translate(const Vector &vector)
		{
			m11 = 1;		  // 1 0 0 x 
			m12 = 0;		  // 0 1 0 y
			m13 = 0;		  // 0 0 1 z
			m14 = vector.x;   // 0 0 0 1
			m21 = 0;
			m22 = 1;
			m23 = 0;
			m24 = vector.y;
			m31 = 0;
			m32 = 0;
			m33 = 1;
			m34 = vector.z;
			m41 = 0;
			m42 = 0;
			m43 = 0;
			m44 = 1;
		}

		// set to a scale matrix.

		void scale(float s)
		{
			m11 = s;
			m12 = 0;
			m13 = 0;
			m14 = 0;
			m21 = 0;
			m22 = s;
			m23 = 0;
			m24 = 0;
			m31 = 0;
			m32 = 0;
			m33 = s;
			m34 = 0;
			m41 = 0;
			m42 = 0;
			m43 = 0;
			m44 = 1;
		}

		// set to a diagonal matrix.

		void diagonal(float a, float b, float c, float d = 1)
		{
			m11 = a;
			m12 = 0;
			m13 = 0;
			m14 = 0;
			m21 = 0;
			m22 = b;
			m23 = 0;
			m24 = 0;
			m31 = 0;
			m32 = 0;
			m33 = c;
			m34 = 0;
			m41 = 0;
			m42 = 0;
			m43 = 0;
			m44 = d;
		}

		// set to a rotation matrix about a specified axis - angle.
	
		void rotate( Vector axis, float angle )
		{
			// note: adapted from david eberly's code with permission
		
			if (axis.lengthSquared()<epsilonSquared)
			{
				identity();
			}
			else
			{
				axis.normalize();

				float fCos = (float) cos(angle);
				float fSin = (float) sin(angle);
				float fOneMinusCos = 1.0f-fCos;
				float fX2 = axis.x*axis.x;
				float fY2 = axis.y*axis.y;
				float fZ2 = axis.z*axis.z;
				float fXYM = axis.x*axis.y*fOneMinusCos;
				float fXZM = axis.x*axis.z*fOneMinusCos;
				float fYZM = axis.y*axis.z*fOneMinusCos;
				float fXSin = axis.x*fSin;
				float fYSin = axis.y*fSin;
				float fZSin = axis.z*fSin;
		
				m11 = fX2*fOneMinusCos+fCos;
				m12 = fXYM-fZSin;
				m13 = fXZM+fYSin;
				m14 = 0;
			
				m21 = fXYM+fZSin;
				m22 = fY2*fOneMinusCos+fCos;
				m23 = fYZM-fXSin;
				m24 = 0;
			
				m31 = fXZM-fYSin;
				m32 = fYZM+fXSin;
				m33 = fZ2*fOneMinusCos+fCos;
				m34 = 0;
			
				m41 = 0;
				m42 = 0;
				m43 = 0;
				m44 = 1;
			}
		}

		// set to a look at matrix.

		void lookat(const Vector &eye, const Vector &at, const Vector &up)
		{
			// left handed

			Vector z_axis = at - eye;
			Vector x_axis = up.cross(z_axis);
			Vector y_axis = z_axis.cross(x_axis);

			x_axis.normalize();
			y_axis.normalize();
			z_axis.normalize();

			m11	= x_axis.x;
			m12 = x_axis.y;
			m13 = x_axis.z;
			m14 = - x_axis.dot(eye);

			m21	= y_axis.x;
			m22 = y_axis.y;
			m23 = y_axis.z;
			m24 = - y_axis.dot(eye);

			m31	= z_axis.x;
			m32 = z_axis.y;
			m33 = z_axis.z;
			m34 = - z_axis.dot(eye);

			m41	= 0;
			m42 = 0;
			m43 = 0;
			m44 = 1;
		}

		// set to an orthographic projection matrix.

		void orthographic(float l, float r, float b, float t, float n, float f)
		{
			float sx = 1 / (r - l);
			float sy = 1 / (t - b);
			float sz = 1 / (f - n);
			m11 = 2 * sx;
			m21 = 0;
			m31 = 0;
			m41 = - (r+l) * sx;
			m12 = 0;
			m22 = 2 * sy;
			m32 = 0;
			m42 = - (t+b) * sy;
			m13 = 0;
			m23 = 0;
			m33 = -2 * sz;
			m43 = - (n+f) * sz;
			m14 = 0;
			m24 = 0;
			m34 = 0;
			m44 = 1;
		}

		// set to a perspective projection matrix.

		void perspective(float l, float r, float t, float b, float n, float f)
		{
			m11	= 2*n / (r-l);
			m12 = 0;
			m13 = 0;
			m14 = 0;

			m21 = 0;
			m22 = 2*n / (t-b);
			m23 = 0;
			m24 = 0;

			m31 = 0;
			m32 = 0;
			m33 = f / (f-n);
			m34 = n*f / (n-f);

			m41 = 0;
			m42 = 0;
			m43 = 1;
			m44 = 0;
		}

		// set to a perspective projection matrix specified in terms of field of view and aspect ratio.

		void perspective(float fov, float aspect, float n, float f)
		{
			const float t = tan(fov*0.5f) * n;
			const float b = -t;

			const float l = aspect * b;
			const float r = aspect * t;

			perspective(l,r,t,b,n,f);
		}

		// calculate determinant of 3x3 sub matrix.

		float determinant() const
		{
			return -m13*m22*m31 + m12*m23*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 + m11*m22*m33;
		}

		// determine if matrix is invertible.
		// note: currently only checks 3x3 sub matrix determinant.

		bool invertible() const
		{
			return !equal(determinant(),0);
		}

		// calculate inverse of matrix.

		Matrix inverse() const
		{
			Matrix matrix;
			inverse(matrix);
			return matrix;
		}

		// calculate inverse of matrix and write result to parameter matrix.

		void inverse(Matrix &inverse) const
		{
			const float determinant = this->determinant();

			assert(!equal(determinant,0));

			float k = 1.0f / determinant;

			inverse.m11 = (m22*m33 - m32*m23) * k;
			inverse.m12 = (m32*m13 - m12*m33) * k;
			inverse.m13 = (m12*m23 - m22*m13) * k;
			inverse.m21 = (m23*m31 - m33*m21) * k;
			inverse.m22 = (m33*m11 - m13*m31) * k;
			inverse.m23 = (m13*m21 - m23*m11) * k;
			inverse.m31 = (m21*m32 - m31*m22) * k;
			inverse.m32 = (m31*m12 - m11*m32) * k;
			inverse.m33 = (m11*m22 - m21*m12) * k;

			inverse.m14 = -(inverse.m11*m14 + inverse.m12*m24 + inverse.m13*m34);
			inverse.m24 = -(inverse.m21*m14 + inverse.m22*m24 + inverse.m23*m34);
			inverse.m34 = -(inverse.m31*m14 + inverse.m32*m24 + inverse.m33*m34);

			inverse.m41 = m41;
			inverse.m42 = m42;
			inverse.m43 = m43;
			inverse.m44 = m44;
		}

		// calculate transpose of matrix.

		Matrix transpose() const
		{
			Matrix matrix;
			transpose(matrix);
			return matrix;
		}

		// calculate transpose of matrix and write to parameter matrix.

		void transpose(Matrix &transpose) const
		{
			transpose.m11 = m11;
			transpose.m12 = m21;
			transpose.m13 = m31;
			transpose.m14 = m41;
			transpose.m21 = m12;
			transpose.m22 = m22;
			transpose.m23 = m32;
			transpose.m24 = m42;
			transpose.m31 = m13;
			transpose.m32 = m23;
			transpose.m33 = m33;
			transpose.m34 = m43;
			transpose.m41 = m14;
			transpose.m42 = m24;
			transpose.m43 = m34;
			transpose.m44 = m44;
		}

		// transform a vector by this matrix.
		// the convention used is post-multiplication by a column vector: x=Ab.

		void transform(Vector &vector) const
		{
			float x = vector.x * m11 + vector.y * m12 + vector.z * m13 + m14;
			float y = vector.x * m21 + vector.y * m22 + vector.z * m23 + m24;
			float z = vector.x * m31 + vector.y * m32 + vector.z * m33 + m34;
			vector.x = x;
			vector.y = y;
			vector.z = z;
		}

		// transform a vector by this matrix, store result in parameter.
		// the convention used is post-multiplication by a column vector: x=Ab.

		void transform(const Vector &vector, Vector &result) const
		{
			result.x = vector.x * m11 + vector.y * m12 + vector.z * m13 + m14;
			result.y = vector.x * m21 + vector.y * m22 + vector.z * m23 + m24;
			result.z = vector.x * m31 + vector.y * m32 + vector.z * m33 + m34;
		}

		// transform a vector by this matrix using only the 3x3 rotation submatrix.
		// the convention used is post-multiplication by a column vector: x=Ab.
	
		void transform3x3(Vector &vector) const
		{
			float x = vector.x * m11 + vector.y * m12 + vector.z * m13;
			float y = vector.x * m21 + vector.y * m22 + vector.z * m23;
			float z = vector.x * m31 + vector.y * m32 + vector.z * m33;
			vector.x = x;
			vector.y = y;
			vector.z = z;
		}
	
		// transform a vector by this matrix, store result in parameter. 3x3 rotation only.
		// the convention used is post-multiplication by a column vector: x=Ab.
	
		void transform3x3(const Vector &vector, Vector &result) const
		{
			result.x = vector.x * m11 + vector.y * m12 + vector.z * m13;
			result.y = vector.x * m21 + vector.y * m22 + vector.z * m23;
			result.z = vector.x * m31 + vector.y * m32 + vector.z * m33;
		}
	
		// add another matrix to this matrix.

		void add(const Matrix &matrix)
		{
			m11 += matrix.m11;
			m12 += matrix.m12;
			m13 += matrix.m13;
			m14 += matrix.m14;
			m21 += matrix.m21;
			m22 += matrix.m22;
			m23 += matrix.m23;
			m24 += matrix.m24;
			m31 += matrix.m31;
			m32 += matrix.m32;
			m33 += matrix.m33;
			m34 += matrix.m34;
			m41 += matrix.m41;
			m42 += matrix.m42;
			m43 += matrix.m43;
			m44 += matrix.m44;
		}

		// subtract a matrix from this matrix.

		void subtract(const Matrix &matrix)
		{
			m11 -= matrix.m11;
			m12 -= matrix.m12;
			m13 -= matrix.m13;
			m14 -= matrix.m14;
			m21 -= matrix.m21;
			m22 -= matrix.m22;
			m23 -= matrix.m23;
			m24 -= matrix.m24;
			m31 -= matrix.m31;
			m32 -= matrix.m32;
			m33 -= matrix.m33;
			m34 -= matrix.m34;
			m41 -= matrix.m41;
			m42 -= matrix.m42;
			m43 -= matrix.m43;
			m44 -= matrix.m44;
		}

		// multiply this matrix by a scalar.

		void multiply(float scalar)
		{
			m11 *= scalar;
			m12 *= scalar;
			m13 *= scalar;
			m14 *= scalar;
			m21 *= scalar;
			m22 *= scalar;
			m23 *= scalar;
			m24 *= scalar;
			m31 *= scalar;
			m32 *= scalar;
			m33 *= scalar;
			m34 *= scalar;
			m41 *= scalar;
			m42 *= scalar;
			m43 *= scalar;
			m44 *= scalar;
		}

		// multiply two matrices.

		void multiply(const Matrix &matrix, Matrix &result)
		{
			result.m11 = m11*matrix.m11 + m12*matrix.m21 + m13*matrix.m31 + m14*matrix.m41;
			result.m12 = m11*matrix.m12 + m12*matrix.m22 + m13*matrix.m32 + m14*matrix.m42;
			result.m13 = m11*matrix.m13 + m12*matrix.m23 + m13*matrix.m33 + m14*matrix.m43;
			result.m14 = m11*matrix.m14 + m12*matrix.m24 + m13*matrix.m34 + m14*matrix.m44;
			result.m21 = m21*matrix.m11 + m22*matrix.m21 + m23*matrix.m31 + m24*matrix.m41;
			result.m22 = m21*matrix.m12 + m22*matrix.m22 + m23*matrix.m32 + m24*matrix.m42;
			result.m23 = m21*matrix.m13 + m22*matrix.m23 + m23*matrix.m33 + m24*matrix.m43;
			result.m24 = m21*matrix.m14 + m22*matrix.m24 + m23*matrix.m34 + m24*matrix.m44;
			result.m31 = m31*matrix.m11 + m32*matrix.m21 + m33*matrix.m31 + m34*matrix.m41;
			result.m32 = m31*matrix.m12 + m32*matrix.m22 + m33*matrix.m32 + m34*matrix.m42;
			result.m33 = m31*matrix.m13 + m32*matrix.m23 + m33*matrix.m33 + m34*matrix.m43;
			result.m34 = m31*matrix.m14 + m32*matrix.m24 + m33*matrix.m34 + m34*matrix.m44;
			result.m41 = m41*matrix.m11 + m42*matrix.m21 + m43*matrix.m31 + m44*matrix.m41;
			result.m42 = m41*matrix.m12 + m42*matrix.m22 + m43*matrix.m32 + m44*matrix.m42;
			result.m43 = m41*matrix.m13 + m42*matrix.m23 + m43*matrix.m33 + m44*matrix.m43;
			result.m44 = m41*matrix.m14 + m42*matrix.m24 + m43*matrix.m34 + m44*matrix.m44;
		}

		// equals operator

		bool operator ==(const Matrix &other) const
		{
			if (equal(m11,other.m11) &&
				equal(m12,other.m12) &&
				equal(m13,other.m13) &&
				equal(m14,other.m14) &&
				equal(m21,other.m21) &&
				equal(m22,other.m22) &&
				equal(m23,other.m23) &&
				equal(m24,other.m24) &&
				equal(m31,other.m31) &&
				equal(m32,other.m32) &&
				equal(m33,other.m33) &&
				equal(m34,other.m34) &&
				equal(m41,other.m41) &&
				equal(m42,other.m42) &&
				equal(m43,other.m43) &&
				equal(m44,other.m44)) return true;
			else return false;
		}

		// not equals operator

		bool operator !=(const Matrix &other) const
		{
			return !(*this==other);
		}

		// cute access to matrix elements via overloaded () operator.
		// use it like this: Matrix matrix; float element = matrix(row, column);

		float& operator()(int i, int j)
		{
			assert(i>=0);
			assert(i<=3);
			assert(j>=0);
			assert(j<=3);
			float *data = &m11;
			return data[(i<<2) + j];
		}

		// const version of element access above.

		const float& operator()(int i, int j) const
		{
			assert(i>=0);
			assert(i<=3);
			assert(j>=0);
			assert(j<=3);
			const float *data = &m11;
			return data[(i<<2) + j];
		}
	
		// data accessor for easy conversion to float* for OpenGL
	
		float* data()
		{
			return &m11;
		}

		friend inline Matrix operator-(const Matrix &a);
		friend inline Matrix operator+(const Matrix &a, const Matrix &b);
		friend inline Matrix operator-(const Matrix &a, const Matrix &b);
		friend inline Matrix operator*(const Matrix &a, const Matrix &b);
		friend inline Matrix& operator+=(Matrix &a, const Matrix &b);
		friend inline Matrix& operator-=(Matrix &a, const Matrix &b);
		friend inline Matrix& operator*=(Matrix &a, const Matrix &b);

		friend inline Vector operator*(const Matrix &matrix, const Vector &vector);
		friend inline Vector operator*(const Vector &vector, const Matrix &matrix);
		friend inline Vector& operator*=(Vector &vector, const Matrix &matrix);

		friend inline Matrix operator*(const Matrix &a, float s);
		friend inline Matrix operator/(const Matrix &a, float s);
		friend inline Matrix& operator*=(Matrix &a, float s);
		friend inline Matrix& operator/=(Matrix &a, float s);
		friend inline Matrix operator*(float s, const Matrix &a);
		friend inline Matrix& operator*=(float s, Matrix &a);
	
		// 4x4 matrix, index m[row][column], convention: pre-multiply column vector, Ax = b
		// hence: (m11,m21,m31) make up the x axis of the 3x3 sub matrix,
		// and (m14,m24,m34) is the translation vector.
	
		float m11,m12,m13,m14;			        
		float m21,m22,m23,m24;
		float m31,m32,m33,m34;          
		float m41,m42,m43,m44;
	};

	inline Matrix operator-(const Matrix &a)
	{
		return Matrix(-a.m11, -a.m12, -a.m13, -a.m14,
					  -a.m21, -a.m22, -a.m23, -a.m24,
					  -a.m31, -a.m32, -a.m33, -a.m34,
					  -a.m41, -a.m42, -a.m43, -a.m44);
	}

	inline Matrix operator+(const Matrix &a, const Matrix &b)
	{
		return Matrix(a.m11+b.m11, a.m12+b.m12, a.m13+b.m13, a.m14+b.m14,
					  a.m21+b.m21, a.m22+b.m22, a.m23+b.m23, a.m24+b.m24,
					  a.m31+b.m31, a.m32+b.m32, a.m33+b.m33, a.m34+b.m34,
					  a.m41+b.m41, a.m42+b.m42, a.m43+b.m43, a.m44+b.m44);
	}

	inline Matrix operator-(const Matrix &a, const Matrix &b)
	{
		return Matrix(a.m11-b.m11, a.m12-b.m12, a.m13-b.m13, a.m14-b.m14,
					  a.m21-b.m21, a.m22-b.m22, a.m23-b.m23, a.m24-b.m24,
					  a.m31-b.m31, a.m32-b.m32, a.m33-b.m33, a.m34-b.m34,
					  a.m41-b.m41, a.m42-b.m42, a.m43-b.m43, a.m44-b.m44);
	}

	inline Matrix operator*(const Matrix &a, const Matrix &b)
	{
		return Matrix(a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31 + a.m14*b.m41,
					  a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32 + a.m14*b.m42,
					  a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33 + a.m14*b.m43,
					  a.m11*b.m14 + a.m12*b.m24 + a.m13*b.m34 + a.m14*b.m44,
					  a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31 + a.m24*b.m41,
					  a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32 + a.m24*b.m42,
					  a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33 + a.m24*b.m43,
					  a.m21*b.m14 + a.m22*b.m24 + a.m23*b.m34 + a.m24*b.m44,
					  a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31 + a.m34*b.m41,
					  a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32 + a.m34*b.m42,
					  a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33 + a.m34*b.m43,
					  a.m31*b.m14 + a.m32*b.m24 + a.m33*b.m34 + a.m34*b.m44,
					  a.m41*b.m11 + a.m42*b.m21 + a.m43*b.m31 + a.m44*b.m41,
					  a.m41*b.m12 + a.m42*b.m22 + a.m43*b.m32 + a.m44*b.m42,
					  a.m41*b.m13 + a.m42*b.m23 + a.m43*b.m33 + a.m44*b.m43,
					  a.m41*b.m14 + a.m42*b.m24 + a.m43*b.m34 + a.m44*b.m44);
	}

	inline Matrix& operator+=(Matrix &a, const Matrix &b)
	{
		a.add(b);
		return a;
	}

	inline Matrix& operator-=(Matrix &a, const Matrix &b)
	{
		a.subtract(b);
		return a;
	}

	inline Matrix& operator*=(Matrix &a, const Matrix &b)
	{
		a = Matrix(a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31 + a.m14*b.m41,
				   a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32 + a.m14*b.m42,
				   a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33 + a.m14*b.m43,
				   a.m11*b.m14 + a.m12*b.m24 + a.m13*b.m34 + a.m14*b.m44,
				   a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31 + a.m24*b.m41,
				   a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32 + a.m24*b.m42,
				   a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33 + a.m24*b.m43,
				   a.m21*b.m14 + a.m22*b.m24 + a.m23*b.m34 + a.m24*b.m44,
				   a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31 + a.m34*b.m41,
				   a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32 + a.m34*b.m42,
				   a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33 + a.m34*b.m43,
				   a.m31*b.m14 + a.m32*b.m24 + a.m33*b.m34 + a.m34*b.m44,
				   a.m41*b.m11 + a.m42*b.m21 + a.m43*b.m31 + a.m44*b.m41,
				   a.m41*b.m12 + a.m42*b.m22 + a.m43*b.m32 + a.m44*b.m42,
				   a.m41*b.m13 + a.m42*b.m23 + a.m43*b.m33 + a.m44*b.m43,
				   a.m41*b.m14 + a.m42*b.m24 + a.m43*b.m34 + a.m44*b.m44);
		return a;											 
	}

	inline Vector operator*(const Matrix &matrix, const Vector &vector)
	{
		return Vector(vector.x * matrix.m11 + vector.y * matrix.m12 + vector.z * matrix.m13 + matrix.m14,
					  vector.x * matrix.m21 + vector.y * matrix.m22 + vector.z * matrix.m23 + matrix.m24,
					  vector.x * matrix.m31 + vector.y * matrix.m32 + vector.z * matrix.m33 + matrix.m34);
	}

	inline Vector operator*(const Vector &vector, const Matrix &matrix)
	{
		// when we premultiply x*A we assume the vector is a row vector

		return Vector(vector.x * matrix.m11 + vector.y * matrix.m21 + vector.z * matrix.m31 + matrix.m41,
					  vector.x * matrix.m12 + vector.y * matrix.m22 + vector.z * matrix.m32 + matrix.m42,
					  vector.x * matrix.m13 + vector.y * matrix.m23 + vector.z * matrix.m33 + matrix.m43);
	}

	inline Vector& operator*=(Vector &vector, const Matrix &matrix)
	{
		// currently equivalent to: vector = matrix * vector (not vector*matrix!)	
		const float rx = vector.x * matrix.m11 + vector.y * matrix.m12 + vector.z * matrix.m13 + matrix.m14;
		const float ry = vector.x * matrix.m21 + vector.y * matrix.m22 + vector.z * matrix.m23 + matrix.m24;
		const float rz = vector.x * matrix.m31 + vector.y * matrix.m32 + vector.z * matrix.m33 + matrix.m34;
		vector.x = rx;
		vector.y = ry;
		vector.z = rz;
		return vector;
	}
	
	inline Matrix operator*(const Matrix &a, float s)
	{
		return Matrix(s*a.m11, s*a.m12, s*a.m13, s*a.m14,
					  s*a.m21, s*a.m22, s*a.m23, s*a.m24,
					  s*a.m31, s*a.m32, s*a.m33, s*a.m34,
					  s*a.m41, s*a.m42, s*a.m43, s*a.m44);
	}

	inline Matrix operator/(const Matrix &a, float s)
	{
		assert(s!=0);
		const float inv = 1.0f / s;
		return Matrix(inv*a.m11, inv*a.m12, inv*a.m13, inv*a.m14,
					  inv*a.m21, inv*a.m22, inv*a.m23, inv*a.m24,
					  inv*a.m31, inv*a.m32, inv*a.m33, inv*a.m34,
					  inv*a.m41, inv*a.m42, inv*a.m43, inv*a.m44);
	}

	inline Matrix& operator*=(Matrix &a, float s)
	{
		a.multiply(s);
		return a;
	}

	inline Matrix& operator/=(Matrix &a, float s)
	{
		assert(s!=0);
		a.multiply(1.0f/s);
		return a;
	}

	inline Matrix operator*(float s, const Matrix &a)
	{
		return Matrix(s*a.m11, s*a.m12, s*a.m13, s*a.m14,
					  s*a.m21, s*a.m22, s*a.m23, s*a.m24,
					  s*a.m31, s*a.m32, s*a.m33, s*a.m34,
					  s*a.m41, s*a.m42, s*a.m43, s*a.m44);
	}

	inline Matrix& operator*=(float s, Matrix &a)
	{
		a.multiply(s);
		return a;
	}
	
	// A quaternion.
	// This quaternion class is generic and may be non-unit, however most 
	// anticipated uses of quaternions are unit length representing
	// a rotation 2*acos(w) about the axis (x,y,z).

	class Quaternion
	{
	public:

		// default constructor.

		Quaternion() {}

		// construct quaternion from real component w and imaginary x,y,z.

		Quaternion( float w, float x, float y, float z )
		{
			this->w = w;
			this->x = x;
			this->y = y;
			this->z = z;
		}

		// construct quaternion from angle-axis

		Quaternion( float angle, const Vector & axis )
		{
			const float a = angle * 0.5f;
			const float s = (float) sin( a );
			const float c = (float) cos( a );
			w = c;
			x = axis.x * s;
			y = axis.y * s;
			z = axis.z * s;
		}

		// construct quaternion from rotation matrix.

		Quaternion( const Matrix & matrix )
		{
			// Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
			// article "Quaternion Calculus and Fast Animation".

			const float trace = matrix.m11 + matrix.m22 + matrix.m33;

			if ( trace > 0 )
			{
				// |w| > 1/2, may as well choose w > 1/2

				float root = sqrt( trace + 1.0f );  // 2w
				w = 0.5f * root;
				root = 0.5f / root;  // 1/(4w)
				x = ( matrix.m32 - matrix.m23 ) * root;
				y = ( matrix.m13 - matrix.m31 ) * root;
				z = ( matrix.m21 - matrix.m12 ) * root;
			}
			else
			{
				// |w| <= 1/2

				static int next[3] = { 2, 3, 1 };

				int i = 1;
				if ( matrix.m22 > matrix.m11 )  
					i = 2;
				if ( matrix.m33 > matrix(i,i) ) 
					i = 3;
				int j = next[i];
				int k = next[j];

				float root = sqrt( matrix(i,i) - matrix(j,j) - matrix(k,k) + 1.0f );
				float * quaternion[3] = { &x, &y, &z };
				*quaternion[i] = 0.5f * root;
				root = 0.5f / root;
				w = ( matrix(k,j) - matrix(j,k) ) * root;
				*quaternion[j] = ( matrix(j,i) + matrix(i,j) ) * root;
				*quaternion[k] = ( matrix(k,i) + matrix(i,k) ) * root;
			}
		}

		// convert quaternion to matrix.

		void to_matrix( Matrix & matrix ) const
		{
			// from david eberly's freemagic 
			//  used with permission.

			float fTx  = 2.0f*x;
			float fTy  = 2.0f*y;
			float fTz  = 2.0f*z;
			float fTwx = fTx*w;
			float fTwy = fTy*w;
			float fTwz = fTz*w;
			float fTxx = fTx*x;
			float fTxy = fTy*x;
			float fTxz = fTz*x;
			float fTyy = fTy*y;
			float fTyz = fTz*y;
			float fTzz = fTz*z;

			matrix.Initialize3x3( 1.0f - ( fTyy + fTzz ), fTxy - fTwz, fTxz + fTwy,
						          fTxy + fTwz, 1.0f - ( fTxx + fTzz ), fTyz - fTwx,
								  fTxz - fTwy, fTyz + fTwx, 1.0f - ( fTxx + fTyy ) );
		}
        
		// convert quaternion to axis-angle.

		void axisAngle( Vector & axis, float & angle ) const
		{
			const float squareLength = x*x + y*y + z*z;

			if ( squareLength > epsilonSquared )
			{
				angle = 2.0f * (float) acos(w);
				const float inverseLength = 1.0f / (float) powf( squareLength, 0.5f );
				axis.x = x * inverseLength;
				axis.y = y * inverseLength;
				axis.z = z * inverseLength;
			}
			else
			{
				angle = 0.0f;
				axis.x = 1.0f;
				axis.y = 0.0f;
				axis.z = 0.0f;
			}
		}

		// set quaternion to zero.

		void zero()
		{
			w = 0;
			x = 0;
			y = 0;
			z = 0;
		}

		// set quaternion to identity.

		void identity()
		{
			w = 1;
			x = 0;
			y = 0;
			z = 0;
		}

		// add another quaternion to this quaternion.

		void add( const Quaternion & q )
		{
			w += q.w;
			x += q.x;
			y += q.y;
			z += q.z;
		}

		// subtract another quaternion from this quaternion.

		void subtract( const Quaternion & q )
		{
			w -= q.w;
			x -= q.x;
			y -= q.y;
			z -= q.z;
		}

		// multiply this quaternion by a scalar.

		void multiply( float s  )
		{
			w *= s;
			x *= s;
			y *= s;
			z *= s;
		}

		// divide this quaternion by a scalar.

		void divide( float s )
		{
			assert( s != 0 );
			const float inv = 1.0f / s;
			w *= inv;
			x *= inv;
			y *= inv;
			z *= inv;
		}

		// multiply this quaternion with another quaternion.

		void multiply( const Quaternion & q )
		{
			const float rw = w*q.w - x*q.x - y*q.y - z*q.z;
			const float rx = w*q.x + x*q.w + y*q.z - z*q.y;
			const float ry = w*q.y - x*q.z + y*q.w + z*q.x;
			const float rz = w*q.z + x*q.y - y*q.x + z*q.w;
			w = rw;
			x = rx;
			y = ry;
			z = rz;
		}

		// multiply this quaternion with another quaternion and store result in parameter.

		void multiply( const Quaternion & q, Quaternion & result ) const
		{
			result.w = w*q.w - x*q.x - y*q.y - z*q.z;
			result.x = w*q.x + x*q.w + y*q.z - z*q.y;
			result.y = w*q.y - x*q.z + y*q.w + z*q.x;
			result.z = w*q.z + x*q.y - y*q.x + z*q.w;
		}

		// dot product of two quaternions.

 		float dot( const Quaternion & q ) const
		{
			return w*q.w + x*q.x + y*q.y + z*q.z;
		}

		// calculate conjugate of quaternion.

		Quaternion conjugate()
		{
			return Quaternion( w, -x, -y, -z );
		}

		// calculate conjugate of quaternion and store result in parameter.

		void conjugate( Quaternion & result ) const
		{
			result = Quaternion( w, -x, -y, -z );
		}
		
		// calculate length of quaternion

		float length() const
		{
			return sqrt( w*w + x*x + y*y + z*z );
		}

		// calculate norm of quaternion.

		float norm() const
		{
			return w*w + x*x + y*y + z*z;
		}

		// normalize quaternion.

		void normalize()
		{
			const float length = this->length();

			if (length == 0)
			{
				w = 1;
				x = 0;
				y = 0;
				z = 0;
			}
			else
			{
				float inv = 1.0f / length;
				x = x * inv;
				y = y * inv;
				z = z * inv;
				w = w * inv;
			}
		}

		// calculate log n of quaternion
		
		Quaternion ln() const
		{
			float r = sqrtf( x*x + y*y + z*z );
			float t = r > 0.00001f ? atan2f( r, w ) / r : 0.0f;
			return Quaternion( 0.5f * logf( norm() ), t*x, t*y, t*z );
		}
		
		// calculate the exponent of a quaternion
		
		Quaternion exp() 
		{
			float r = sqrtf( x*x + y*y + z*z );
			float et = expf( w );
			float s = r >= 0.00001f ? et * sinf(r) / r : 0.0f;
			return Quaternion( et * cosf(r), s*x, s*y, s*z );
		}

		// transform vector by quaternion
		
		Vector transform( const Vector & input )
		{
			Quaternion inv = inverse();
			Quaternion a( 0, input.x, input.y, input.z );
			Quaternion r = ( *this * a ) * inv;
			return Vector( r.x, r.y, r.z );
		}

		// check if quaternion is normalized

		bool normalized() const
		{
			return equal( norm(), 1 );
		}

		// calculate inverse of quaternion

		Quaternion inverse() const
		{
			const float n = norm();
			assert( n != 0 );
			return Quaternion( w/n, -x/n, -y/n, -z/n ); 
		}

		// calculate inverse of quaternion and store result in parameter.

		void inverse( Quaternion & result ) const
		{
			const float n = norm();
			result = Quaternion( w/n, -x/n, -y/n, -z/n );
		}

		// equals operator

		bool operator == ( const Quaternion & other ) const
		{
			return equal( w, other.w ) && equal( x, other.x ) && equal( y, other.y ) && equal( z, other.z );
		}

		// not equals operator

		bool operator != ( const Quaternion & other ) const
		{
			return !( *this == other );
		}

		// element access

		float & operator [] ( int i )
		{
			assert( i >= 0 );
			assert( i <= 2 );
			return *( &w + i );
		}

		// element access (const)

		const float & operator [] ( int i ) const
		{
			assert( i >= 0 );
			assert( i <= 2 );
			return *( &w + i );
		}

		friend inline Quaternion operator - ( const Quaternion & a );
		friend inline Quaternion operator + ( const Quaternion & a, const Quaternion & b);
		friend inline Quaternion operator - ( const Quaternion & a, const Quaternion & b);
		friend inline Quaternion operator * ( const Quaternion & a, const Quaternion & b);
		friend inline Quaternion& operator += ( Quaternion & a, const Quaternion & b);
		friend inline Quaternion& operator -= ( Quaternion & a, const Quaternion & b);
		friend inline Quaternion& operator *= ( Quaternion & a, const Quaternion & b);

		friend inline bool operator == ( const Quaternion & q, float scalar );
		friend inline bool operator != ( const Quaternion & q, float scalar );
		friend inline bool operator == ( float scalar, const Quaternion & q );
		friend inline bool operator != ( float scalar, const Quaternion & q );

		friend inline Quaternion operator * ( const Quaternion & a, float s);
		friend inline Quaternion operator * ( float s, const Quaternion & a );
		friend inline Quaternion operator / ( const Quaternion & a, float s);
		friend inline Quaternion & operator *= ( Quaternion & a, float s );
		friend inline Quaternion & operator /= ( Quaternion & a, float s );
		friend inline Quaternion & operator *= ( float s, Quaternion & a );

		float w;        // w component of quaternion
		float x;        // x component of quaternion
		float y;        // y component of quaternion
		float z;        // z component of quaternion
	};


	inline Quaternion operator - ( const Quaternion & a )
	{
		return Quaternion( -a.w, -a.x, -a.y, -a.z );
	}

	inline Quaternion operator + ( const Quaternion & a, const Quaternion & b )
	{
		return Quaternion( a.w + b.w, a.x + b.x, a.y + b.y, a.z + b.z );
	}

	inline Quaternion operator - ( const Quaternion & a, const Quaternion & b )
	{
		return Quaternion( a.w - b.w, a.x - b.x, a.y - b.y, a.z - b.z );
	}

	inline Quaternion operator * ( const Quaternion & a, const Quaternion & b )
	{
		return Quaternion( a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z, 
						   a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
						   a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
						   a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w );
	}

	inline Quaternion & operator += ( Quaternion & a, const Quaternion & b )
	{
		a.w += b.w;
		a.x += b.x;
		a.y += b.y;
		a.z += b.z;
		return a;
	}

	inline Quaternion & operator -= ( Quaternion & a, const Quaternion & b )
	{
		a.w -= b.w;
		a.x -= b.x;
		a.y -= b.y;
		a.z -= b.z;
		return a;
	}

	inline Quaternion & operator *= ( Quaternion & a, const Quaternion & b )
	{
		a.multiply( b );
		return a;
	}

	inline bool operator == ( const Quaternion & q, float scalar )
	{
		return equal( q.w, scalar ) && equal( q.x, 0 ) && equal( q.y,0 ) && equal( q.z,0 );
	}

	inline bool operator != ( const Quaternion & q, float scalar )
	{
		return ! ( q == scalar );
	}

	inline bool operator == ( float scalar, const Quaternion & q )
	{
		return equal( q.w, scalar ) && equal( q.x, 0 ) && equal( q.y, 0 ) && equal( q.z, 0 );
	}

	inline bool operator != ( float scalar, const Quaternion & q )
	{
		return !( q == scalar );
	}

	inline Quaternion operator * ( const Quaternion & a, float s )
	{
		return Quaternion( a.w * s, a.x * s, a.y * s, a.z * s );
	}

	inline Quaternion operator / ( const Quaternion & a, float s )
	{
		return Quaternion( a.w / s, a.x / s, a.y / s, a.z / s );
	}

	inline Quaternion & operator *= ( Quaternion & a, float s )
	{
		a.multiply( s );
		return a;
	}

	inline Quaternion & operator /= ( Quaternion & a, float s )
	{
		a.divide( s );
		return a;
	}

	inline Quaternion operator * ( float s, const Quaternion & a )
	{
		return Quaternion( a.w * s, a.x * s, a.y * s, a.z * s );
	}

	inline Quaternion & operator *= ( float s, Quaternion & a )
	{
		a.multiply( s );
		return a;
	}

	inline Quaternion slerp( const Quaternion & a, const Quaternion & _b, float t ) 
	{
		assert( t >= 0 );
		assert( t <= 1 );
		
		Quaternion b = _b;

		float cosine = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
		if ( cosine < 0 )
			b = -b;

		Quaternion c = a * ( 1 - t ) + b * t; 
		c.normalize();
		return c;

		/*
		if ( ( 1 - cosine ) < epsilon ) 
			return a * ( 1 - t ) + b * t; 
			*/

		/*
		float theta = acos( cosine ); 
		float sine = sin( theta ); 
		float beta = sin( ( 1 - t ) * theta ) / sine; 
		float alpha = sin( t * theta ) / sine; 

		return a * beta + b * alpha; 
		*/
	}

/*
inline Quaternion slerp( const Quaternion & a, const Quaternion & b, float t ) 
{
	assert( t >= 0 );
	assert( t <= 1 );

	float flip = +1;

	float cosine = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;

	if ( cosine < 0 )
	{ 
		cosine = -cosine; 
		flip = -1; 
	} 

	if ( ( 1 - cosine ) < epsilon ) 
		return a * ( 1 - t ) + b * ( t * flip ); 

	float theta = acos( cosine ); 
	float sine = sin( theta ); 
	float beta = sin( ( 1 - t ) * theta ) / sine; 
	float alpha = sin( t * theta ) / sine * flip; 

	return a * beta + b * alpha; 
}
*/

	inline Quaternion nlerp( const Quaternion & a, const Quaternion & b, float t ) 
	{
		assert( t >= 0 );
		assert( t <= 1 );

		float cosine = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;

		Quaternion output;
		if ( cosine >= 0 ) 
		{ 
			output.w = a.w + ( b.w - a.w ) * t;
			output.x = a.x + ( b.x - a.x ) * t;
			output.y = a.y + ( b.y - a.y ) * t;
			output.z = a.z + ( b.z - a.z ) * t;
		}
		else
		{
			output.w = a.w + ( -b.w - a.w ) * t;
			output.x = a.x + ( -b.x - a.x ) * t;
			output.y = a.y + ( -b.y - a.y ) * t;
			output.z = a.z + ( -b.z - a.z ) * t;
		}

		output.normalize();

		return output;
	}
    
    // build 4x4 transform matrix from position, orientation and scale
    
    inline void build_transform( float * matrix, const math::Vector & position, const math::Quaternion & orientation, float scale )
    {
        float fTx  = 2.0f * orientation.x;
        float fTy  = 2.0f * orientation.y;
        float fTz  = 2.0f * orientation.z;
        float fTwx = fTx * orientation.w;
        float fTwy = fTy * orientation.w;
        float fTwz = fTz * orientation.w;
        float fTxx = fTx * orientation.x;
        float fTxy = fTy * orientation.x;
        float fTxz = fTz * orientation.x;
        float fTyy = fTy * orientation.y;
        float fTyz = fTz * orientation.y;
        float fTzz = fTz * orientation.z;
        
        matrix[0] = scale * ( 1.0f - ( fTyy + fTzz ) );
        matrix[1] = scale * ( fTxy + fTwz );
        matrix[2] = scale * ( fTxz - fTwy );
        matrix[3] = 0;
        
        matrix[4] = scale * ( fTxy - fTwz );
        matrix[5] = scale * ( 1.0f - ( fTxx + fTzz ) );
        matrix[6] = scale * ( fTyz + fTwx );
        matrix[7] = 0;
        
        matrix[8] = scale * ( fTxz + fTwy );
        matrix[9] = scale * ( fTyz - fTwx );
        matrix[10] = scale * ( 1.0f - ( fTxx + fTyy ) );
        matrix[11] = 0;
        
        matrix[12] = position.x;
        matrix[13] = position.y;
        matrix[14] = position.z;
        matrix[15] = 1;
    }
    
    inline void build_translation( float * matrix, const math::Vector & position )
    {
        matrix[0] = 1;
        matrix[1] = 0;
        matrix[2] = 0;
        matrix[3] = 0;
        
        matrix[4] = 0;
        matrix[5] = 1;
        matrix[6] = 0;
        matrix[7] = 0;
        
        matrix[8] = 0;
        matrix[9] = 0;
        matrix[10] = 1;
        matrix[11] = 0;
        
        matrix[12] = position.x;
        matrix[13] = position.y;
        matrix[14] = position.z;
        matrix[15] = 1;
    }

    inline void build_rotation( float * matrix, const math::Quaternion & orientation )
    {
        float fTx  = 2.0f * orientation.x;
        float fTy  = 2.0f * orientation.y;
        float fTz  = 2.0f * orientation.z;
        float fTwx = fTx * orientation.w;
        float fTwy = fTy * orientation.w;
        float fTwz = fTz * orientation.w;
        float fTxx = fTx * orientation.x;
        float fTxy = fTy * orientation.x;
        float fTxz = fTz * orientation.x;
        float fTyy = fTy * orientation.y;
        float fTyz = fTz * orientation.y;
        float fTzz = fTz * orientation.z;
        
        matrix[0] = ( 1.0f - ( fTyy + fTzz ) );
        matrix[1] = ( fTxy + fTwz );
        matrix[2] = ( fTxz - fTwy );
        matrix[3] = 0;
        
        matrix[4] = ( fTxy - fTwz );
        matrix[5] = ( 1.0f - ( fTxx + fTzz ) );
        matrix[6] = ( fTyz + fTwx );
        matrix[7] = 0;
        
        matrix[8] = ( fTxz + fTwy );
        matrix[9] = ( fTyz - fTwx );
        matrix[10] = ( 1.0f - ( fTxx + fTyy ) );
        matrix[11] = 0;
        
        matrix[12] = 0;
        matrix[13] = 0;
        matrix[14] = 0;
        matrix[15] = 1;
    }
    
    inline void build_scale( float * matrix, float scale )
    {
        matrix[0] = scale;
        matrix[1] = 0;
        matrix[2] = 0;
        matrix[3] = 0;
        
        matrix[4] = 0;
        matrix[5] = scale;
        matrix[6] = 0;
        matrix[7] = 0;
        
        matrix[8] = 0;
        matrix[9] = 0;
        matrix[10] = scale;
        matrix[11] = 0;
        
        matrix[12] = 0;
        matrix[13] = 0;
        matrix[14] = 0;
        matrix[15] = 1;
    }
        
    // get axis angle from quaternion
    
	inline void GetAxisAngle( const math::Quaternion & quaternion, math::Vector & axis, float & angle )
	{
		const float w = quaternion.w;
		const float x = quaternion.x;
		const float y = quaternion.y;
		const float z = quaternion.z;
		
		const float lengthSquared = x*x + y*y + z*z;
		
		if ( lengthSquared > 0.00001f )
		{
			angle = 2.0f * (float) acos( w );
			const float inverseLength = 1.0f / (float) pow( lengthSquared, 0.5f );
			axis.x = x * inverseLength;
			axis.y = y * inverseLength;
			axis.z = z * inverseLength;
		}
		else
		{
			angle = 0.0f;
			axis.x = 1.0f;
			axis.y = 0.0f;
			axis.z = 0.0f;
		}
	}

	// hermite spline from cubic.org
	
	inline void hermite_spline( float t,
					            const math::Vector & p0, 
	                            const math::Vector & p1,
	                            const math::Vector & t0,
	                            const math::Vector & t1,
					            math::Vector & output )
	{
		const float t2 = t*t;
		const float t3 = t2*t;
	    float h1 =  2*t3 - 3*t2 + 1;
	    float h2 = -2*t3 + 3*t2;
	    float h3 =    t3 - 2*t2 + t;
	    float h4 =    t3 - t2;
	    output = h1*p0 + h2*p1 + h3*t0 + h4*t1;
	}

	// nlerp interpolation using hermite spline
	// just treat the quaternion like a linear vector, tangents are quaternion "spin"
	// RAD game tools guys use this for granny, it wont have a constant rotation angle
	// but it should be better than linear orientation interpolation (still C2 continuous)

	inline void hermite_spline( float t,
					            const math::Quaternion & p0, 
	                            const math::Quaternion & p1,
	                            const math::Quaternion & t0,
	                            const math::Quaternion & t1,
					            math::Quaternion & output )
	{
		const float t2 = t*t;
		const float t3 = t2*t;
	    float h1 =  2*t3 - 3*t2 + 1;
	    float h2 = -2*t3 + 3*t2;
	    float h3 =    t3 - 2*t2 + t;
	    float h4 =    t3 - t2;
		math::Quaternion _p0 = p0;
		math::Quaternion _p1 = p1;
		math::Quaternion _t0 = t0;
		math::Quaternion _t1 = t1;
		if ( _p0.dot(_p1) < 0 )
		{
			_p0 = -_p0;
			_t0 = -_t0;
		}
	   	output = h1*_p0 + h2*_p1 + h3*_t0 + h4*_t1;
		output.normalize();
	}
	
	// Plane class.
	// Represents a plane using a normal and a plane constant.

	struct Plane
	{
		Vector normal;          // the plane normal.
		float constant;         // the plane constant relative to the plane normal.

	    // Default constructor.
	    // normal is zero, constant is zero.

	    Plane()
	    {
	        normal.zero();
	        constant = 0;
	    }

	    // Create a plane given a normal and a point on the plane.

	    Plane(const Vector &normal, const Vector &point)
	    {
	        this->normal = normal;
	        this->constant = normal.dot(point);
	    }

	    // Create a plane given a normal and a plane constant.

	    Plane(const Vector &normal, float constant)
	    {
	        this->normal = normal;
	        this->constant = constant;
	    }

	    // Normalize the plane normal and adjust the plane constant to match.

		void normalize()
		{
			const float inverseLength = 1.0f / normal.length();
			normal *= inverseLength;
			constant *= inverseLength;
		}

	    // Clip a point against the plane.
	    // @param distance the minimum allowable distance between the point and plane.

	    void clip(Vector &point, float distance = 0.0f)
	    {
	        const float d = (point.dot(normal) - constant) - distance;
	        if (d<0)
	            point -= normal * d;
	    }

		// calculate distance from plane
	
		float distance( const Vector & point )
		{
			return point.dot( normal ) - constant;
		}
	};
	
	// Frustum
	
	struct Frustum
	{
		math::Plane left, right, front, back, top, bottom;
	};
}

#endif
