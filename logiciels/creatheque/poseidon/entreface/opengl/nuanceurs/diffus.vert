#version 330 core

layout(location = 0) in vec3 sommets;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 uvs;

uniform mat4 MVP;
uniform mat4 matrice;
uniform mat3 N;

smooth out vec4 couleur_fragment;
smooth out vec3 nor;
smooth out vec3 UV;

void main()
{
	/* clipspace vertex position */
	gl_Position = MVP * matrice * vec4(sommets.xyz, 1.0);

	nor = normalize(N * normal);
	UV = uvs;
}
