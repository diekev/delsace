#version 330 core

out vec2 vVertex;
smooth out vec2 vUV;

void main()
{
	gl_Position = vec4(vVertex * 2.0 - 1.0, 0.0, 1.0);
	vUV = vVertex;
}
