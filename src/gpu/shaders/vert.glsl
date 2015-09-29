#version 330 core

layout(location = 0) in vec2 vertex;
smooth out vec2 UV;

void main()
{
	gl_Position = vec4(vertex * 2.0 - 1.0, 0.0, 1.0);
	UV = vertex;
}
