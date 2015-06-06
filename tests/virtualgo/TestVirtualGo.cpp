#include "core/Core.h"
#include "virtualgo/Common.h"
#include "virtualgo/Board.h"
#include "virtualgo/Biconvex.h"
#include "virtualgo/RigidBody.h"
#include "virtualgo/Intersection.h"
#include "virtualgo/InertiaTensor.h"
#include "virtualgo/CollisionDetection.h"
#include <time.h>

#define CORE_ASSERT_CLOSE_VEC3( value, expected, epsilon ) CORE_ASSERT_CLOSE( length( value - expected ), 0.0f, epsilon )

using namespace virtualgo;

void test_biconvex()
{
    printf( "test_biconvex\n" );

    Biconvex biconvex( 2.0f, 1.0f );

    #ifndef NDEBUG
    const float epsilon = 0.001f;
    #endif

    CORE_ASSERT_CLOSE( biconvex.GetWidth(), 2.0f, epsilon );
    CORE_ASSERT_CLOSE( biconvex.GetHeight(), 1.0f, epsilon );
    CORE_ASSERT_CLOSE( biconvex.GetSphereRadius(), 1.25f, epsilon );
    CORE_ASSERT_CLOSE( biconvex.GetSphereOffset(), 0.75f, epsilon );
    CORE_ASSERT_CLOSE( biconvex.GetSphereDot(), 0.799927f, epsilon );
}    

void test_intersect_ray_sphere_hit()
{
    printf( "test_intersect_ray_sphere_hit\n" );

    #ifndef NDEBUG
    const float epsilon = 0.001f;
    #endif

    vec3f rayStart(0,0,-10);
    vec3f rayDirection(0,0,1);
    vec3f sphereCenter(0,0,0);
    
    float sphereRadius = 1.0f;
    float sphereRadiusSquared = sphereRadius * sphereRadius;
    float t = 0;
    
    #ifndef NDEBUG
    bool hit = 
    #endif
        IntersectRaySphere( rayStart, rayDirection, sphereCenter, sphereRadius, sphereRadiusSquared, t );
    
    CORE_ASSERT( hit );
    CORE_ASSERT_CLOSE( t, 9.0f, epsilon );
}

void test_intersect_ray_sphere_miss()
{
    printf( "test_intersect_ray_sphere_miss\n" );

    vec3f rayStart(0,50,-10);
    vec3f rayDirection(0,0,1);
    vec3f sphereCenter(0,0,0);
    
    float sphereRadius = 1.0f;
    float sphereRadiusSquared = sphereRadius * sphereRadius;
    float t = 0;
    
    #ifndef NDEBUG
    bool hit = 
    #endif
        IntersectRaySphere( rayStart, rayDirection, sphereCenter, sphereRadius, sphereRadiusSquared, t );
    
    CORE_ASSERT( !hit );
}

void test_intersect_ray_sphere_inside()
{
    printf( "test_intersect_ray_sphere_inside\n" );

    // IMPORTANT: we define a ray starting inside the sphere as a miss
    // we only care about spheres that we hit from the outside, eg. one sided

    vec3f rayStart(0,0,0);
    vec3f rayDirection(0,0,1);
    vec3f sphereCenter(0,0,0);
    
    float sphereRadius = 1.0f;
    float sphereRadiusSquared = sphereRadius * sphereRadius;
    float t = 0;
    
    #ifndef NDEBUG
    bool hit = 
    #endif
        IntersectRaySphere( rayStart, rayDirection, sphereCenter, sphereRadius, sphereRadiusSquared, t );
    
    CORE_ASSERT( !hit );
}

