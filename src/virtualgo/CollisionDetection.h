#ifndef COLLISION_DETECTION_H
#define COLLISION_DETECTION_H

#include "Board.h"
#include "Biconvex.h"
#include "Intersection.h"
#include <stdint.h>

// --------------------------------------------------------------------------

struct StaticContact
{
    RigidBody * rigidBody;
    vec3f point;
    vec3f normal;
    float depth;
};

struct DynamicContact
{
    RigidBody * a;
    RigidBody * b;
    vec3f point;
    vec3f normal;
};

inline bool StoneBoardCollision( const Biconvex & biconvex,
                                 const Board & board, 
                                 RigidBody & rigidBody,
                                 StaticContact & contact,
                                 bool pushOut = false,
                                 bool hasPreferredDirection = false,
                                 const vec3f & preferredDirection = vec3f(0,0,1) );

inline bool StonePlaneCollision( const Biconvex & biconvex,
                                 const vec4f & plane,
                                 RigidBody & rigidBody,
                                 StaticContact & contact );

// --------------------------------------------------------------------------

inline void ClosestFeaturesBiconvexPlane_LocalSpace( const vec3f & planeNormal,
                                                     float planeDistance,
                                                     const Biconvex & biconvex,
                                                     vec3f & biconvexPoint,
                                                     vec3f & biconvexNormal,
                                                     vec3f & planePoint )
{
    const float sphereDot = biconvex.GetSphereDot();
    const float planeNormalDot = fabs( dot( vec3f(0,0,1), planeNormal ) );
    if ( planeNormalDot > sphereDot )
    {
        // sphere surface collision
        const float sphereRadius = biconvex.GetSphereRadius();
        const float sphereOffset = planeNormal.z() < 0 ? -biconvex.GetSphereOffset() : +biconvex.GetSphereOffset();
        vec3f sphereCenter( 0, 0, sphereOffset );
        biconvexPoint = sphereCenter - normalize( planeNormal ) * sphereRadius;
        biconvexNormal = normalize( biconvexPoint - sphereCenter );
    }
    else
    {
        // circle edge collision
        const float circleRadius = biconvex.GetCircleRadius();
        biconvexPoint = normalize( vec3f( -planeNormal.x(), -planeNormal.y(), 0 ) ) * circleRadius;
        biconvexNormal = normalize( biconvexPoint );
    }

    planePoint = biconvexPoint - planeNormal * ( dot( biconvexPoint, planeNormal ) - planeDistance );
}

inline bool ClosestFeaturePrimarySurface( const Board & board, 
                                          const Biconvex & biconvex, 
                                          const RigidBodyTransform & biconvexTransform,
                                          vec3f & stonePoint,
                                          vec3f & stoneNormal,
                                          vec3f & boardPoint,
                                          vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,0,1,t) );

    vec3f local_stonePoint;
    vec3f local_stoneNormal;
    vec3f local_boardPoint;

    ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), plane.w(), 
                                             biconvex, 
                                             local_stonePoint,
                                             local_stoneNormal,
                                             local_boardPoint );

    stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
    stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
    boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
    boardNormal = vec3f(0,0,1);

    const float x = boardPoint.x();
    const float y = boardPoint.y();

    return x >= -w && x <= w && y >= -h && y <= h;
}

inline bool ClosestFeatureLeftSide( const Board & board, 
                                    const Biconvex & biconvex, 
                                    const RigidBodyTransform & biconvexTransform,
                                    vec3f & stonePoint,
                                    vec3f & stoneNormal,
                                    vec3f & boardPoint,
                                    vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(-1,0,0,w) );

    vec3f local_stonePoint;
    vec3f local_stoneNormal;
    vec3f local_boardPoint;

    ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), plane.w(), 
                                             biconvex, 
                                             local_stonePoint,
                                             local_stoneNormal,
                                             local_boardPoint );

    stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
    stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
    boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
    boardNormal = vec3f(-1,0,0);

    const float y = boardPoint.y();
    const float z = boardPoint.z();

    return z <= t && y >= -h && y <= h;
}

