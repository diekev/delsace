#version 330 core

layout(location = 0) in vec3 sommets;

uniform mat4 MVP;
uniform mat4 matrice;
uniform vec3 offset;
uniform vec3 dimension;

smooth out vec3 UV;

void main()
{
	gl_Position = MVP * matrice * vec4(sommets.xyz, 1.0);
	UV = (sommets - offset) / dimension;
}

