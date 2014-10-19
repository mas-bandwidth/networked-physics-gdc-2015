#version 410

out vec3 Position;
out vec3 Normal;

layout ( location = 0 ) in vec3 VertexPosition;
layout ( location = 1 ) in vec3 VertexNormal;
layout ( location = 2 ) in mat4 Model;
layout ( location = 6 ) in mat4 ModelView;
layout ( location = 10 ) in mat4 ModelViewProjection;

void main()
{
    Normal = mat3( Model ) * VertexNormal;

    Position = vec3( Model * vec4( VertexPosition, 1.0 ) );

    gl_Position = ModelViewProjection * vec4( VertexPosition, 1.0 );
}