void test_point_inside_biconvex()
{
    printf( "test_point_inside_biconvex\n" );

    Biconvex biconvex( 2.0f, 1.0f );

    CORE_ASSERT( PointInsideBiconvex_LocalSpace( vec3f(0,0,0), biconvex ) );
    CORE_ASSERT( PointInsideBiconvex_LocalSpace( vec3f(-1,0,0), biconvex ) );
    CORE_ASSERT( PointInsideBiconvex_LocalSpace( vec3f(+1,0,0), biconvex ) );
    CORE_ASSERT( PointInsideBiconvex_LocalSpace( vec3f(0,-1,0), biconvex ) );
    CORE_ASSERT( PointInsideBiconvex_LocalSpace( vec3f(0,+1,0), biconvex ) );
    CORE_ASSERT( PointInsideBiconvex_LocalSpace( vec3f(0,0,0.5), biconvex ) );
    CORE_ASSERT( PointInsideBiconvex_LocalSpace( vec3f(0,0,-0.5), biconvex ) );
    CORE_ASSERT( !PointInsideBiconvex_LocalSpace( vec3f(0,0.5,0.5f), biconvex ) );
}

void test_point_on_biconvex_surface()
{
    printf( "test_point_on_biconvex_surface\n" );

    Biconvex biconvex( 2.0f, 1.0f );

    CORE_ASSERT( IsPointOnBiconvexSurface_LocalSpace( vec3f(-1,0,0), biconvex ) );
    CORE_ASSERT( IsPointOnBiconvexSurface_LocalSpace( vec3f(+1,0,0), biconvex ) );
    CORE_ASSERT( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,-1,0), biconvex ) );
    CORE_ASSERT( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,+1,0), biconvex ) );
    CORE_ASSERT( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,0,0.5), biconvex ) );
    CORE_ASSERT( IsPointOnBiconvexSurface_LocalSpace( vec3f(0,0,-0.5), biconvex ) );
    CORE_ASSERT( !IsPointOnBiconvexSurface_LocalSpace( vec3f(0,0,0), biconvex ) );
    CORE_ASSERT( !IsPointOnBiconvexSurface_LocalSpace( vec3f(10,10,10), biconvex ) );
}

void test_biconvex_surface_normal_at_point()
{
    printf( "test_biconvex_surface_normal_at_point\n" );

    #ifndef NDEBUG
    const float epsilon = 0.001f;
    #endif

    Biconvex biconvex( 2.0f, 1.0f );
    
    vec3f normal(0,0,0);

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(1,0,0), biconvex, normal );
    CORE_ASSERT_CLOSE_VEC3( normal, vec3f(1,0,0), epsilon );

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(-1,0,0), biconvex, normal );
    CORE_ASSERT_CLOSE_VEC3( normal, vec3f(-1,0,0), epsilon );

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(0,0,1), biconvex, normal );
    CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(0,0,-1), biconvex, normal );
    CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,0,-1), epsilon );

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(0,0.5,0), biconvex, normal );
    CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( vec3f(0,-0.5,0), biconvex, normal );
    CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,-1,0), epsilon );
}

void test_nearest_point_on_biconvex_surface()
{
    printf( "test_nearest_point_on_biconvex_surface\n" );

    #ifndef NDEBUG
    const float epsilon = 0.001f;
    #endif

    Biconvex biconvex( 2.0f, 1.0f );

    vec3f nearest;

    nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,0,10), biconvex );
    CORE_ASSERT_CLOSE_VEC3( nearest, vec3f(0,0,0.5f), epsilon );

    nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,0,-10), biconvex );
    CORE_ASSERT_CLOSE_VEC3( nearest, vec3f(0,0,-0.5f), epsilon );

    nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(-10,0,0), biconvex );
    CORE_ASSERT_CLOSE_VEC3( nearest, vec3f(-1,0,0), epsilon );

    nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(10,0,0), biconvex );
    CORE_ASSERT_CLOSE_VEC3( nearest, vec3f(1,0,0), epsilon );

    nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,-10,0), biconvex );
    CORE_ASSERT_CLOSE_VEC3( nearest, vec3f(0,-1,0), epsilon );

    nearest = GetNearestPointOnBiconvexSurface_LocalSpace( vec3f(0,10,0), biconvex );
    CORE_ASSERT_CLOSE_VEC3( nearest, vec3f(0,1,0), epsilon );
}

