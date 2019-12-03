#version 330 core

layout(location = 0) out vec4 couleur_sortie;

smooth in vec4 couleur_fragment;
smooth in vec4 vpos;

void main()
{
	float dist = vpos.z / 100.0;
	couleur_sortie = vec4(couleur_fragment.rgb, 1.0 - dist);
}
