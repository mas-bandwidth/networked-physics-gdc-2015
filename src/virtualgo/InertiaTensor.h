#ifndef VIRTUALGO_INERTIA_TENSOR_H
#define VIRTUALGO_INERTIA_TENSOR_H

#include "virtualgo/Common.h"
#include "virtualgo/Biconvex.h"

namespace virtualgo
{
    using namespace vectorial;

    void CalculateSphereInertiaTensor( float mass, float r, mat4f & inertiaTensor, mat4f & inverseInertiaTensor )
    {
        const float i = 2.0f / 5.0f * mass * r * r;
        float values[] = { i, 0, 0, 0, 
                           0, i, 0, 0,
                           0, 0, i, 0,
                           0, 0, 0, 1 };
        float inverse_values[] = { 1/i,  0,   0,   0, 
                                    0, 1/i,   0,   0, 
                                    0,   0, 1/i,   0,
                                    0,   0,   0,   1 };
        inertiaTensor.load( values );
        inverseInertiaTensor.load( inverse_values );
    }

    void CalculateEllipsoidInertiaTensor( float mass, float a, float b, float c, mat4f & inertiaTensor, mat4f & inverseInertiaTensor )
    {
        const float i_a = 1.0f/5.0f * mass * ( b*b + c*c );
        const float i_b = 1.0f/5.0f * mass * ( a*a + c*c );
        const float i_c = 1.0f/5.0f * mass * ( a*a + b*b );

        float values[] = { i_a,   0,   0, 0, 
                             0, i_b,   0, 0,
                             0,   0, i_c, 0,
                             0,   0,   0, 1 };
                             
        float inverse_values[] = { 1/i_a,     0,     0,   0, 
                                       0, 1/i_b,     0,   0, 
                                       0,     0, 1/i_c,   0,
                                       0,     0,     0,   1 };
        inertiaTensor.load( values );
        inverseInertiaTensor.load( inverse_values );
    }

    float CalculateBiconvexVolume( const Biconvex & biconvex )
    {
        const float r = biconvex.GetSphereRadius();
        const float h = r - biconvex.GetHeight() / 2;
        return h*h + ( pi * r / 4 + pi * h / 24 );
    }

    void CalculateBiconvexInertiaTensor( float mass, const Biconvex & biconvex, vec3f & inertia, mat4f & inertiaTensor, mat4f & inverseInertiaTensor )
    {
        const float resolution = 0.01f;
        const float width = biconvex.GetWidth();
        const float height = biconvex.GetHeight();
        const float xy_steps = ceil( width / resolution );
        const float z_steps = ceil( height / resolution );
        const float dx = width / xy_steps;
        const float dy = width / xy_steps;
        const float dz = height / xy_steps;

        float sx = -width / 2;
        float sy = -width / 2;
        float sz = -height / 2;
        float ix = 0.0;
        float iy = 0.0;
        float iz = 0.0;
        
        const float v = CalculateBiconvexVolume( biconvex );
        const float p = mass / v;
        const float m = dx*dy*dz * p;
        
        for ( int index_z = 0; index_z <= z_steps; ++index_z )
        {
            for ( int index_y = 0; index_y <= xy_steps; ++index_y )
            {
                for ( int index_x = 0; index_x <= xy_steps; ++index_x )
                {
                    const float x = sx + index_x * dx;
                    const float y = sy + index_y * dy;
                    const float z = sz + index_z * dz;

                    vec3f point(x,y,z);

                    if ( PointInsideBiconvex_LocalSpace( point, biconvex ) )
                    {
                        const float rx2 = z*z + y*y;
                        const float ry2 = x*x + z*z;
                        const float rz2 = x*x + y*y;

                        ix += rx2 * m;
                        iy += ry2 * m;
                        iz += rz2 * m;
                    }
                }
            }
        }

        const float inertiaValues[] = { ix, iy, iz };

        const float inertiaTensorValues[] = { ix, 0,  0, 0, 
                                              0, iy,  0, 0, 
                                              0,  0, iz, 0, 
                                              0,  0,  0, 1 };

        const float inverseInertiaTensorValues[] = { 1/ix,    0,    0, 0, 
                                                        0, 1/iy,    0, 0, 
                                                        0,    0, 1/iz, 0, 
                                                        0,    0,    0, 1 };

        inertia.load( inertiaValues );
        inertiaTensor.load( inertiaTensorValues );
        inverseInertiaTensor.load( inverseInertiaTensorValues );
    }
}

#endif // #ifndef VIRTUALGO_INERTIA_TENSOR_H
