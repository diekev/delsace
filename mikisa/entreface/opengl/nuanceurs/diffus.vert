#version 330 core

layout(location = 0) in vec3 sommets;
layout(location = 1) in vec3 normaux;
layout(location = 2) in vec2 uvs;

uniform mat4 MVP;
uniform mat4 matrice;

smooth out vec3 normal;
smooth out vec3 sommet;
smooth out vec2 UV;
smooth out mat4 mat;

void main()
{
	vec4 s = vec4(sommets.xyz, 1.0);
	gl_Position = MVP * matrice * s;
	normal = normaux;
	sommet = s.xyz;
	UV = uvs;
	mat = matrice;
}
