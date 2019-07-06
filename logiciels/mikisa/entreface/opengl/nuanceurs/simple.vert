#version 330 core
  
layout(location = 0) in vec3 sommets;
layout(location = 1) in vec3 couleur_sommet;

uniform mat4 MVP;
uniform mat4 matrice;
uniform bool possede_couleur_sommet;

uniform vec4 couleur;

smooth out vec4 couleur_fragment;
smooth out vec3 sommet;

void main()
{
	gl_Position = MVP * matrice * vec4(sommets.xyz, 1.0);

	sommet = sommets.xyz;

	if (possede_couleur_sommet) {
		couleur_fragment = vec4(couleur_sommet, 1.0);
	}
	else {
		couleur_fragment = couleur;
	}
}