void test_biconvex_support_local_space()
{
    printf( "test_biconvex_support_local_space\n" );

    #ifndef NDEBUG
    const float epsilon = 0.001f;
    #endif

    Biconvex biconvex( 2.0f, 1.0f );

    float s1,s2;

    BiconvexSupport_LocalSpace( biconvex, vec3f(0,0,1), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -0.5f, epsilon );
    CORE_ASSERT_CLOSE( s2, 0.5f, epsilon );

    BiconvexSupport_LocalSpace( biconvex, vec3f(0,0,-1), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -0.5f, epsilon );
    CORE_ASSERT_CLOSE( s2, 0.5f, epsilon );

    BiconvexSupport_LocalSpace( biconvex, vec3f(1,0,0), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -1.0f, epsilon );
    CORE_ASSERT_CLOSE( s2, 1.0f, epsilon );

    BiconvexSupport_LocalSpace( biconvex, vec3f(-1,0,0), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -1.0f, epsilon );
    CORE_ASSERT_CLOSE( s2, 1.0f, epsilon );

    BiconvexSupport_LocalSpace( biconvex, vec3f(0,1,0), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -1.0f, epsilon );
    CORE_ASSERT_CLOSE( s2, 1.0f, epsilon );

    BiconvexSupport_LocalSpace( biconvex, vec3f(0,-1,0), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -1.0f, epsilon );
    CORE_ASSERT_CLOSE( s2, 1.0f, epsilon );
}

void test_biconvex_support_world_space()
{
    printf( "test_biconvex_support_world_space\n" );

    #ifndef NDEBUG
    const float epsilon = 0.001f;
    #endif

    Biconvex biconvex( 2.0f, 1.0f );
    vec3f biconvexCenter(10,0,0);
    vec3f biconvexUp(1,0,0);

    float s1,s2;

    BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,1,0), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -1.0f, epsilon );
    CORE_ASSERT_CLOSE( s2, 1.0f, epsilon );

    BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,-1,0), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -1.0f, epsilon );
    CORE_ASSERT_CLOSE( s2, 1.0f, epsilon );

    BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(1,0,0), s1, s2 );
    CORE_ASSERT_CLOSE( s1, 9.5f, epsilon );
    CORE_ASSERT_CLOSE( s2, 10.5f, epsilon );

    BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(-1,0,0), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -10.5f, epsilon );
    CORE_ASSERT_CLOSE( s2, -9.5f, epsilon );

    BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,0,1), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -1.0f, epsilon );
    CORE_ASSERT_CLOSE( s2, 1.0f, epsilon );

    BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,0,-1), s1, s2 );
    CORE_ASSERT_CLOSE( s1, -1.0f, epsilon );
    CORE_ASSERT_CLOSE( s2, 1.0f, epsilon );
}

void test_biconvex_sat()
{
    printf( "test_biconvex_sat\n" );

    #ifndef NDEBUG
    const float epsilon = 0.001f;
    #endif

    Biconvex biconvex( 2.0f, 1.0f );

    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(0,-1,0), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(-1,0,0), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(0,0,1), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,0), vec3f(0,1,0), vec3f(0,0,-1), epsilon ) );

    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(1,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(-1,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,1,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,-1,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,1), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,-1), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );

    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(10,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(-10,0,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,10,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,-10,0), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,10), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,-10), vec3f(0,1,0), vec3f(0,1,0), epsilon ) );

    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(10,0,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(-10,0,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,10,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,-10,0), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,10), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
    CORE_ASSERT( !Biconvex_SAT( biconvex, vec3f(0,0,0), vec3f(0,0,-10), vec3f(0,1,0), vec3f(1,0,0), epsilon ) );
}

