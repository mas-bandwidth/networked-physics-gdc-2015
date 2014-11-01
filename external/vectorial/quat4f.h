#ifndef VECTORIAL_QUAT4F_H
#define VECTORIAL_QUAT4F_H

namespace vectorial 
{
    struct quat4f
    {
        float x,y,z,w;

        quat4f()
        {
            // ...
        }

        quat4f( float w, float x, float y, float z )
        {
            this->w = w;
            this->x = x;
            this->y = y;
            this->z = z;
        }

        static quat4f identity()
        {
            quat4f q;
            q.x = 0;
            q.y = 0;
            q.z = 0;
            q.w = 1;
            return q;
        }

        static quat4f axisRotation( float angleRadians, vec3f axis )
        {
            const float a = angleRadians * 0.5f;
            const float s = (float) sin( a );
            const float c = (float) cos( a );
            quat4f q;
            q.w = c;
            q.x = axis.x() * s;
            q.y = axis.y() * s;
            q.z = axis.z() * s;
            return q;
        }

        float length() const
        {
            return sqrt( x*x + y*y + z*z + w*w );
        }

        void toMatrix( mat4f & matrix ) const
        {
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

            const float array[] = 
            {
                1.0f - ( fTyy + fTzz ), fTxy + fTwz, fTxz - fTwy, 0,
                fTxy - fTwz, 1.0f - ( fTxx + fTzz ), fTyz + fTwx, 0, 
                fTxz + fTwy, fTyz - fTwx, 1.0f - ( fTxx + fTyy ), 0,
                0, 0, 0, 1 
            };

            matrix.load( array );
        }

        void toAxisAngle( vec3f & axis, float & angle, const float epsilonSquared = 0.001f * 0.001f ) const
        {
            const float squareLength = x*x + y*y + z*z;
            if ( squareLength > epsilonSquared )
            {
                angle = 2.0f * (float) acos( w );
                const float inverseLength = 1.0f / (float) sqrt( squareLength );
                axis = vec3f( x, y, z ) * inverseLength;
            }
            else
            {
                axis = vec3f(1,0,0);
                angle = 0.0f;
            }
        }
    };

    inline quat4f multiply( const quat4f & q1, const quat4f & q2 )
    {
        quat4f result;
        result.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
        result.x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
        result.y = q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x;
        result.z = q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w;
        return result;
    }

    inline quat4f normalize( const quat4f & q, const float epsilon = 0.0001f )
    {
        const float length = q.length();
        CORE_ASSERT( length > epsilon );
        float inv = 1.0f / length;
        quat4f result;
        result.x = q.x * inv;
        result.y = q.y * inv;
        result.z = q.z * inv;
        result.w = q.w * inv;
        return result;
    }

    inline quat4f & operator += ( quat4f & q1, const quat4f & q2 )
    {
        q1.x += q2.x;
        q1.y += q2.y;
        q1.z += q2.z;
        q1.w += q2.w;
        return q1;
    }

    inline quat4f operator + ( quat4f & q1, const quat4f & q2 )
    {
        quat4f result;
        result.x = q1.x + q2.x;
        result.y = q1.y + q2.y;
        result.z = q1.z + q2.z;
        result.w = q1.w + q2.w;
        return result;
    }

    inline quat4f operator * ( const quat4f & q1, const quat4f & q2 )
    {
        return multiply( q1, q2 );
    }

    inline quat4f operator * ( const quat4f & q, float s )
    {
        quat4f result;
        result.x = q.x * s;
        result.y = q.y * s;
        result.z = q.z * s;
        result.w = q.w * s;
        return result;
    }

    inline quat4f operator * ( float s, const quat4f & q )
    {
        quat4f result;
        result.x = q.x * s;
        result.y = q.y * s;
        result.z = q.z * s;
        result.w = q.w * s;
        return result;
    }
}

#endif
