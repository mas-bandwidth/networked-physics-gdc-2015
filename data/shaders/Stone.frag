#version 410

in vec3 Position;
in vec3 Normal;

uniform vec3 EyePosition;
uniform vec3 LightPosition = vec3( 0.0, 0.0, 10.0 );
uniform vec3 LightIntensity = vec3( 1, 1, 1 );
uniform vec3 Ks = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Kd = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Ld = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Ka = vec3( 0.2, 0.2, 0.2 );
uniform float Shininess = 50.0;

layout( location = 0 ) out vec4 FragColor;

layout ( location = 14 ) in vec4 BaseColor;
layout ( location = 15 ) in float Metallic;
layout ( location = 16 ) in float Specular;
layout ( location = 17 ) in float Roughness;

uniform samplerCube CubeMap;

void main()
{
    vec3 n = normalize( Normal );

    // ---------

    vec3 e = Position - EyePosition;

    vec3 t = reflect( e, n );

    vec4 cubemap_color = texture( CubeMap, vec3( t.x, -t.z, t.y ) );

    // ------

    vec3 s = normalize( LightPosition - Position );

    vec3 v = normalize( e );

    vec3 r = reflect( -s, n );

    vec4 lighting_color = vec4( LightIntensity * ( Ka + Kd * max( dot(s,n), 0.0 ) + Ks * pow( max( dot(r,v), 0.0 ), Shininess ) ), 1 );

    // ------

    FragColor = cubemap_color * lighting_color;
}
