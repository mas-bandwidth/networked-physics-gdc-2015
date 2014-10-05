#ifndef INTERSECTION_H
#define INTERSECTION_H

#include "Board.h"
#include <float.h>

inline bool IntersectRayPlane( vec3f rayStart,
                               vec3f rayDirection,
                               vec3f planeNormal,
                               float planeDistance,
                               float & t,
                               float epsilon = 0.001f )
{
    // IMPORTANT: we only consider intersections *in front* the ray start, eg. t >= 0
    const float d = dot( rayDirection, planeNormal );
    if ( d > -epsilon )
        return false;
    t = - ( dot( rayStart, planeNormal ) - planeDistance ) / d;
    assert( t >= 0 );
    return true;
}

inline bool IntersectRaySphere( vec3f rayStart,
                                vec3f rayDirection, 
                                vec3f sphereCenter, 
                                float sphereRadius, 
                                float sphereRadiusSquared, 
                                float & t )
{
    vec3f delta = sphereCenter - rayStart;
    const float distanceSquared = dot( delta, delta );
    const float timeClosest = dot( delta, rayDirection );
    if ( timeClosest < 0 )
        return false;                   // ray points away from sphere
    const float timeHalfChordSquared = sphereRadiusSquared - distanceSquared + timeClosest*timeClosest;
    if ( timeHalfChordSquared < 0 )
        return false;                   // ray misses sphere
    t = timeClosest - sqrt( timeHalfChordSquared );
    if ( t < 0 )
        return false;                   // ray started inside sphere. we only want one sided collisions from outside of sphere
    return true;
}

inline bool IntersectRayBiconvex_LocalSpace( vec3f rayStart, 
                                             vec3f rayDirection, 
                                             const Biconvex & biconvex,
                                             float & t, 
                                             vec3f & point, 
                                             vec3f & normal )
{
    const float sphereOffset = biconvex.GetSphereOffset();
    const float sphereRadius = biconvex.GetSphereRadius();
    const float sphereRadiusSquared = biconvex.GetSphereRadiusSquared();

    // intersect ray with bottom sphere
    vec3f bottomSphereCenter( 0, 0, -sphereOffset );
    if ( IntersectRaySphere( rayStart, rayDirection, bottomSphereCenter, sphereRadius, sphereRadiusSquared, t ) )
    {
        point = rayStart + rayDirection * t;
        if ( point.z() >= 0 )
        {
            normal = normalize( point - bottomSphereCenter );
            return true;
        }        
    }

    // intersect ray with top sphere
    vec3f topSphereCenter( 0, 0, sphereOffset );
    if ( IntersectRaySphere( rayStart, rayDirection, topSphereCenter, sphereRadius, sphereRadiusSquared, t ) )
    {
        point = rayStart + rayDirection * t;
        if ( point.z() <= 0 )
        {
            normal = normalize( point - topSphereCenter );
            return true;
        }
    }

    return false;
}

inline float IntersectRayStone( const Biconvex & biconvex, 
                                const RigidBodyTransform & biconvexTransform,
                                vec3f rayStart, 
                                vec3f rayDirection, 
                                vec3f & point, 
                                vec3f & normal )
{
    vec3f local_rayStart = TransformPoint( biconvexTransform.worldToLocal, rayStart );
    vec3f local_rayDirection = TransformVector( biconvexTransform.worldToLocal, rayDirection );
    
    vec3f local_point, local_normal;

    float t;

    bool result = IntersectRayBiconvex_LocalSpace( local_rayStart, 
                                                   local_rayDirection,
                                                   biconvex,
                                                   t,
                                                   local_point,
                                                   local_normal );

    if ( result )
    {
        point = TransformPoint( biconvexTransform.localToWorld, local_point );
        normal = TransformVector( biconvexTransform.localToWorld, local_normal );
        return t;
    }

    return -1;
}

inline float IntersectRayBoard( const Board & board,
                                vec3f rayStart,
                                vec3f rayDirection,
                                vec3f & point,
                                vec3f & normal,
                                float epsilon = 0.001f )
{ 
    // first check with the primary surface
    // statistically speaking this is the most likely
    {
        float t;
        if ( IntersectRayPlane( rayStart, rayDirection, vec3f(0,0,1), 0, t, epsilon ) )
        {
            point = rayStart + rayDirection * t;
            normal = vec3f(0,0,1);
            const float w = board.GetHalfWidth();
            const float h = board.GetHalfHeight();
            const float px = point.x();
            const float pz = point.z();
            if ( px >= -w && px <= w && pz >= -h && pz <= h )
                return t;
        }
    }

    // todo: other cases
    // left side, right side, top side, bottom side

    return -1;
}

