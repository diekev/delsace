#version 330 core

layout(location = 0) out vec4 couleur_sortie;

smooth in vec4 couleur_fragment;
smooth in vec3 sommet;

uniform sampler2D image;

float TAU = 6.283185307;
float INV_PI = 0.318309886;
float INV_2PI = 0.159154943;

float theta_spherique(in vec3 co)
{
	return acos(clamp(co.y, -1.0, 1.0));
}

float phi_spherique(in vec3 co)
{
	float p = atan(co.x, co.z);
	return (p < 0.0) ? p + TAU : p;
}

void main()
{
	float theta = theta_spherique(sommet / 100.0);
	float phi = phi_spherique(sommet / 100.0);
	float t = theta * INV_PI;
	float s = phi * INV_2PI;

	vec2 co = vec2(s, t);
	couleur_sortie = vec4(texture2D(image, co).rgb, 1.0);
}