inline bool ClosestFeatureLeftEdge( const Board & board, 
                                    const Biconvex & biconvex, 
                                    const vec3f & biconvexPosition,
                                    const vec3f & biconvexUp,
                                    const RigidBodyTransform & biconvexTransform,
                                    vec3f & stonePoint,
                                    vec3f & stoneNormal,
                                    vec3f & boardPoint,
                                    vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    const vec3f lineOrigin = vec3f( -w, -h, t );
    const vec3f lineDirection = vec3f(0,1,0);

    GetNearestPoint_Biconvex_Line( biconvex,
                                   biconvexPosition,
                                   biconvexUp,
                                   lineOrigin,
                                   lineDirection,
                                   stonePoint,
                                   boardPoint );

    const float x = boardPoint.x();
    const float y = boardPoint.y();
    const float z = boardPoint.z();

    const float dx = fabs( x - (-w) );
    const float dz = fabs( z - t );
    assert( dx < 0.001f );
    assert( dz < 0.001f );

    vec3f local_point = transformPoint( biconvexTransform.worldToLocal, stonePoint );
    vec3f local_normal;

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_point, biconvex, local_normal );

    stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

    boardNormal = -stoneNormal;

    return y >= -h && y <= h;
}

inline bool ClosestFeatureRightSide( const Board & board, 
                                     const Biconvex & biconvex, 
                                     const RigidBodyTransform & biconvexTransform,
                                     vec3f & stonePoint,
                                     vec3f & stoneNormal,
                                     vec3f & boardPoint,
                                     vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(1,0,0,w) );

    vec3f local_stonePoint;
    vec3f local_stoneNormal;
    vec3f local_boardPoint;

    ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), plane.w(), 
                                             biconvex, 
                                             local_stonePoint,
                                             local_stoneNormal,
                                             local_boardPoint );

    stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
    stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
    boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
    boardNormal = vec3f(1,0,0);

    const float y = boardPoint.y();
    const float z = boardPoint.z();

    return z <= t && y >= -h && y <= h;
}

inline bool ClosestFeatureRightEdge( const Board & board, 
                                     const Biconvex & biconvex, 
                                     const vec3f & biconvexPosition,
                                     const vec3f & biconvexUp,
                                     const RigidBodyTransform & biconvexTransform,
                                     vec3f & stonePoint,
                                     vec3f & stoneNormal,
                                     vec3f & boardPoint,
                                     vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    const vec3f lineOrigin = vec3f( w, -h, t );
    const vec3f lineDirection = vec3f(0,1,0);

    GetNearestPoint_Biconvex_Line( biconvex,
                                   biconvexPosition,
                                   biconvexUp,
                                   lineOrigin,
                                   lineDirection,
                                   stonePoint,
                                   boardPoint );

    const float x = boardPoint.x();
    const float y = boardPoint.y();
    const float z = boardPoint.z();

    const float dx = fabs( x - w );
    const float dz = fabs( z - t );
    assert( dx < 0.001f );
    assert( dz < 0.001f );

    vec3f local_point = transformPoint( biconvexTransform.worldToLocal, stonePoint );
    vec3f local_normal;

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_point, biconvex, local_normal );

    stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

    boardNormal = -stoneNormal;

    return y >= -h && y <= h;
}

inline bool ClosestFeatureTopSide( const Board & board, 
                                   const Biconvex & biconvex, 
                                   const RigidBodyTransform & biconvexTransform,
                                   vec3f & stonePoint,
                                   vec3f & stoneNormal,
                                   vec3f & boardPoint,
                                   vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,h) );

    vec3f local_stonePoint;
    vec3f local_stoneNormal;
    vec3f local_boardPoint;

    ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), plane.w(), 
                                             biconvex, 
                                             local_stonePoint,
                                             local_stoneNormal,
                                             local_boardPoint );

    stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
    stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
    boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
    boardNormal = vec3f(0,1,0);

    const float x = boardPoint.x();
    const float z = boardPoint.z();

    return z <= t && x >= -w && x <= w;
}

