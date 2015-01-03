#ifndef VECTORIAL_QUAT4F_H
#define VECTORIAL_QUAT4F_H

#include <math.h>
#include "vectorial/vec3f.h"
#include "vectorial/vec4f.h"

namespace vectorial 
{
    struct quat4f : public vec4f
    {
        quat4f() {}

        quat4f( float x, float y, float z, float w )
            : vec4f( x, y, z, w ) {}

        quat4f( vec4f v )
            : vec4f( v ) {}

        static quat4f identity()
        {
            return quat4f( 0, 0, 0, 1 );
        }

        static quat4f axis_rotation( float angle_radians, vec3f axis )
        {
            const float a = angle_radians * 0.5f;
            const float s = (float) sin( a );
            const float c = (float) cos( a );
            return quat4f( axis.x() * s, axis.y() * s, axis.z() * s, c );
        }

        void to_axis_angle( vec3f & axis, float & angle, const float epsilonSquared = 0.001f * 0.001f ) const
        {
            const float squareLength = length_squared( *this );

            const float _x = x();
            const float _y = y();
            const float _z = z();
            const float _w = w();

            if ( squareLength > epsilonSquared )
            {
                angle = 2.0f * (float) acos( _w );
                const float inverseLength = 1.0f / (float) sqrt( squareLength );
                axis = vec3f( _x, _y, _z ) * inverseLength;
            }
            else
            {
                axis = vec3f( 1, 0, 0 );
                angle = 0.0f;
            }
        }
    };

    static inline float norm( const quat4f & q )
    {
        return length_squared( q );
    }

    static inline quat4f exp( const quat4f & q ) 
    {
        float a[4];
        q.store( a );
        float r  = sqrt( a[0]*a[0] + a[1]*a[1] + a[2]*a[2] );
        float et = ::exp( a[3] );
        float s  = ( r >= 0.00001f ) ? et * sin(r) / r : 0;
        return quat4f( s*a[0],s *a[1], s*a[2], et*cos(r) );
    }

    static inline quat4f ln( const quat4f & q ) 
    {
        float a[4];
        q.store( a );
        float r = sqrt( a[0]*a[0] + a[1]*a[1] + a[2]*a[2] );
        float t = ( r > 0.00001f ) ? atan2( r, a[3] ) / r : 0;
        return quat4f( t*a[0], t*a[1], t*a[2], 0.5*log( norm(q) ) );
    }

    // todo: nlerp

    static inline quat4f slerp( float t, const quat4f & a, const quat4f & b ) 
    {
        float flip = 1;

        float cosine = dot( a, b );

        if ( cosine < 0 )
        { 
            cosine = -cosine; 
            flip = -1; 
        }

        const float epsilon = 0.001f;   

        if ( ( 1 - cosine ) < epsilon ) 
            return a * ( 1 - t ) + b * ( t * flip ); 

        float theta = acos( cosine ); 
        float sine = sin( theta ); 
        float beta = sin( ( 1 - t ) * theta ) / sine; 
        float alpha = sin( t * theta ) / sine;

        return a * beta + b * alpha * flip;
    }
}

#endif
