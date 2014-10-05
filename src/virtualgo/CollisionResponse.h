#ifndef COLLISION_RESPONSE_H
#define COLLISION_RESPONSE_H

#include "RigidBody.h"
#include "CollisionDetection.h"

void ApplyLinearCollisionImpulse( StaticContact & contact, float e )
{
    vec3f velocityAtPoint;
    contact.rigidBody->GetVelocityAtWorldPoint( contact.point, velocityAtPoint );
    const float k = contact.rigidBody->inverseMass;
    const float j = max( - ( 1 + e ) * dot( velocityAtPoint, contact.normal ) / k, 0 );
    contact.rigidBody->linearMomentum += j * contact.normal;
}

void ApplyCollisionImpulseWithFriction( StaticContact & contact, float e, float u, float epsilon = 0.001f )
{
	RigidBody & rigidBody = *contact.rigidBody;

    vec3f velocityAtPoint;
    rigidBody.GetVelocityAtWorldPoint( contact.point, velocityAtPoint );

    const float vn = min( 0, dot( velocityAtPoint, contact.normal ) );

    const mat4f & i = rigidBody.inverseInertiaTensorWorld;

    // apply collision impulse

    const vec3f r = contact.point - rigidBody.position;

    const float k = rigidBody.inverseMass + 
                    dot( cross( r, contact.normal ), 
                         transformVector( i, cross( r, contact.normal ) ) );

    const float j = - ( 1 + e ) * vn / k;

    rigidBody.linearMomentum += j * contact.normal;
    rigidBody.angularMomentum += j * cross( r, contact.normal );

    // apply friction impulse

    rigidBody.GetVelocityAtWorldPoint( contact.point, velocityAtPoint );

    vec3f tangentVelocity = velocityAtPoint - contact.normal * dot( velocityAtPoint, contact.normal );

    if ( length_squared( tangentVelocity ) > epsilon * epsilon )
    {
        vec3f tangent = normalize( tangentVelocity );

        const float vt = dot( velocityAtPoint, tangent );

        const float kt = rigidBody.inverseMass + 
                         dot( cross( r, tangent ), 
                              transformVector( i, cross( r, tangent ) ) );

        const float jt = clamp( -vt / kt, -u * j, u * j );

        rigidBody.linearMomentum += jt * tangent;
        rigidBody.angularMomentum += jt * cross( r, tangent );
    }
}

#endif