inline bool ClosestFeatureTopEdge( const Board & board, 
                                   const Biconvex & biconvex, 
                                   const vec3f & biconvexPosition,
                                   const vec3f & biconvexUp,
                                   const RigidBodyTransform & biconvexTransform,
                                   vec3f & stonePoint,
                                   vec3f & stoneNormal,
                                   vec3f & boardPoint,
                                   vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    const vec3f lineOrigin = vec3f( -w, h, t );
    const vec3f lineDirection = vec3f(1,0,0);

    GetNearestPoint_Biconvex_Line( biconvex,
                                   biconvexPosition,
                                   biconvexUp,
                                   lineOrigin,
                                   lineDirection,
                                   stonePoint,
                                   boardPoint );

    const float x = boardPoint.x();
    const float y = boardPoint.y();
    const float z = boardPoint.z();

    const float dy = fabs( y - h );
    const float dz = fabs( z - t );
    assert( dy < 0.001f );
    assert( dz < 0.001f );

    vec3f local_point = transformPoint( biconvexTransform.worldToLocal, stonePoint );
    vec3f local_normal;

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_point, biconvex, local_normal );

    stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

    boardNormal = -stoneNormal;

    return x >= -w && x <= w;
}

inline bool ClosestFeatureBottomSide( const Board & board, 
                                      const Biconvex & biconvex, 
                                      const RigidBodyTransform & biconvexTransform,
                                      vec3f & stonePoint,
                                      vec3f & stoneNormal,
                                      vec3f & boardPoint,
                                      vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,-1,0,h) );

    vec3f local_stonePoint;
    vec3f local_stoneNormal;
    vec3f local_boardPoint;

    ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), plane.w(), 
                                             biconvex, 
                                             local_stonePoint,
                                             local_stoneNormal,
                                             local_boardPoint );

    stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
    stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
    boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
    boardNormal = vec3f(0,-1,0);

    const float x = boardPoint.x();
    const float z = boardPoint.z();

    return z <= t && x >= -w && x <= w;
}

inline bool ClosestFeatureBottomEdge( const Board & board, 
                                      const Biconvex & biconvex, 
                                      const vec3f & biconvexPosition,
                                      const vec3f & biconvexUp,
                                      const RigidBodyTransform & biconvexTransform,
                                      vec3f & stonePoint,
                                      vec3f & stoneNormal,
                                      vec3f & boardPoint,
                                      vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    const vec3f lineOrigin = vec3f( -w, -h, t );
    const vec3f lineDirection = vec3f(1,0,0);

    GetNearestPoint_Biconvex_Line( biconvex,
                                   biconvexPosition,
                                   biconvexUp,
                                   lineOrigin,
                                   lineDirection,
                                   stonePoint,
                                   boardPoint );

    const float x = boardPoint.x();
    const float y = boardPoint.y();
    const float z = boardPoint.z();

    const float dy = fabs( y - (-h) );
    const float dz = fabs( z - t );
    assert( dy < 0.001f );
    assert( dz < 0.001f );

    vec3f local_point = transformPoint( biconvexTransform.worldToLocal, stonePoint );
    vec3f local_normal;

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_point, biconvex, local_normal );

    stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

    boardNormal = -stoneNormal;

    return x >= -w && x <= w;
}

