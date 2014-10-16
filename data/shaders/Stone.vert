#version 410

out vec4 Color;

layout ( location = 0 )  in vec3 VertexPosition;
layout ( location = 1 )  in vec3 VertexNormal;
layout ( location = 2 )  in mat4 MVP;
layout ( location = 6 )  in mat3 NormalMatrix;
layout ( location = 10 ) in mat4 ModelViewMatrix;
layout ( location = 14 ) in vec4 LightPosition;

uniform vec3 Kd = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Ld = vec3( 1.0, 1.0, 1.0 );
uniform vec3 Ambient = vec3( 0.25, 0.25, 1.0 );

void main()
{
    vec3 Normal = mat3( ModelViewMatrix ) * VertexNormal;
    
    vec4 Position = ModelViewMatrix * vec4( VertexPosition, 1.0 );
    
    vec3 LightDirection = normalize( vec3( LightPosition - Position ) );

    Color = vec4( Ambient + Ld * Kd * max( dot( LightDirection, Normal ), 0.0 ), 1.0 );

    gl_Position = MVP * vec4( VertexPosition, 1.0 );
}
