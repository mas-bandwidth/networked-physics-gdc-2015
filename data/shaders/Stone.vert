#version 410

out vec4 Color;

layout ( location = 0 ) in vec3 VertexPosition;
layout ( location = 1 ) in vec3 VertexNormal;
layout ( location = 2 ) in mat4 ModelView;
layout ( location = 6 ) in mat4 ModelViewProjection;

uniform vec4 LightPosition;
uniform vec3 Kd = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Ld = vec3( 1.0, 1.0, 1.0 );
uniform vec3 Ambient = vec3( 0.25, 0.25, 1.0 );

void main()
{
    vec3 Normal = normalize( mat3( ModelView ) * VertexNormal );
    
    vec4 Position = ModelView * vec4( VertexPosition, 1.0 );
    
    vec3 LightDirection = normalize( vec3( LightPosition - Position ) );

    Color = vec4( Ambient + Ld * Kd * max( dot( LightDirection, Normal ), 0.0 ), 1.0 );

    gl_Position = ModelViewProjection * vec4( VertexPosition, 1.0 );
}