inline bool ClosestFeatureBottomLeftEdge( const Board & board, 
                                          const Biconvex & biconvex, 
                                          const vec3f & biconvexPosition,
                                          const vec3f & biconvexUp,
                                          const RigidBodyTransform & biconvexTransform,
                                          vec3f & stonePoint,
                                          vec3f & stoneNormal,
                                          vec3f & boardPoint,
                                          vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    const vec3f lineOrigin = vec3f( -w, -h, 0 );
    const vec3f lineDirection = vec3f(0,0,1);

    GetNearestPoint_Biconvex_Line( biconvex,
                                   biconvexPosition,
                                   biconvexUp,
                                   lineOrigin,
                                   lineDirection,
                                   stonePoint,
                                   boardPoint );

    const float x = boardPoint.x();
    const float y = boardPoint.y();
    const float z = boardPoint.z();

    const float dx = fabs( x - (-w) );
    const float dy = fabs( y - (-h) );
    assert( dx < 0.001f );
    assert( dy < 0.001f );

    vec3f local_point = transformPoint( biconvexTransform.worldToLocal, stonePoint );
    vec3f local_normal;

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_point, biconvex, local_normal );

    stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

    boardNormal = -stoneNormal;

    return z <= t;
}

inline bool ClosestFeatureBottomRightEdge( const Board & board, 
                                           const Biconvex & biconvex, 
                                           const vec3f & biconvexPosition,
                                           const vec3f & biconvexUp,
                                           const RigidBodyTransform & biconvexTransform,
                                           vec3f & stonePoint,
                                           vec3f & stoneNormal,
                                           vec3f & boardPoint,
                                           vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    const vec3f lineOrigin = vec3f( w, -h, t );
    const vec3f lineDirection = vec3f(0,0,-1);

    GetNearestPoint_Biconvex_Line( biconvex,
                                   biconvexPosition,
                                   biconvexUp,
                                   lineOrigin,
                                   lineDirection,
                                   stonePoint,
                                   boardPoint );

    const float x = boardPoint.x();
    const float y = boardPoint.y();
    const float z = boardPoint.z();

    const float dx = fabs( x - w );
    const float dy = fabs( y - (-h) );
    assert( dx < 0.001f );
    assert( dy < 0.001f );

    vec3f local_point = transformPoint( biconvexTransform.worldToLocal, stonePoint );
    vec3f local_normal;

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_point, biconvex, local_normal );

    stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

    boardNormal = -stoneNormal;

    return z <= t;
}

inline bool ClosestFeatureTopLeftEdge( const Board & board, 
                                       const Biconvex & biconvex, 
                                       const vec3f & biconvexPosition,
                                       const vec3f & biconvexUp,
                                       const RigidBodyTransform & biconvexTransform,
                                       vec3f & stonePoint,
                                       vec3f & stoneNormal,
                                       vec3f & boardPoint,
                                       vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    const vec3f lineOrigin = vec3f( -w, h, t );
    const vec3f lineDirection = vec3f(0,0,-1);

    GetNearestPoint_Biconvex_Line( biconvex,
                                   biconvexPosition,
                                   biconvexUp,
                                   lineOrigin,
                                   lineDirection,
                                   stonePoint,
                                   boardPoint );

    const float x = boardPoint.x();
    const float y = boardPoint.y();
    const float z = boardPoint.z();

    const float dx = fabs( x - (-w) );
    const float dy = fabs( y - h );
    assert( dx < 0.001f );
    assert( dy < 0.001f );

    vec3f local_point = transformPoint( biconvexTransform.worldToLocal, stonePoint );
    vec3f local_normal;

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_point, biconvex, local_normal );

    stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

    boardNormal = -stoneNormal;

    return z <= t;
}

