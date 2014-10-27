#version 410

in vec3 Position;
in vec3 Normal;
in vec3 BaseColor;
in vec3 SpecularColor;
in float Metallic;          // [0,1]
in float Roughness;         // [0,1]
in float SpecularPower;     // [0,~8k]

uniform vec3 EyePosition;
uniform vec3 LightPosition;
uniform vec3 LightIntensity = vec3( 1, 1, 1 );
uniform vec3 Ks = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Kd = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Ld = vec3( 0.5, 0.5, 0.5 );
uniform vec3 Ka = vec3( 0.2, 0.2, 0.2 );

layout( location = 0 ) out vec4 FragColor;

uniform samplerCube CubeMap;

/*
float3 FresnelSchlick(float3 SpecularColor,float3 E,float3 H)
{
    return SpecularColor + (1.0f - SpecularColor) * pow(1.0f - saturate(dot(E, H)), 5);
}

FresnelSchlick(SpecularColor, L, H) * ((SpecularPower + 2) / 8 ) * pow(saturate(dot(N, H)), SpecularPower) * dotNL;
*/

vec4 tonemap_none( vec3 color )
{
    return vec4( color, 1 );
}

vec4 tonemap_reinhardt( vec3 color )
{
    color *= 16.0;
    color = color / ( vec3(1,1,1) + color );
    return vec4( color, 1 );
}

void main()
{
    vec3 n = normalize( Normal );

    vec3 e = Position - EyePosition;

    vec3 t = reflect( e, n );

    vec3 s = normalize( LightPosition - Position );

    vec3 v = normalize( e );

    vec3 r = reflect( s, n );

    float SpecularIntensity = pow( max( dot(r,v), 0.0 ), SpecularPower );

    vec3 CubeMapColor = texture( CubeMap, vec3( t.x, -t.z, t.y ) ).rgb;

    vec3 NonMetalColor = BaseColor * ( LightIntensity * ( Ka + Kd * max( dot(s,n), 0.0 ) ) )
                          + LightIntensity * SpecularIntensity;

    vec3 MetalColor = SpecularColor * ( SpecularIntensity + CubeMapColor );

    FragColor = tonemap_none( mix( NonMetalColor, MetalColor, Metallic ) );
}
