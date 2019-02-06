#version 330 core

layout(location = 0) out vec4 couleur_sortie;

smooth in vec3 normal;
smooth in vec3 sommet;
smooth in vec3 couleur;
smooth in vec2 UV;
smooth in mat4 mat;

uniform sampler2D image;
uniform mat4 MV;
uniform mat4 P;
uniform vec3 direction_camera;
uniform int methode;
uniform vec2 taille_texture;
uniform int possede_uvs;

vec4 projection_planaire(vec3 pos, int axe)
{
	vec2 uv;

	if (axe == 0) {
		uv = vec2(pos.y, pos.z);
	}

	if (axe == 1) {
		uv = vec2(pos.x, pos.z);
	}

	if (axe == 2) {
		uv = vec2(pos.x, pos.y);
	}

	return texture2D(image, uv * taille_texture);
}

vec4 projection_triplanaire(vec3 pos)
{
	float angle_xy = abs(normal.z);
	float angle_xz = abs(normal.y);
	float angle_yz = abs(normal.x);
	float poids = angle_xy + angle_xz + angle_yz;

	if (poids == 0.0) {
		return vec4(0.0, 0.0, 0.0, 1.0);
	}

	vec2 uv_xy = vec2(pos.x, pos.y);
	vec2 uv_xz = vec2(pos.x, pos.z);
	vec2 uv_yz = vec2(pos.y, pos.z);

	vec4 couleur_xy = texture2D(image, uv_xy * taille_texture);
	vec4 couleur_xz = texture2D(image, uv_xz * taille_texture);
	vec4 couleur_yz = texture2D(image, uv_yz * taille_texture);

	return (angle_xy * couleur_xy + angle_xz * couleur_xz + angle_yz * couleur_yz) / poids;
}

vec4 projection_cubique(vec3 pos)
{
	return vec4(1.0, 0.0, 1.0, 1.0);
}

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

vec4 projection_sherique(vec3 pos)
{
	float theta = theta_spherique(pos);
	float phi = phi_spherique(pos);
	float t = theta * INV_PI;
	float s = phi * INV_2PI;

	return texture2D(image, vec2(s, t));
}

vec4 projection_cylindrique(vec3 pos)
{
	return vec4(1.0, 0.0, 1.0, 1.0);
}

vec3 unproject(in vec3 pos, in mat4 MV, in mat4 P, in vec4 fenetre)
{
	mat4 I = inverse(P * MV);

	vec4 tmp = vec4(pos, 1.0);
	tmp.x = (tmp.x - fenetre.x) / fenetre.z;
	tmp.y = (tmp.y - fenetre.y) / fenetre.w;
	tmp = tmp * 2 - 1;

	vec4 obj = I * tmp;
	obj /= obj.w;

	return obj.xyz;
}

vec3 project(in vec3 obj, in mat4 model, in mat4 proj, in vec4 fenetre)
{
	vec4 tmp = vec4(obj, 1.0);
	tmp = model * tmp;
	tmp = proj * tmp;

	tmp /= tmp.w;
	tmp = tmp * 0.5 + 0.5;
	tmp.x = tmp.x * fenetre.z + fenetre.x;
	tmp.y = tmp.y * fenetre.w + fenetre.y;

	return tmp.xyz;
}

vec4 projection_camera(vec3 pos)
{
//	if (dot(direction_camera, normal) > 0.0) {
//		return vec4(0.0, 0.0, 0.0, 1.0);
//	}

	vec3 position = vec3(pos.x, pos.y, pos.z);
	vec4 fenetre = vec4(0, 0, 1.0, 1.0);

	vec3 uv = project(position, MV, P, fenetre);
	return texture2D(image, vec2(uv.x, 1.0 - uv.y) * taille_texture);
}

vec4 projection_uv(vec3 pos)
{
	return texture2D(image, UV * taille_texture);
}

void main()
{
	if (methode == 0) {
		couleur_sortie = projection_planaire(sommet, 0);
	}
	else if (methode == 1) {
		couleur_sortie = projection_triplanaire(sommet);
	}
	else if (methode == 2) {
		couleur_sortie = projection_camera((mat * vec4(sommet, 1.0)).xyz);
	}
	else if (methode == 3) {
		couleur_sortie = projection_cubique(sommet);
	}
	else if (methode == 4) {
		couleur_sortie = projection_cylindrique(sommet);
	}
	else if (methode == 5) {
		couleur_sortie = projection_sherique(sommet);
	}
	else if (methode == 6) {
		couleur_sortie = projection_uv(sommet);
	}
	else {
		couleur_sortie = vec4(couleur, 1.0);
	}
}