inline bool ClosestFeatureTopRightEdge( const Board & board, 
                                        const Biconvex & biconvex, 
                                        const vec3f & biconvexPosition,
                                        const vec3f & biconvexUp,
                                        const RigidBodyTransform & biconvexTransform,
                                        vec3f & stonePoint,
                                        vec3f & stoneNormal,
                                        vec3f & boardPoint,
                                        vec3f & boardNormal )
{
    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    const vec3f lineOrigin = vec3f( w, h, t );
    const vec3f lineDirection = vec3f(0,0,-1);

    GetNearestPoint_Biconvex_Line( biconvex,
                                   biconvexPosition,
                                   biconvexUp,
                                   lineOrigin,
                                   lineDirection,
                                   stonePoint,
                                   boardPoint );

    const float x = boardPoint.x();
    const float y = boardPoint.y();
    const float z = boardPoint.z();

    const float dx = fabs( x - w );
    const float dy = fabs( y - h );
    assert( dx < 0.001f );
    assert( dy < 0.001f );

    vec3f local_point = transformPoint( biconvexTransform.worldToLocal, stonePoint );
    vec3f local_normal;

    GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_point, biconvex, local_normal );

    stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

    boardNormal = -stoneNormal;

    return z <= t;
}

inline void ClosestFeatureCorner( const Board & board, 
                                  const Biconvex & biconvex, 
                                  const RigidBodyTransform & biconvexTransform,
                                  vec3f cornerPoint,
                                  vec3f & stonePoint,
                                  vec3f & stoneNormal,
                                  vec3f & boardPoint,
                                  vec3f & boardNormal )
{
    vec3f local_corner_point = transformPoint( biconvexTransform.worldToLocal, cornerPoint );

    vec3f local_biconvex_point = 
        GetNearestPointOnBiconvexSurface_LocalSpace( local_corner_point, biconvex );

    vec3f local_normal;
    GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_biconvex_point, biconvex, local_normal );

    stonePoint = transformPoint( biconvexTransform.localToWorld, local_biconvex_point );
    stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

    boardNormal = -stoneNormal;
    boardPoint = cornerPoint;
}

// -------------------------------------------------------------------------------------------

