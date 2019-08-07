#version 330 core

layout(location = 0) out vec4 couleur_sortie;

smooth in vec4 couleur_fragment;
smooth in vec3 nor;
smooth in vec3 sommet;

struct Lumiere {
	vec3 position;
	vec4 couleur;
	int type;
};

#define NOMBRE_LUMIERES 8

uniform Lumiere lumieres[NOMBRE_LUMIERES];

vec4 couleur_ambiante = vec4(0.05, 0.35, 0.8, 1.0);

vec4 evalue_lumiere_point(
        in vec4 albedo_surface,
        in vec3 normal_surface,
        in vec4 couleur_lumiere,
        in vec3 position_lumiere)
{
	float angle = dot(normal_surface, position_lumiere);
	angle = clamp(angle, 0.0, 1.0);

	float distance = length(position_lumiere - sommet);
	distance = 1.0 / (distance * distance);

	float intensite_lumiere = angle * distance;

	vec4 couleur = couleur_lumiere * albedo_surface;
	vec4 couleur_diffuse = vec4(couleur.rgb * vec3(intensite_lumiere), 0.0);

	return couleur_diffuse;
}

vec4 evalue_lumiere_distante(
        in vec4 albedo_surface,
        in vec3 normal_surface,
        in vec4 couleur_lumiere,
        in vec3 position_lumiere)
{
	float angle = dot(normal_surface, position_lumiere);
	angle = clamp(angle, 0.0, 1.0);

	vec4 couleur = couleur_lumiere * albedo_surface;
	vec4 couleur_diffuse = vec4(couleur.rgb * vec3(angle), 0.0);

	return couleur_diffuse;
}

void main()
{
	vec3 normal_surface = normalize(nor);

	vec4 couleur = vec4(0.0, 0.0, 0.0, 1.0);
	vec4 couleur_diffuse = vec4(0.0, 0.0, 0.0, 1.0);

	for (int i = 0; i < NOMBRE_LUMIERES; ++i) {
		if (lumieres[i].type == 0) {
			couleur_diffuse += evalue_lumiere_point(
			            couleur_fragment,
			            normal_surface,
			            lumieres[i].couleur,
			            lumieres[i].position);
		}
		else if (lumieres[i].type == 1) {
			couleur_diffuse += evalue_lumiere_distante(
			            couleur_fragment,
			            normal_surface,
			            lumieres[i].couleur,
			            lumieres[i].position);
		}
	}

	couleur_sortie = couleur_diffuse;
}
