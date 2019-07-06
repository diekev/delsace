#version 330 core

layout(location = 0) out vec4 couleur_sortie;

smooth in vec3 nor;
smooth in vec3 sommet;

uniform sampler2D texture_poly;

uniform float taille_u;
uniform float taille_v;

void main()
{
	vec2 UV_xy = vec2(sommet.x * taille_u, sommet.y * taille_v);
	vec2 UV_xz = vec2(sommet.x * taille_u, sommet.z * taille_v);
	vec2 UV_yz = vec2(sommet.y * taille_u, sommet.z * taille_v);

	vec4 couleur_xy = texture2D(texture_poly, UV_xy);
	vec4 couleur_xz = texture2D(texture_poly, UV_xz);
	vec4 couleur_yz = texture2D(texture_poly, UV_yz);

	float angle_xy = dot(abs(nor), vec3(0.0, 0.0, 1.0));
	float angle_xz = dot(abs(nor), vec3(0.0, 1.0, 0.0));
	float angle_yz = dot(abs(nor), vec3(1.0, 0.0, 0.0));

	float poids = (angle_xy + angle_xz + angle_yz);

	if (poids == 0.0) {
		poids = 1.0;
	}

	vec4 couleur = (angle_xy * couleur_xy + angle_xz * couleur_xz + angle_yz * couleur_yz) / poids;

	couleur_sortie = couleur;
}
