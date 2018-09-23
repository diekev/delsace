/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "creation.h"

#include <math/vec2.h>
#include <math/vec3.h>

#include "adaptrice_creation.h"

namespace objets {

/* ************************************************************************** */

void cree_boite(AdaptriceCreationObjet *adaptrice,
				const float taille_x,
				const float taille_y,
				const float taille_z,
				const float centre_x,
				const float centre_y,
				const float centre_z)
{
#if 0
	/* todo: expose this to the UI */
	const auto &x_div = 2;
	const auto &y_div = 2;
	const auto &z_div = 2;

	const auto size = dimension * uniform_scale;

	const auto &start_x = -(size.x / 2.0f) + center.x;
	const auto &start_y = -(size.y / 2.0f) + center.y;
	const auto &start_z = -(size.z / 2.0f) + center.z;

	const auto &x_increment = size.x / (x_div - 1);
	const auto &y_increment = size.y / (y_div - 1);
	const auto &z_increment = size.z / (z_div - 1);

	for (auto x = 0; x < x_div; ++x) {
		vec[0] = start_x + x * x_increment;

		for (auto y = 0; y < y_div; ++y) {
			vec[1] = start_y + y * y_increment;

			for (auto z = 0; z < z_div; ++z) {
				vec[2] = start_z + z * z_increment;

				points->push_back(vec);
			}
		}
	}
#endif

	numero7::math::vec3f min(-taille_x + centre_x, -taille_y + centre_y, -taille_z + centre_z);
	numero7::math::vec3f max( taille_x + centre_x,  taille_y + centre_y,  taille_z + centre_z);

	const numero7::math::vec3f sommets[8] = {
		numero7::math::vec3f(min[0], min[1], min[2]),
		numero7::math::vec3f(max[0], min[1], min[2]),
		numero7::math::vec3f(max[0], max[1], min[2]),
		numero7::math::vec3f(min[0], max[1], min[2]),
		numero7::math::vec3f(min[0], min[1], max[2]),
		numero7::math::vec3f(max[0], min[1], max[2]),
		numero7::math::vec3f(max[0], max[1], max[2]),
		numero7::math::vec3f(min[0], max[1], max[2])
	};

	adaptrice->reserve_sommets(8);

	for (const auto &sommet : sommets) {
		adaptrice->ajoute_sommet(sommet.x, sommet.y, sommet.z);
	}

	const numero7::math::vec3f normaux[6] = {
		numero7::math::vec3f(-1.0f,  0.0f,  0.0f),
		numero7::math::vec3f( 1.0f,  0.0f,  0.0f),
		numero7::math::vec3f( 0.0f, -1.0f,  0.0f),
		numero7::math::vec3f( 0.0f,  1.0f,  0.0f),
		numero7::math::vec3f( 0.0f,  0.0f, -1.0f),
		numero7::math::vec3f( 0.0f,  0.0f,  1.0f),
	};

	adaptrice->reserve_normaux(6);

	for (const auto &normal : normaux) {
		adaptrice->ajoute_normal(normal.x, normal.y, normal.z);
	}

	const numero7::math::vec2f uvs[4] = {
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
	};

	adaptrice->reserve_uvs(4);

	for (const auto &uv : uvs) {
		adaptrice->ajoute_coord_uv_sommet(uv.x, uv.y);
	}

	const int polygones[6][4] = {
		{0, 4, 7, 3}, // min x
		{5, 1, 2, 6}, // max x
		{1, 5, 4, 0}, // min y
		{3, 7, 6, 2}, // max y
		{1, 0, 3, 2}, // min z
		{4, 5, 6, 7}, // max z
	};

	const int normaux_polygones[6][4] = {
		{0, 0, 0, 0},
		{1, 1, 1, 1},
		{2, 2, 2, 2},
		{3, 3, 3, 3},
		{4, 4, 4, 4},
		{5, 5, 5, 5}
	};

	const int uvs_polygones[4] = {
		0, 1, 2, 3
	};

	adaptrice->reserve_polygones(6);

	for (size_t i = 0; i < 6; ++i) {
		adaptrice->ajoute_polygone(polygones[i], uvs_polygones, normaux_polygones[i], 4);
	}
}

/* ************************************************************************** */

template <typename T>
auto sphere(const T u, const T v, const T rayon)
{
	return numero7::math::vec3<T>(
				std::cos(u) * std::sin(v) * rayon,
				std::cos(v) * rayon,
				std::sin(u) * std::sin(v) * rayon);
}

void cree_sphere_uv(AdaptriceCreationObjet *adaptrice,
					const float rayon,
					const float centre_x,
					const float centre_y,
					const float centre_z)
{
	/* Création d'une sphère UV à partir de son équation paramétrique.
	 * U est la longitude, V la latitude. */
	const auto resolution_u = 48;
	const auto resolution_v = resolution_u / 2;
	const auto debut_u = 0.0f;
	const auto debut_v = 0.0f;
	const auto fin_u = static_cast<float>(M_PI * 2);
	const auto fin_v = static_cast<float>(M_PI);

	/* Taille entre deux points de longitude. */
	const auto taille_pas_u = (fin_u - debut_u) / resolution_u;

	/* Taille entre deux points de latitude. */
	const auto taille_pas_v = (fin_v - debut_v) / resolution_v;

	int poly[4] = { 0, 1, 2, 3 };
	const int decalage = 4;

	const auto &centre = numero7::math::vec3f(centre_x, centre_y, centre_z);

	for (int i = 0; i < resolution_u; i++) {
		for (int j = 0; j < resolution_v; j++) {
			const auto u = i * taille_pas_u + debut_u;
			const auto v = j * taille_pas_v + debut_v;
			const auto un = (i + 1 == resolution_u) ? fin_u : (i + 1) * taille_pas_u + debut_u;
			const auto vn = (j + 1 == resolution_v) ? fin_v : (j + 1) * taille_pas_v + debut_v;

			/* Trouve les quatre points de la grille en évaluant la fonction
			 * paramétrique.
			 *
			 * REMARQUE : pour les sphères, le normal est simplement la version
			 * normalisée du sommet. Ce n'est généralement pas le cas pour
			 * toutes les fonctions paramétriques.
			 */

			const auto p0 = sphere(u, v, rayon) + centre;
			const auto p1 = sphere(un, v, rayon) + centre;
			const auto p2 = sphere(un, vn, rayon) + centre;
			const auto p3 = sphere(u, vn, rayon) + centre;

			adaptrice->ajoute_sommet(p0.x, p0.y, p0.z);
			adaptrice->ajoute_sommet(p1.x, p1.y, p1.z);
			adaptrice->ajoute_sommet(p2.x, p2.y, p2.z);
			adaptrice->ajoute_sommet(p3.x, p3.y, p3.z);

			const auto n0 = normalise(p0);
			const auto n1 = normalise(p1);
			const auto n2 = normalise(p2);
			const auto n3 = normalise(p3);

			adaptrice->ajoute_normal(n0.x, n0.y, n0.z);
			adaptrice->ajoute_normal(n1.x, n1.y, n1.z);
			adaptrice->ajoute_normal(n2.x, n2.y, n2.z);
			adaptrice->ajoute_normal(n3.x, n3.y, n3.z);

			adaptrice->ajoute_polygone(poly, nullptr, poly, 4);

			poly[0] += decalage;
			poly[1] += decalage;
			poly[2] += decalage;
			poly[3] += decalage;
		}
	}
}

/* ************************************************************************** */

void cree_torus(AdaptriceCreationObjet *adaptrice,
				const float rayon_mineur,
				const float rayon_majeur,
				const float segment_mineur,
				const float segment_majeur,
				const float centre_x,
				const float centre_y,
				const float centre_z)
{
	constexpr auto tau = static_cast<float>(M_PI) * 2.0f;

	const auto vertical_angle_stride = tau / static_cast<float>(segment_majeur);
	const auto horizontal_angle_stride = tau / static_cast<float>(segment_mineur);

	int f1 = 0, f2, f3, f4;
	const auto tot_verts = segment_majeur * segment_mineur;

	adaptrice->reserve_sommets(tot_verts);
	adaptrice->reserve_polygones(tot_verts);

	int poly[4];

	for (int i = 0; i < segment_majeur; ++i) {
		auto theta = vertical_angle_stride * i;

		for (int j = 0; j < segment_mineur; ++j) {
			auto phi = horizontal_angle_stride * j;

			auto x = std::cos(theta) * (rayon_majeur + rayon_mineur * std::cos(phi));
			auto y = rayon_mineur * std::sin(phi);
			auto z = std::sin(theta) * (rayon_majeur + rayon_mineur * std::cos(phi));

			adaptrice->ajoute_sommet(x + centre_x, y + centre_y, z + centre_z);

			if (j + 1 == segment_mineur) {
				f2 = i * segment_mineur;
				f3 = f1 + segment_mineur;
				f4 = f2 + segment_mineur;
			}
			else {
				f2 = f1 + 1;
				f3 = f1 + segment_mineur;
				f4 = f3 + 1;
			}

			if (f2 >= tot_verts) {
				f2 -= tot_verts;
			}
			if (f3 >= tot_verts) {
				f3 -= tot_verts;
			}
			if (f4 >= tot_verts) {
				f4 -= tot_verts;
			}

			if (f2 > 0) {
				poly[0] = f1;
				poly[1] = f3;
				poly[2] = f4;
				poly[3] = f2;
			}
			else {
				poly[0] = f2;
				poly[1] = f1;
				poly[2] = f3;
				poly[3] = f4;
			}

			adaptrice->ajoute_polygone(poly, nullptr, nullptr, 4);

			++f1;
		}
	}
}

/* ************************************************************************** */

void cree_grille(AdaptriceCreationObjet *adaptrice,
				 const float taille_x,
				 const float taille_y,
				 const int lignes,
				 const int colonnes,
				 const float centre_x,
				 const float centre_y,
				 const float centre_z)
{
	adaptrice->reserve_sommets(lignes * colonnes);
	adaptrice->reserve_uvs(lignes * colonnes);
	adaptrice->reserve_polygones((lignes - 1) * (colonnes - 1));

	float point[3] = { 0.0f, centre_y, 0.0f };

	const auto &increment_x = taille_x * 2.0f / (lignes - 1);
	const auto &increment_y = taille_y * 2.0f / (colonnes - 1);
	const auto &debut_x = -taille_x + centre_x;
	const auto &debut_y = -taille_y + centre_z;

	const auto increment_x_uv = 1.0f / (lignes - 1);
	const auto increment_y_uv = 1.0f / (colonnes - 1);

	float uv[2] = { 0.0f, 0.0f };

	for (auto y = 0; y < colonnes; ++y) {
		point[2] = debut_y + y * increment_y;
		uv[1] = y * increment_y_uv;

		for (auto x = 0; x < lignes; ++x) {
			point[0] = debut_x + x * increment_x;
			uv[0] = x * increment_x_uv;

			adaptrice->ajoute_coord_uv_sommet(uv[0], uv[1]);
			adaptrice->ajoute_sommet(point[0], point[1], point[2]);
		}
	}

	adaptrice->ajoute_normal(0.0f, 1.0f, 0.0f);

	int poly[4] = { 0, 0, 0, 0 };
	int normaux[4] = { 0, 0, 0, 0 };

	/* crée une copie pour le lambda */
	const auto tot_x = lignes;

	auto index = [&tot_x](const int x, const int y)
	{
		return x + y * tot_x;
	};

	for (auto y = 1; y < colonnes; ++y) {
		for (auto x = 1; x < lignes; ++x) {
			poly[0] = index(x - 1, y - 1);
			poly[1] = index(x,     y - 1);
			poly[2] = index(x,     y    );
			poly[3] = index(x - 1, y    );

			adaptrice->ajoute_polygone(poly, poly, normaux, 4);
		}
	}
}

/* ************************************************************************** */

void cree_cercle(AdaptriceCreationObjet *adaptrice,
				 const int segments,
				 const float rayon,
				 const float centre_x,
				 const float centre_y,
				 const float centre_z)
{
	const auto phid = 2.0f * static_cast<float>(M_PI) / segments;
	auto phi = 0.0f;

	adaptrice->reserve_sommets(segments + 1);
	adaptrice->reserve_polygones(segments);

	float point[3] = { centre_x, centre_y, centre_z };

	adaptrice->ajoute_sommet(point[0], point[1], point[2]);

	for (int a = 0; a < segments; ++a, phi += phid) {
		/* Going this way ends up with normal(s) upward */
		point[0] = centre_x - rayon * std::sin(phi);
		point[2] = centre_z + rayon * std::cos(phi);

		adaptrice->ajoute_sommet(point[0], point[1], point[2]);
	}

	adaptrice->ajoute_normal(0.0f, 1.0f, 0.0f);

	auto index = segments;
	int poly[3] = { 0, 0, 0 };
	int normaux[3] = { 0, 0, 0 };

	for (auto i = 1ul; i <= segments; ++i) {
		poly[1] = index;
		poly[2] = i;

		adaptrice->ajoute_polygone(poly, nullptr, normaux, 3);

		index = i;
	}
}

/* ************************************************************************** */

void cree_cylindre(AdaptriceCreationObjet *adaptrice,
				   const int segments,
				   const float rayon_mineur,
				   const float rayon_majeur,
				   const float profondeur,
				   const float centre_x,
				   const float centre_y,
				   const float centre_z)
{
	const auto phid = 2.0f * static_cast<float>(M_PI) / segments;
	auto phi = 0.0f;

	adaptrice->reserve_sommets((rayon_majeur != 0.0f) ? segments * 2 + 2 : segments + 2);

	float vec[3] = {0.0f, 0.0f, 0.0f};

	const auto cent1 = 0;
	vec[1] = -profondeur;
	adaptrice->ajoute_sommet(vec[0] + centre_x, vec[1] + centre_y, vec[2] + centre_z);

	const auto cent2 = 1;
	vec[1] = profondeur;
	adaptrice->ajoute_sommet(vec[0] + centre_x, vec[1] + centre_y, vec[2] + centre_z);

	auto firstv1 = 0;
	auto firstv2 = 0;
	auto lastv1 = 0;
	auto lastv2 = 0;
	auto v1 = 0;
	auto v2 = 0;

	int nombre_points = 2;
	int poly[4];

	for (int a = 0; a < segments; ++a, phi += phid) {
		/* Going this way ends up with normal(s) upward */
		vec[0] = -rayon_mineur * std::sin(phi);
		vec[1] = -profondeur;
		vec[2] = rayon_mineur * std::cos(phi);

		v1 = nombre_points++;
		adaptrice->ajoute_sommet(vec[0] + centre_x, vec[1] + centre_y, vec[2] + centre_z);

		vec[0] = -rayon_majeur * std::sin(phi);
		vec[1] = profondeur;
		vec[2] = rayon_majeur * std::cos(phi);

		v2 = nombre_points++;
		adaptrice->ajoute_sommet(vec[0] + centre_x, vec[1] + centre_y, vec[2] + centre_z);

		if (a > 0) {
			/* Poly for the bottom cap. */
			poly[0] = cent1;
			poly[1] = lastv1;
			poly[2] = v1;

			adaptrice->ajoute_polygone(poly, nullptr, nullptr, 3);

			/* Poly for the top cap. */
			poly[0] = cent2;
			poly[1] = v2;
			poly[2] = lastv2;

			adaptrice->ajoute_polygone(poly, nullptr, nullptr, 3);

			/* Poly for the side. */
			poly[0] = lastv1;
			poly[1] = lastv2;
			poly[2] = v2;
			poly[3] = v1;

			adaptrice->ajoute_polygone(poly, nullptr, nullptr, 4);
		}
		else {
			firstv1 = v1;
			firstv2 = v2;
		}

		lastv1 = v1;
		lastv2 = v2;
	}

	/* Poly for the bottom cap. */
	poly[0] = cent1;
	poly[1] = v1;
	poly[2] = firstv1;

	adaptrice->ajoute_polygone(poly, nullptr, nullptr, 3);

	/* Poly for the top cap. */
	poly[0] = cent2;
	poly[1] = firstv2;
	poly[2] = v2;

	adaptrice->ajoute_polygone(poly, nullptr, nullptr, 3);

	/* Poly for the side. */
	poly[0] = v1;
	poly[1] = v2;
	poly[2] = firstv2;
	poly[3] = firstv1;

	adaptrice->ajoute_polygone(poly, nullptr, nullptr, 4);
}

/* ************************************************************************** */

static const float icovert[12][3] = {
	{   0.000f,    0.000f, -200.000f},
	{ 144.720f, -105.144f,  -89.443f},
	{ -55.277f, -170.128f,  -89.443f},
	{-178.885f,    0.000f,  -89.443f},
	{ -55.277f,  170.128f,  -89.443f},
	{ 144.720f,  105.144f,  -89.443f},
	{  55.277f, -170.128f,   89.443f},
	{-144.720f, -105.144f,   89.443f},
	{-144.720f,  105.144f,   89.443f},
	{  55.277f,  170.128f,   89.443f},
	{ 178.885f,    0.000f,   89.443f},
	{   0.000f,    0.000f,  200.000f}
};

static const int icoface[20][3] = {
	{ 0,  1,  2},
	{ 1,  0,  5},
	{ 0,  2,  3},
	{ 0,  3,  4},
	{ 0,  4,  5},
	{ 1,  5, 10},
	{ 2,  1,  6},
	{ 3,  2,  7},
	{ 4,  3,  8},
	{ 5,  4,  9},
	{ 1, 10,  6},
	{ 2,  6,  7},
	{ 3,  7,  8},
	{ 4,  8,  9},
	{ 5,  9, 10},
	{ 6, 10, 11},
	{ 7,  6, 11},
	{ 8,  7, 11},
	{ 9,  8, 11},
	{10,  9, 11}
};

void cree_icosphere(AdaptriceCreationObjet *adaptrice,
					const float rayon,
					const float centre_x,
					const float centre_y,
					const float centre_z)
{
	const auto rayon_div = rayon / 200.0f;

	adaptrice->reserve_sommets(12);

	numero7::math::vec3f vec, nor;

	for (int a = 0; a < 12; a++) {
		vec[0] = rayon_div * icovert[a][0] + centre_x;
		vec[1] = rayon_div * icovert[a][2] + centre_y;
		vec[2] = rayon_div * icovert[a][1] + centre_z;

		nor = normalise(vec);

		adaptrice->ajoute_sommet(vec[0], vec[1], vec[2]);
		adaptrice->ajoute_normal(nor[0], nor[1], nor[2]);
	}

	adaptrice->reserve_polygones(20);

	for (auto i = 0; i < 20; ++i) {
		adaptrice->ajoute_polygone(icoface[i], nullptr, icoface[i], 3);
	}
}

}  /* namespace objets */
