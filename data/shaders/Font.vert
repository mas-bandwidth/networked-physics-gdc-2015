#version 410

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec2 VertexTexCoord;

out vec3 Position;
out vec2 TexCoord;

uniform mat4 ModelViewProjection;

void main()
{
    TexCoord = VertexTexCoord;
    gl_Position = ModelViewProject * vec4( VertexPosition, 1.0 );
}
