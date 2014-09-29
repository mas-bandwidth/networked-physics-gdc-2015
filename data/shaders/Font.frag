#version 410

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D Tex1;

uniform float TextColor;

void main() 
{
    vec4 textureColor = texture( Tex1, TexCoord );
    FragColor = vec4( TextColor.r, TextColor.g, TextColor.b, textureColor.r );
}
