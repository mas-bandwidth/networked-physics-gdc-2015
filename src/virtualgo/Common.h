#ifndef VIRTUALGO_COMMON_H
#define VIRTUALGO_COMMON_H

#include "core/Core.h"
#include "vectorial/vec2f.h"
#include "vectorial/vec3f.h"
#include "vectorial/vec4f.h"
#include "vectorial/mat4f.h"
#include "vectorial/quat4f.h"

namespace virtualgo
{
    using namespace vectorial;

    const float pi = 3.1415926f;

    float DegToRad( float degrees )
    {
        return ( degrees / 360.0f ) * 2 * pi;
    }

    extern void PrintVector( vec3f vector );
    extern void PrintVector( vec4f vector );
    extern void PrintMatrix( mat4f matrix );

    inline vec3f TransformPoint( mat4f matrix, vec3f point )
    {
        return transformPoint( matrix, point );
    }

    inline vec3f TransformVector( mat4f matrix, vec3f normal )
    {
        return transformVector( matrix, normal );
    }

    inline vec4f TransformPlane( mat4f matrix, vec4f plane )
    {
        // hack: slow version -- original code is commented out. it does not seem to be getting correct w coordinate?!
        vec3f normal( plane.x(), plane.y(), plane.z() );
        float d = plane.w();
        vec3f point = normal * d;
        normal = TransformVector( matrix, normal );
        point = TransformPoint( matrix, point );
        d = dot( point, normal );
        return vec4f( normal.x(), normal.y(), normal.z(), d );

        /*
        // IMPORTANT: to transform a plane (nx,ny,nz,d) by a matrix multiply it by the inverse of the transpose
        // http://www.opengl.org/discussion_boards/showthread.php/159564-Clever-way-to-transform-plane-by-matrix
        mat4f m = RigidBodyInverse( transpose( matrix ) ) );
        return m * plane;
        */
    }

    inline float DecayFactor( float factor, float deltaTime, float ideal_fps = 60 )
    {
        return pow( factor, ideal_fps * deltaTime );
    }

    struct Frustum
    {
        vec4f left, right, front, back, top, bottom;
    };

    void CalculateFrustumPlanes( const mat4f & clipMatrix, Frustum & frustum );

    inline void AngularVelocityToSpin( const quat4f & orientation, vec3f angularVelocity, quat4f & spin )
    {
        spin = 0.5f * ( quat4f( 0, angularVelocity.x(), angularVelocity.y(), angularVelocity.z() ) * orientation );
    }

    inline void RigidBodyInverse( const mat4f & matrix, const mat4f & transposeRotation, mat4f & inverse )
    {
        // http://graphics.stanford.edu/courses/cs248-98-fall/Final/q4.html
        inverse = transposeRotation;
        const vec4f & translation = matrix.value.w;
        inverse.value.w = simd4f_create( -dot( matrix.value.x, translation ),
                                         -dot( matrix.value.y, translation ),
                                         -dot( matrix.value.z, translation ),
                                         1.0f );
    }

    struct RigidBodyTransform
    {    
        mat4f localToWorld, worldToLocal;

        RigidBodyTransform()
        {
            localToWorld.identity();
            localToWorld.identity();
        }

        RigidBodyTransform( const vec3f & position )
        {
            localToWorld.identity();
            localToWorld.value.w = simd4f_create( position.x(), position.y(), position.z(), 1 );
            
            worldToLocal.identity();
            worldToLocal.value.w = simd4f_create( -position.x(), -position.y(), -position.z(), 1 );
        }

        RigidBodyTransform( const vec3f & position, const mat4f & rotation, const mat4f & inverseRotation )
        {
            localToWorld = rotation;
            localToWorld.value.w = simd4f_create( position.x(), position.y(), position.z(), 1 );
            RigidBodyInverse( localToWorld, inverseRotation, worldToLocal );
        }

        void GetUp( vec3f & up ) const
        {
            up = transformVector( localToWorld, vec3f(0,0,1) );
        }

        void GetPosition( vec3f & position ) const
        {
            position = localToWorld.value.w;
        }
    };
}

#endif // #ifndef VIRTUALGO_COMMON_H
