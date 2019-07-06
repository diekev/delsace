#version 330 core

layout(location = 0) out vec4 couleur_sortie;

smooth in vec3 nor;
smooth in vec3 UV;

uniform sampler2DArray texture_poly;

void main()
{
	vec4 couleur = texture(texture_poly, UV);
	vec3 normal_normalise = normalize(nor);
	float w = 0.5 * (1.0 + dot(normal_normalise, vec3(0.0, 1.0, 0.0)));
	vec4 couleur_diffuse = w * couleur + (1.0 - w) * (couleur * 0.3);

	couleur_sortie = couleur_diffuse;
	//couleur_sortie = vec4(UV, couleur_diffuse.a);
}
