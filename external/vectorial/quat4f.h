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
}

#endif