inline void ClosestFeaturesStoneBoard( const Board & board, 
                                       const Biconvex & biconvex, 
                                       const vec3f & biconvexPosition,
                                       const RigidBodyTransform & biconvexTransform,
                                       vec3f & stonePoint,
                                       vec3f & stoneNormal,
                                       vec3f & boardPoint,
                                       vec3f & boardNormal )
{
    const float boundingSphereRadius = biconvex.GetWidth() * 0.5f;

    bool broadPhaseReject;
    StoneBoardRegion region = DetermineStoneBoardRegion( board, biconvexPosition, boundingSphereRadius, broadPhaseReject );

    const float w = board.GetWidth() / 2;
    const float h = board.GetHeight() / 2;
    const float t = board.GetThickness();

    vec3f biconvexUp;
    biconvexTransform.GetUp( biconvexUp );
    
    if ( region == STONE_BOARD_REGION_Primary )
    {
        ClosestFeaturePrimarySurface( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );
    }
    else if ( region == STONE_BOARD_REGION_LeftSide )
    {
        if ( ClosestFeaturePrimarySurface( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureLeftSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        ClosestFeatureLeftEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );
    }
    else if ( region == STONE_BOARD_REGION_RightSide )
    {
        if ( ClosestFeaturePrimarySurface( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureRightSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        ClosestFeatureRightEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );
    }
    else if ( region == STONE_BOARD_REGION_TopSide )
    {
        if ( ClosestFeaturePrimarySurface( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureTopSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        ClosestFeatureTopEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );
    }
    else if ( region == STONE_BOARD_REGION_BottomSide )
    {
        if ( ClosestFeaturePrimarySurface( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureBottomSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        ClosestFeatureBottomEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal );
    }
    else if ( region == STONE_BOARD_REGION_BottomLeftCorner )
    {
        if ( ClosestFeaturePrimarySurface( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureLeftSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureBottomSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureLeftEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureBottomEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureBottomLeftEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        ClosestFeatureCorner( board, biconvex, biconvexTransform, vec3f(-w,-h,t), stonePoint, stoneNormal, boardPoint, boardNormal );
    }
    else if ( region == STONE_BOARD_REGION_BottomRightCorner )
    {
        if ( ClosestFeaturePrimarySurface( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureRightSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureBottomSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureRightEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureBottomEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureBottomRightEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        ClosestFeatureCorner( board, biconvex, biconvexTransform, vec3f(w,-h,t), stonePoint, stoneNormal, boardPoint, boardNormal );
    }
    else if ( region == STONE_BOARD_REGION_TopLeftCorner )
    {
        if ( ClosestFeaturePrimarySurface( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureLeftSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureTopSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureLeftEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureTopEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureTopLeftEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        ClosestFeatureCorner( board, biconvex, biconvexTransform, vec3f(-w,h,t), stonePoint, stoneNormal, boardPoint, boardNormal );
    }
    else if ( region == STONE_BOARD_REGION_TopRightCorner )
    {
        if ( ClosestFeaturePrimarySurface( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureRightSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureTopSide( board, biconvex, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureRightEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureTopEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        if ( ClosestFeatureTopRightEdge( board, biconvex, biconvexPosition, biconvexUp, biconvexTransform, stonePoint, stoneNormal, boardPoint, boardNormal ) )
            return;

        ClosestFeatureCorner( board, biconvex, biconvexTransform, vec3f(w,h,t), stonePoint, stoneNormal, boardPoint, boardNormal );
    }
}

// -----------------------------------------------------------------------

bool StoneBoardCollision( const Biconvex & biconvex,
                          const Board & board, 
                          RigidBody & rigidBody,
                          StaticContact & contact,
                          bool pushOut,
                          bool hasPreferredDirection,
                          const vec3f & preferredDirection )
{
    const RigidBodyTransform & transform = rigidBody.transform;

    // detect collision with the board

    float depth;
    vec3f normal;
    if ( !IntersectStoneBoard( board, biconvex, transform,
                               normal, depth, hasPreferredDirection, preferredDirection ) )
        return false;

    // project the stone out of the board

    if ( pushOut )
        rigidBody.position += normal * depth;

    // fill the contact information for the caller

    vec3f stonePoint, stoneNormal, boardPoint, boardNormal;

    ClosestFeaturesStoneBoard( board, biconvex, rigidBody.position, transform,
                               stonePoint, stoneNormal, boardPoint, boardNormal );

    contact.rigidBody = &rigidBody;
    contact.point = boardPoint;
    contact.normal = boardNormal;
    contact.depth = depth;

    return true;
}

bool StonePlaneCollision( const Biconvex & biconvex,
                          const vec4f & plane,
                          RigidBody & rigidBody,
                          StaticContact & contact )
{
    vec3f planeNormal( plane.value );
    const float planeD = plane.w();
    if ( dot( rigidBody.position, planeNormal ) > planeD + biconvex.GetBoundingSphereRadius() )
        return false;

    const RigidBodyTransform & transform = rigidBody.transform;
    
    const vec3f & biconvexPosition = rigidBody.position;

    float s1,s2;
    vec3f biconvexUp;
    transform.GetUp( biconvexUp );
    BiconvexSupport_WorldSpace( biconvex, biconvexPosition, biconvexUp, planeNormal, s1, s2 );
    
    if ( s1 > planeD )
        return false;

    float depth = planeD - s1;

    rigidBody.position += planeNormal * depth;

    vec4f local_plane = TransformPlane( transform.worldToLocal, plane );

    vec3f local_stonePoint;
    vec3f local_stoneNormal;
    vec3f local_floorPoint;

    ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( local_plane.value ),
                                             local_plane.w(), 
                                             biconvex, 
                                             local_stonePoint,
                                             local_stoneNormal,
                                             local_floorPoint );

    contact.rigidBody = &rigidBody;
    contact.point = TransformPoint( transform.localToWorld, local_floorPoint );
    contact.normal = planeNormal;
    contact.depth = depth;

    return true;
}

// -----------------------------------------------------------------------

#endif
