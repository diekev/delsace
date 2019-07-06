#version 330 core

layout(location = 0) in vec3 sommets;
layout(location = 1) in vec3 normal;

uniform mat4 MVP;
uniform mat4 matrice;

uniform vec4 couleur;

smooth out vec3 sommet;
smooth out vec3 nor;
smooth out vec4 couleur_fragment;

void main()
{
	/* clipspace vertex position */
	gl_Position = MVP * matrice * vec4(sommets.xyz, 1.0);
	sommet = (sommets + vec3(1.0)) * 0.5;
	nor = normal;
}
