#version 410

flat in vec4 BaseColor;

layout( location = 0 ) out vec4 FragColor;

void main()
{
    FragColor = BaseColor;
}
