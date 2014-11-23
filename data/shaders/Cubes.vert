#version 410

in mat4 Model;
in mat4 ModelView;
in mat4 ModelViewProjection;
in vec3 VertexPosition;
in vec3 VertexNormal;
in vec4 VertexColor;

out vec3 Position;
out vec3 Normal;
out vec4 Color;

void main()
{
    Normal = mat3( Model ) * VertexNormal;
    Position = vec3( Model * vec4( VertexPosition, 1.0 ) );
    Color = VertexColor;
    gl_Position = ModelViewProjection * vec4( VertexPosition, 1.0 );
}