enum BoardEdges
{
    BOARD_EDGE_None = 0,
    BOARD_EDGE_Left = 1,
    BOARD_EDGE_Top = 2,
    BOARD_EDGE_Right = 4,
    BOARD_EDGE_Bottom = 8
};

enum StoneBoardRegion
{
    STONE_BOARD_REGION_Primary = BOARD_EDGE_None,     // common case: collision with the primary surface (the plane at y = 0)
    STONE_BOARD_REGION_LeftSide = BOARD_EDGE_Left,
    STONE_BOARD_REGION_TopSide = BOARD_EDGE_Top,
    STONE_BOARD_REGION_RightSide = BOARD_EDGE_Right,
    STONE_BOARD_REGION_BottomSide = BOARD_EDGE_Bottom,
    STONE_BOARD_REGION_TopLeftCorner = BOARD_EDGE_Top | BOARD_EDGE_Left,
    STONE_BOARD_REGION_TopRightCorner = BOARD_EDGE_Top | BOARD_EDGE_Right,
    STONE_BOARD_REGION_BottomRightCorner = BOARD_EDGE_Bottom | BOARD_EDGE_Right,
    STONE_BOARD_REGION_BottomLeftCorner = BOARD_EDGE_Bottom | BOARD_EDGE_Left
};

inline StoneBoardRegion DetermineStoneBoardRegion( const Board & board, vec3f position, float radius, bool & broadPhaseReject )
{
    const float thickness = board.GetThickness();
    
    const float x = position.x();
    const float y = position.y();
    const float z = position.z();

    const float w = board.GetHalfWidth();
    const float h = board.GetHalfHeight();
    const float r = radius;

    uint32_t edges = BOARD_EDGE_None;

    if ( x <= -w + r )                            // IMPORTANT: assumption that the board width/height is large 
        edges |= BOARD_EDGE_Left;                 // relative to the bounding sphere, eg. that only one corner
    else if ( x >= w - r )                        // would potentially be intersecting with a stone at any time
        edges |= BOARD_EDGE_Right;

    if ( y >= h - r )
        edges |= BOARD_EDGE_Top;
    else if ( y <= -h + r )
        edges |= BOARD_EDGE_Bottom;

    broadPhaseReject = ( z > thickness + radius ) || x < -w - r || x > w + r || y < -h - r || y > h + r;

    return (StoneBoardRegion) edges;
}

struct Axis
{
    vec3f normal;
    float d;
    float s1,s2;
};