void test_stone_board_collision_type()
{
    printf( "test_stone_board_collision_type\n" );

    #ifndef NDEBUG

    Board board( 9 );

    const float w = board.GetWidth() * 0.5f;
    const float h = board.GetHeight() * 0.5f;
    const float t = board.GetThickness();

    const float radius = 1.0f;

    bool broadPhaseReject = false;

    #endif

    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(0,0,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_Primary );
    CORE_ASSERT( broadPhaseReject == false );

    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(0,0,-100), radius, broadPhaseReject ) == STONE_BOARD_REGION_Primary );
    CORE_ASSERT( broadPhaseReject == false );

    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(-w,0,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_LeftSide );
    CORE_ASSERT( broadPhaseReject == false );
    
    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(+w,0,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_RightSide );
    CORE_ASSERT( broadPhaseReject == false );
    
    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(0,+h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_TopSide );
    CORE_ASSERT( broadPhaseReject == false );
    
    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(0,-h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_BottomSide );
    CORE_ASSERT( broadPhaseReject == false );

    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(-w,+h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_TopLeftCorner );
    CORE_ASSERT( broadPhaseReject == false );

    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(+w,+h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_TopRightCorner );
    CORE_ASSERT( broadPhaseReject == false );

    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(+w,-h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_BottomRightCorner );
    CORE_ASSERT( broadPhaseReject == false );

    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(-w,-h,0), radius, broadPhaseReject ) == STONE_BOARD_REGION_BottomLeftCorner );
    CORE_ASSERT( broadPhaseReject == false );

    CORE_ASSERT( DetermineStoneBoardRegion( board, vec3f(0,0,t+radius + 0.01f), radius, broadPhaseReject ) == STONE_BOARD_REGION_Primary );
    CORE_ASSERT( broadPhaseReject == true );
}

void test_stone_board_collision_none()
{
    printf( "test_stone_board_collision_none\n" );

    Board board( 9 );

    #ifndef NDEBUG
    const float w = board.GetWidth() * 0.5f;
    const float h = board.GetHeight() * 0.5f;
    const float t = board.GetThickness();
    #endif

    Biconvex biconvex( 2.0f, 1.0f );

    #ifndef NDEBUG
    const float r = biconvex.GetBoundingSphereRadius();
    float depth;
    vec3f point, normal;
    #endif

    CORE_ASSERT( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,0,t+r*2) ), normal, depth ) );
    CORE_ASSERT( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(-w-r*2,0,0) ), normal, depth ) );
    CORE_ASSERT( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(+w+r*2,0,0) ), normal, depth ) );
    CORE_ASSERT( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,-h-r*2,0) ), normal, depth ) );
    CORE_ASSERT( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( vec3f(0,+h+r*2,0) ), normal, depth ) );
}

