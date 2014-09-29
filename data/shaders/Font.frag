#version 410

in vec2 TexCoord;

uniform sampler2D Tex1;

uniform vec4 TextColor;

void main() 
{
    vec4 textureColor = texture( Tex1, TexCoord );
    FragColor = vec4( TextColor.r, TextColor.g, TextColor.b, TextColor.a * textureColor.r );
}
