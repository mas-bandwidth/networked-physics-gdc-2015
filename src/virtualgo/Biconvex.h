#ifndef VIRTUALGO_BICONVEX_H
#define VIRTUALGO_BICONVEX_H

#include "vectorial/vec3f.h"

namespace virtualgo
{
    using namespace vectorial;

    /*
        Biconvex solid.

        The biconvex solid is the intersection of two equally sized spheres

        The two spheres of specific radius are placed vertically relative to 
        each other so that their intersection generate a biconvex solid with
        the desired width (xy circle intersection) and height (z axis top to bottom).
    */

    class Biconvex
    {
    public:

        Biconvex();

        Biconvex( float width, float height, float bevel = 0 );

        float GetWidth() const { return width; }
        float GetHeight() const { return height; }
        float GetBevel() const { return bevel; }

        float GetSphereRadius() const { return sphereRadius; }
        float GetSphereRadiusSquared() const { return sphereRadiusSquared; }
        float GetSphereOffset() const { return sphereOffset; }
        float GetSphereDot() const { return sphereDot; }

        float GetBoundingSphereRadius() const { return boundingSphereRadius; }

        float GetCircleRadius() const { return circleRadius; }

        float GetBevelCircleRadius() const { return bevelCircleRadius; }
        float GetBevelTorusMajorRadius() const { return bevelTorusMajorRadius; }
        float GetBevelTorusMinorRadius() const { return bevelTorusMinorRadius; }

        float GetRealWidth() const { return realWidth; }

    private:

        float width;                            // width of biconvex solid
        float height;                           // height of biconvex solid
        float bevel;                            // height of bevel measured vertically looking at the stone side-on

        float sphereRadius;                     // radius of spheres that intersect to generate this biconvex solid
        float sphereRadiusSquared;              // radius squared
        float sphereOffset;                     // vertical offset from biconvex origin to center of spheres
        float sphereDot;                        // dot product threshold for detecting circle edge vs. sphere surface collision

        float circleRadius;                     // the radius of the circle edge at the intersection of the spheres surfaces

        float boundingSphereRadius;             // bounding sphere radius for biconvex shape
        float boundingSphereRadiusSquared;      // bounding sphere radius squared

        float bevelCircleRadius;                // radius of circle on sphere at start of bevel. if no bevel then equal to circle radius
        float bevelTorusMajorRadius;            // the major radius of the torus generating the bevel
        float bevelTorusMinorRadius;            // the minor radius of the torus generating the bevel

        float realWidth;                        // the real width of the biconvex, taking the bevel into consideration.
    };

    bool PointInsideBiconvex_LocalSpace( vec3f point, const Biconvex & biconvex, float epsilon = 0.001f );

    bool IsPointOnBiconvexSurface_LocalSpace( vec3f point, const Biconvex & biconvex, float epsilon = 0.001f );

    void GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f point, const Biconvex & biconvex, vec3f & normal, float epsilon = 0.001 );

    vec3f GetNearestPointOnBiconvexSurface_LocalSpace( vec3f point, const Biconvex & biconvex, float epsilon = 0.001f );

    void BiconvexSupport_LocalSpace( const Biconvex & biconvex, vec3f axis, float & s1, float & s2 );

    void BiconvexSupport_WorldSpace( const Biconvex & biconvex, vec3f biconvexCenter, vec3f biconvexUp, vec3f axis, float & s1, float & s2 );

    void GetNearestPoint_Biconvex_Line( const Biconvex & biconvex, 
                                        vec3f biconvexCenter,
                                        vec3f biconvexUp,
                                        vec3f lineOrigin,
                                        vec3f lineDirection,
                                        vec3f & biconvexPoint,
                                        vec3f & linePoint );

    #define TEST_BICONVEX_AXIS( name, axis )                                            \
    {                                                                                   \
        float s1,s2,t1,t2;                                                              \
        BiconvexSupport_WorldSpace( biconvex, position_a, up_a, axis, s1, s2 );         \
        BiconvexSupport_WorldSpace( biconvex, position_b, up_b, axis, t1, t2 );         \
        if ( s2 + epsilon < t1 || t2 + epsilon < s1 )                                   \
            return false;                                                               \
    }

    /*
    #define TEST_BICONVEX_AXIS( name, axis )                                            \
    {                                                                                   \
        printf( "-----------------------------------\n" );                              \
        printf( name ":\n" );                                                           \
        printf( "-----------------------------------\n" );                              \
        float s1,s2,t1,t2;                                                              \
        BiconvexSupport_WorldSpace( biconvex, position_a, up_a, axis, s1, s2 );         \
        BiconvexSupport_WorldSpace( biconvex, position_b, up_b, axis, t1, t2 );         \
        printf( "(%f,%f) | (%f,%f)\n", s1, s2, t1, t2 );                                \
        if ( s2 + epsilon < t1 || t2 + epsilon < s1 )                                   \
        {                                                                               \
            printf( "not intersecting\n" );                                             \
            return false;                                                               \
        }                                                                               \
    }
    */

    inline bool Biconvex_SAT( const Biconvex & biconvex,
                              vec3f position_a,
                              vec3f position_b,
                              vec3f up_a,
                              vec3f up_b,
                              float epsilon = 0.001f )
    {
        const float sphereOffset = biconvex.GetSphereOffset();

        vec3f top_a = position_a + up_a * sphereOffset;
        vec3f top_b = position_b + up_b * sphereOffset;

        vec3f bottom_a = position_a - up_a * sphereOffset;
        vec3f bottom_b = position_b - up_b * sphereOffset;

        TEST_BICONVEX_AXIS( "primary", normalize( position_b - position_a ) );
        TEST_BICONVEX_AXIS( "top_a|top_b", normalize( top_b - top_a ) );
        TEST_BICONVEX_AXIS( "top_a|bottom_b", normalize( bottom_b - top_a ) );
        TEST_BICONVEX_AXIS( "bottom_a|top_b", normalize( top_b - bottom_a ) );
        TEST_BICONVEX_AXIS( "bottom_b|top_a", normalize( bottom_b - bottom_a ) );

        return true;
    }
}

#endif // #ifndef VIRTUALGO_BICONVEX_H