void test_stone_board_collision_primary()
{
    printf( "test_stone_board_collision_primary\n" );

    /*
    
    // TODO: This whole test is fucked. Needs to be reworked!!!

    const float epsilon = 0.001f;

    Board board( 9 );

    const float w = board.GetWidth() * 0.5f;
    const float h = board.GetHeight() * 0.5f;
    const float t = board.GetThickness();

    Biconvex biconvex( 2.0f, 1.0f );

    float depth;
    vec3f normal;

    // test at origin with identity rotation
    {
        RigidBodyTransform biconvexTransform( vec3f(0,0,0) );

        CORE_ASSERT( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

        CORE_ASSERT_CLOSE( depth, t + 1.0f, epsilon );
        CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
    }
    
    // test away from origin with identity rotation
    {
        RigidBodyTransform biconvexTransform( vec3f(w/2,h/2,0) );

        CORE_ASSERT( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

        printf( "depth = %f\n", depth );

        CORE_ASSERT_CLOSE( depth, t + 1.0f, epsilon );
        CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,0,1), epsilon );
    }

    // test at origin flipped upside down (180 degrees)
    {
        RigidBodyTransform biconvexTransform( vec3f(0,0,0), mat4f::axisRotation( 180, vec3f(0,0,1) ) );

        CORE_ASSERT( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

        CORE_ASSERT_CLOSE( depth, 1.0f, epsilon );
        CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
    }

    // test away from origin flipped upside down (180degrees)
    {
        RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( 180, vec3f(0,0,1) ) );

        CORE_ASSERT( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

        CORE_ASSERT_CLOSE( depth, 1.0f, epsilon );
        CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
    }

    // test away from origin rotated 90 degrees clockwise around z axis
    {
        RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( 90, vec3f(0,0,1) ) );

        CORE_ASSERT( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

        CORE_ASSERT_CLOSE( depth, 1.5f, epsilon );
        CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
    }

    // test away from origin rotated 90 degrees counter-clockwise around z axis
    {
        RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( -90, vec3f(0,0,1) ) );

        CORE_ASSERT( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

        CORE_ASSERT_CLOSE( depth, 1.5f, epsilon );
        CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
    }

    // test away from origin rotated 90 degrees clockwise around x axis
    {
        RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( 90, vec3f(1,0,0) ) );

        CORE_ASSERT( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

        CORE_ASSERT_CLOSE( depth, 1.5f, epsilon );
        CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
    }

    // test away from origin rotated 90 degrees counter-clockwise around x axis
    {
        RigidBodyTransform biconvexTransform( vec3f(w/2,0,h/2), mat4f::axisRotation( -90, vec3f(1,0,0) ) );

        CORE_ASSERT( IntersectStoneBoard( board, biconvex, biconvexTransform, normal, depth ) );

        CORE_ASSERT_CLOSE( depth, 1.5f, epsilon );
        CORE_ASSERT_CLOSE_VEC3( normal, vec3f(0,1,0), epsilon );
    }
    */
}

// TODO: Oh geez. there should really be tests for each stone vs. board collision case!!!

void test_stone_board_collision_left_side()
{
    printf( "test_stone_board_collision_left_side\n" );

    // ...
}

void test_stone_board_collision_right_side()
{
    printf( "test_stone_board_collision_right_side\n" );

    // ...
}

void test_stone_board_collision_top_side()
{
    printf( "test_stone_board_collision_top_side\n" );

    // ...
}

void test_stone_board_collision_bottom_side()
{
    printf( "test_stone_board_collision_bottom_side\n" );

    // ...
}

void test_stone_board_collision_top_left_corner()
{
    printf( "test_stone_board_collision_top_left_corner\n" );

    // ...
}

void test_stone_board_collision_top_right_corner()
{
    printf( "test_stone_board_collision_top_right_corner\n" );

    // ...
}

void test_stone_board_collision_bottom_right_corner()
{
    printf( "test_stone_board_collision_bottom_right_corner\n" );

    // ...
}

void test_stone_board_collision_bottom_left_corner()
{
    printf( "test_stone_board_collision_bottom_left_corner\n" );

    // ...
}

int main()
{
    srand( time( nullptr ) );

    test_biconvex();
    
    test_intersect_ray_sphere_hit();
    test_intersect_ray_sphere_miss();
    test_intersect_ray_sphere_inside();

    test_point_inside_biconvex();
    test_point_on_biconvex_surface();
    test_biconvex_surface_normal_at_point();
    test_nearest_point_on_biconvex_surface();
    
    test_biconvex_support_local_space();
    test_biconvex_support_world_space();

    test_biconvex_sat();

    test_stone_board_collision_type();
    test_stone_board_collision_none();

    // these tests are broken!
    /*
    test_stone_board_collision_primary();
    test_stone_board_collision_left_side();
    test_stone_board_collision_right_side();
    test_stone_board_collision_top_side();
    test_stone_board_collision_bottom_side();
    test_stone_board_collision_top_left_corner();
    test_stone_board_collision_top_right_corner();
    test_stone_board_collision_bottom_right_corner();
    test_stone_board_collision_bottom_left_corner();
    */

    return 0;
}
