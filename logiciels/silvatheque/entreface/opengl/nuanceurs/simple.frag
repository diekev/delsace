#version 330 core

layout(location = 0) out vec4 couleur_sortie;

smooth in vec4 couleur_fragment;

void main()
{
	couleur_sortie = couleur_fragment;
}
