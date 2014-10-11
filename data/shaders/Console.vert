#version 410

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec4 VertexColor;

flat out vec4 Color;

uniform mat4 ModelViewProjection;

void main()
{
    Color = VertexColor;
    gl_Position = ModelViewProjection * vec4( VertexPosition, 1.0 );
}
