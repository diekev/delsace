#version 330 core

layout(location = 0) in vec3 sommets;
layout(location = 1) in vec3 normal;

uniform mat4 MVP;
uniform mat4 matrice;
uniform mat3 N;

uniform vec4 couleur;

smooth out vec4 couleur_fragment;
smooth out vec3 sommet;
smooth out vec3 nor;

void main()
{
	/* clipspace vertex position */
	gl_Position = MVP * matrice * vec4(sommets.xyz, 1.0);

	nor = normal;
	sommet = sommets.xyz;
	couleur_fragment = couleur;
}
