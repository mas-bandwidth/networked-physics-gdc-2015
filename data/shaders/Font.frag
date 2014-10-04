#version 410

in vec2 TexCoord;
flat in vec4 TextColor;

layout( location = 0 ) out vec4 FragColor;

uniform sampler2D Tex1;

void main() 
{
    vec4 textureColor = texture( Tex1, TexCoord );
    FragColor = vec4( TextColor.r, TextColor.g, TextColor.b, TextColor.a * textureColor.r );
}
