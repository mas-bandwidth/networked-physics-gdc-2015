#version 410

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec4 VertexTextColor;
layout (location = 2) in vec2 VertexTexCoord;

out vec2 TexCoord;
flat out vec4 TextColor;

uniform mat4 ModelViewProjection;

void main()
{
    TexCoord = VertexTexCoord;
    TextColor = VertexTextColor;
    gl_Position = ModelViewProjection * vec4( VertexPosition + vec3( 0.1, 0.1, 0 ), 1.0 );
}