inline bool IntersectStoneBoard( const Board & board, 
                                 const Biconvex & biconvex, 
                                 const RigidBodyTransform & biconvexTransform,
                                 vec3f & normal,
                                 float & depth,
                                 bool hasPreferredDirection = false,
                                 const vec3f & preferredDirection = vec3f(0,0,1),
                                 float epsilon = 0.0001f )
{
    const float boundingSphereRadius = biconvex.GetBoundingSphereRadius();

    vec3f biconvexPosition;
    biconvexTransform.GetPosition( biconvexPosition );

    bool broadPhaseReject;
    StoneBoardRegion region = DetermineStoneBoardRegion( board, biconvexPosition, boundingSphereRadius, broadPhaseReject );
    if ( broadPhaseReject )
        return false;

    const int MaxAxes = 7;

    int numAxes = 0;
    Axis axis[MaxAxes];

    vec3f biconvexUp;
    biconvexTransform.GetUp( biconvexUp );

    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    if ( region == STONE_BOARD_REGION_Primary )
    {
        numAxes = 1;
        axis[0].d = t;
        axis[0].normal = vec3f(0,0,1);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[0].normal, axis[0].s1, axis[0].s2 );
    }
    else if ( region == STONE_BOARD_REGION_LeftSide )
    {
        numAxes = 3;

        // primary
        axis[0].d = t;
        axis[0].normal = vec3f(0,0,1);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[0].normal, axis[0].s1, axis[0].s2 );

        // side
        axis[1].d = w;
        axis[1].normal = vec3f(-1,0,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[1].normal, axis[1].s1, axis[1].s2 );

        // edge
        axis[2].normal = vec3f( -0.70710,0,+0.70710 );
        axis[2].d = dot( vec3f( -w, 0, t ), axis[2].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[2].normal, axis[2].s1, axis[2].s2 );
    }
    else if ( region == STONE_BOARD_REGION_RightSide )
    {
        numAxes = 3;

        // primary
        axis[0].d = t;
        axis[0].normal = vec3f(0,0,1);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[0].normal, axis[0].s1, axis[0].s2 );

        // side
        axis[1].d = w;
        axis[1].normal = vec3f(1,0,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[1].normal, axis[1].s1, axis[1].s2 );

        // edge
        axis[2].normal = vec3f( +0.70710,0,+0.70710 );
        axis[2].d = dot( vec3f( w, 0, t ), axis[2].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[2].normal, axis[2].s1, axis[2].s2 );
    }
    else if ( region == STONE_BOARD_REGION_TopSide )
    {
        numAxes = 3;

        // primary
        axis[0].d = t;
        axis[0].normal = vec3f(0,0,1);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[0].normal, axis[0].s1, axis[0].s2 );

        // side
        axis[1].d = h;
        axis[1].normal = vec3f(0,1,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[1].normal, axis[1].s1, axis[1].s2 );

        // edge
        axis[2].normal = vec3f( 0,+0.70710,+0.70710 );
        axis[2].d = dot( vec3f( 0, h, t ), axis[2].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[2].normal, axis[2].s1, axis[2].s2 );
    }
    else if ( region == STONE_BOARD_REGION_BottomSide )
    {
        numAxes = 3;

        // primary
        axis[0].d = t;
        axis[0].normal = vec3f(0,0,1);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[0].normal, axis[0].s1, axis[0].s2 );

        // side
        axis[1].d = h;
        axis[1].normal = vec3f(0,-1,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[1].normal, axis[1].s1, axis[1].s2 );

        // edge
        axis[2].normal = vec3f( 0,-0.70710,+0.70710 );
        axis[2].d = dot( vec3f( 0, -h, t ), axis[2].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[2].normal, axis[2].s1, axis[2].s2 );
    }
    else if ( region == STONE_BOARD_REGION_BottomLeftCorner )
    {
        numAxes = 7;

        // primary
        axis[0].d = t;
        axis[0].normal = vec3f(0,0,1);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[0].normal, axis[0].s1, axis[0].s2 );

        // left side
        axis[1].d = w;
        axis[1].normal = vec3f(-1,0,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[1].normal, axis[1].s1, axis[1].s2 );

        // left edge
        axis[2].normal = vec3f( -0.70710,0,+0.70710 );
        axis[2].d = dot( vec3f( -w, -h, t ), axis[2].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[2].normal, axis[2].s1, axis[2].s2 );

        // bottom side
        axis[3].d = h;
        axis[3].normal = vec3f(0,-1,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[3].normal, axis[3].s1, axis[3].s2 );

        // bottom edge
        axis[4].normal = vec3f( 0,-0.70710,+0.70710 );
        axis[4].d = dot( vec3f( -w, -h, t ), axis[4].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[4].normal, axis[4].s1, axis[4].s2 );

        // bottom-left corner edge (vertical)
        axis[5].normal = vec3f( -0.70710,-0.70710, 0 );
        axis[5].d = dot( vec3f( -w, -h, t ), axis[5].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[5].normal, axis[5].s1, axis[5].s2 );

        // bottom-left corner
        axis[6].normal = vec3f( -0.577271, -0.577271, 0.577271 );
        axis[6].d = dot( vec3f( -w, -h, t ), axis[6].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[6].normal, axis[6].s1, axis[6].s2 );
    }
    else if ( region == STONE_BOARD_REGION_BottomRightCorner )
    {
        numAxes = 7;

        // primary
        axis[0].d = t;
        axis[0].normal = vec3f(0,0,1);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[0].normal, axis[0].s1, axis[0].s2 );

        // right side
        axis[1].d = w;
        axis[1].normal = vec3f(1,0,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[1].normal, axis[1].s1, axis[1].s2 );

        // right edge
        axis[2].normal = vec3f( +0.70710,0,+0.70710 );
        axis[2].d = dot( vec3f( w, 0, t ), axis[2].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[2].normal, axis[2].s1, axis[2].s2 );

        // bottom side
        axis[3].d = h;
        axis[3].normal = vec3f(0,-1,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[3].normal, axis[3].s1, axis[3].s2 );

        // bottom edge
        axis[4].normal = vec3f( 0,-0.70710,+0.70710 );
        axis[4].d = dot( vec3f( 0, -h, t ), axis[4].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[4].normal, axis[4].s1, axis[4].s2 );

        // bottom-right corner edge (vertical)
        axis[5].normal = vec3f( 0.70710,-0.70710, 0 );
        axis[5].d = dot( vec3f( w, -h, t ), axis[5].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[5].normal, axis[5].s1, axis[5].s2 );

        // bottom-right corner
        axis[6].normal = vec3f( 0.577271, -0.577271, 0.577271 );
        axis[6].d = dot( vec3f( w, -h, t ), axis[6].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[6].normal, axis[6].s1, axis[6].s2 );
    }
    else if ( region == STONE_BOARD_REGION_TopLeftCorner )
    {
        numAxes = 7;

        // primary
        axis[0].d = t;
        axis[0].normal = vec3f(0,0,1);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[0].normal, axis[0].s1, axis[0].s2 );

        // left side
        axis[1].d = w;
        axis[1].normal = vec3f(-1,0,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[1].normal, axis[1].s1, axis[1].s2 );

        // left edge
        axis[2].normal = vec3f( -0.70710,0,+0.70710 );
        axis[2].d = dot( vec3f( -w, 0, t ), axis[2].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[2].normal, axis[2].s1, axis[2].s2 );

        // top side
        axis[3].d = h;
        axis[3].normal = vec3f(0,1,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[3].normal, axis[3].s1, axis[3].s2 );

        // top edge
        axis[4].normal = vec3f( 0,0.70710,+0.70710 );
        axis[4].d = dot( vec3f( 0, h, t ), axis[4].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[4].normal, axis[4].s1, axis[4].s2 );

        // top-left corner edge (vertical)
        axis[5].normal = vec3f( -0.70710,0.70710,0 );
        axis[5].d = dot( vec3f( -w, h, t ), axis[5].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[5].normal, axis[5].s1, axis[5].s2 );

        // top-left corner
        axis[6].normal = vec3f( -0.577271, 0.577271, 0.577271 );
        axis[6].d = dot( vec3f( -w, h, t ), axis[6].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[6].normal, axis[6].s1, axis[6].s2 );
    }
    else if ( region == STONE_BOARD_REGION_TopRightCorner )
    {
        numAxes = 7;

        // primary
        axis[0].d = t;
        axis[0].normal = vec3f(0,0,1);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[0].normal, axis[0].s1, axis[0].s2 );

        // right side
        axis[1].d = w;
        axis[1].normal = vec3f(1,0,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[1].normal, axis[1].s1, axis[1].s2 );

        // right edge
        axis[2].normal = vec3f( 0.70710,0,+0.70710 );
        axis[2].d = dot( vec3f( w, 0, t ), axis[2].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[2].normal, axis[2].s1, axis[2].s2 );

        // top side
        axis[3].d = h;
        axis[3].normal = vec3f(0,1,0);
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[3].normal, axis[3].s1, axis[3].s2 );

        // top edge
        axis[4].normal = vec3f( 0,0.70710,0.70710 );
        axis[4].d = dot( vec3f( 0, h, t ), axis[4].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[4].normal, axis[4].s1, axis[4].s2 );

        // top-left corner edge (vertical)
        axis[5].normal = vec3f( 0.70710,0.70710,0 );
        axis[5].d = dot( vec3f( w, h, t ), axis[5].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[5].normal, axis[5].s1, axis[5].s2 );

        // top-right corner
        axis[6].normal = vec3f( 0.577271, 0.577271, 0.577271 );
        axis[6].d = dot( vec3f( w, h, t ), axis[6].normal );
        BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, axis[6].normal, axis[6].s1, axis[6].s2 );
    }

    // not colliding if no axes defined
    if ( numAxes == 0 )
        return false;

    // not colliding if any axis separates the stone and the board
    for ( int i = 0; i < numAxes; ++i )
    {
        if ( axis[i].s1 > axis[i].d )
            return false;
    }

    Axis * selectedAxis = NULL;
    float penetrationDepth = 0;

    if ( !hasPreferredDirection )
    {
        // colliding: find axis with the least amount of penetration
        penetrationDepth = FLT_MAX;
        for ( int i = 0; i < numAxes; ++i )
        {
            const float depth = axis[i].d - axis[i].s1;
            if ( depth < penetrationDepth )
            {
                penetrationDepth = depth;
                selectedAxis = &axis[i];
            }
        }
    }
    else
    {
        // push out along axis most in line with preferred direction
        float preferredDot = -FLT_MAX;
        for ( int i = 0; i < numAxes; ++i )
        {
            const float currentDot = dot( axis[i].normal, preferredDirection );
            if ( currentDot >= preferredDot )
            {
                preferredDot = currentDot;
                selectedAxis = &axis[i];
                penetrationDepth = axis[i].d - axis[i].s1;
            }
        }
    }

    assert( selectedAxis );
    normal = selectedAxis->normal;
    depth = penetrationDepth;

    return true;
}

#endif
