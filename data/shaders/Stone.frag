#version 410

in vec3 Position;
in vec3 Normal;

uniform vec4 LightPosition;
uniform vec3 LightIntensity = vec3( 1, 1, 1 );
uniform vec3 Ks = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Kd = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Ld = vec3( 1.0, 1.0, 1.0 );
uniform vec3 Ka = vec3( 0.4, 0.4, 0.4 );
uniform float Shininess = 50.0;

layout( location = 0 ) out vec4 FragColor;

void main()
{
    vec3 n = normalize( Normal );

    vec3 vp = Position;

    vec3 s = normalize( vec3( LightPosition ) - vp );

    vec3 v = normalize( -vp );

    vec3 r = reflect( -s, n );

    FragColor = vec4( LightIntensity * ( Ka + Kd * max( dot(s,n), 0.0 ) + Ks * pow( max( dot(r,v), 0.0 ), Shininess ) ), 1 );
}
