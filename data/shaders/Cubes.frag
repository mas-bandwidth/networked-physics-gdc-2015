#version 410

in vec3 Position;
in vec3 Normal;
in vec4 Color;

uniform vec3 EyePosition;
uniform vec3 LightPosition;
uniform vec3 LightIntensity = vec3( 1, 1, 1 );
uniform vec3 Ks = vec3( 0.15, 0.15, 0.15 );
uniform vec3 Kd = vec3( 0.6, 0.6, 0.6 );
uniform vec3 Ka = vec3( 0.2, 0.2, 0.2 );
uniform float SpecularPower = 1.0;

layout( location = 0 ) out vec4 FragColor;

void main()
{
    vec3 n = normalize( Normal );

    vec3 e = Position - EyePosition;

    vec3 t = reflect( e, n );

    vec3 s = normalize( LightPosition - Position );

    vec3 v = normalize( e );

    vec3 r = reflect( s, n );

    float SpecularIntensity = pow( max( dot(r,v), 0.0 ), SpecularPower );

    FragColor = vec4( Color.rgb * ( LightIntensity * ( Ka + Kd * max( dot(s,n), 0.0 ) ) ) + 
                                  ( LightIntensity * SpecularIntensity * Ks ), 
                      Color.a );
}
