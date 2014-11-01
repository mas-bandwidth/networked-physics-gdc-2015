#include "virtualgo/Common.h"

namespace virtualgo
{
    void PrintVector( vec3f vector )
    {
        printf( "[%f,%f,%f]\n", vector.x(), vector.y(), vector.z() );
    }

    void PrintVector( vec4f vector )
    {
        printf( "[%f,%f,%f,%f]\n", vector.x(), vector.y(), vector.z(), vector.w() );
    }

    void PrintMatrix( mat4f matrix )
    {
        printf( "[%f,%f,%f,%f,\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f]\n", 
            simd4f_get_x(matrix.value.x), 
            simd4f_get_x(matrix.value.y), 
            simd4f_get_x(matrix.value.z), 
            simd4f_get_x(matrix.value.w), 
            simd4f_get_y(matrix.value.x), 
            simd4f_get_y(matrix.value.y), 
            simd4f_get_y(matrix.value.z), 
            simd4f_get_y(matrix.value.w), 
            simd4f_get_z(matrix.value.x), 
            simd4f_get_z(matrix.value.y), 
            simd4f_get_z(matrix.value.z), 
            simd4f_get_z(matrix.value.w), 
            simd4f_get_w(matrix.value.x), 
            simd4f_get_w(matrix.value.y), 
            simd4f_get_w(matrix.value.z), 
            simd4f_get_w(matrix.value.w) );
    }

    void CalculateFrustumPlanes( const mat4f & clipMatrix, Frustum & frustum )
    {
        float clipData[16];

        clipMatrix.store( clipData );

        #define clip(i,j) clipData[j+i*4]

        const vec3f left( clip(0,3) + clip(0,0),
                          clip(1,3) + clip(1,0),
                          clip(2,3) + clip(2,0) );
        
        const vec3f right( clip(0,3) - clip(0,0),
                           clip(1,3) - clip(1,0),
                           clip(2,3) - clip(2,0) );

        const vec3f bottom( clip(0,3) + clip(0,1),
                            clip(1,3) + clip(1,1),
                            clip(2,3) + clip(2,1) );

        const vec3f top( clip(0,3) - clip(0,1),
                         clip(1,3) - clip(1,1),
                         clip(2,3) - clip(2,1) );

        const vec3f front( clip(0,3) + clip(0,2),
                           clip(1,3) + clip(1,2),
                           clip(2,3) + clip(2,2) );

        const vec3f back( clip(0,3) - clip(0,2),
                          clip(1,3) - clip(1,2),
                          clip(2,3) - clip(2,2) );

        const float left_d = - ( clip(3,3) + clip(3,0) );
        const float right_d = - ( clip(3,3) - clip(3,0) );

        const float bottom_d = - ( clip(3,3) + clip(3,1) );
        const float top_d = - ( clip(3,3) - clip(3,1) );

        const float front_d = - ( clip(3,3) + clip(3,2) );
        const float back_d = - ( clip(3,3) - clip(3,2) );

        const float left_length = length( left );
        const float right_length = length( right );
        const float top_length = length( top );
        const float bottom_length = length( bottom );
        const float front_length = length( front );
        const float back_length = length( back );

        frustum.left = vec4f( left.x(),
                              left.y(),
                              left.z(),
                              left_d ) * 1.0f / left_length;

        frustum.right = vec4f( right.x(),
                               right.y(),
                               right.z(),
                               right_d ) * 1.0f / right_length;

        frustum.top = vec4f( top.x(),
                             top.y(),
                             top.z(),
                             top_d ) * 1.0f / top_length;

        frustum.bottom = vec4f( bottom.x(),
                                bottom.y(),
                                bottom.z(),
                                bottom_d ) * 1.0f / bottom_length;

        frustum.front = vec4f( front.x(),
                               front.y(),
                               front.z(),
                               front_d ) * 1.0f / front_length;

        frustum.back = vec4f( back.x(),
                              back.y(),
                              back.z(),
                              back_d ) * 1.0f / back_length;
    }
}
