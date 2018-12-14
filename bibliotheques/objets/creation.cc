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

#include <delsace/math/vecteur.hh>

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
	auto const &x_div = 2;
	auto const &y_div = 2;
	auto const &z_div = 2;

	auto const size = dimension * uniform_scale;

	auto const &start_x = -(size.x / 2.0f) + center.x;
	auto const &start_y = -(size.y / 2.0f) + center.y;
	auto const &start_z = -(size.z / 2.0f) + center.z;

	auto const &x_increment = size.x / (x_div - 1);
	auto const &y_increment = size.y / (y_div - 1);
	auto const &z_increment = size.z / (z_div - 1);

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

	dls::math::vec3f min(-taille_x + centre_x, -taille_y + centre_y, -taille_z + centre_z);
	dls::math::vec3f max( taille_x + centre_x,  taille_y + centre_y,  taille_z + centre_z);

	const dls::math::vec3f sommets[8] = {
		dls::math::vec3f(min[0], min[1], min[2]),
		dls::math::vec3f(max[0], min[1], min[2]),
		dls::math::vec3f(max[0], max[1], min[2]),
		dls::math::vec3f(min[0], max[1], min[2]),
		dls::math::vec3f(min[0], min[1], max[2]),
		dls::math::vec3f(max[0], min[1], max[2]),
		dls::math::vec3f(max[0], max[1], max[2]),
		dls::math::vec3f(min[0], max[1], max[2])
	};

	adaptrice->reserve_sommets(8);

	for (auto const &sommet : sommets) {
		adaptrice->ajoute_sommet(sommet.x, sommet.y, sommet.z);
	}

	const dls::math::vec3f normaux[6] = {
		dls::math::vec3f(-1.0f,  0.0f,  0.0f),
		dls::math::vec3f( 1.0f,  0.0f,  0.0f),
		dls::math::vec3f( 0.0f, -1.0f,  0.0f),
		dls::math::vec3f( 0.0f,  1.0f,  0.0f),
		dls::math::vec3f( 0.0f,  0.0f, -1.0f),
		dls::math::vec3f( 0.0f,  0.0f,  1.0f),
	};

	adaptrice->reserve_normaux(6);

	for (auto const &normal : normaux) {
		adaptrice->ajoute_normal(normal.x, normal.y, normal.z);
	}

	const dls::math::vec2f uvs[4] = {
		dls::math::vec2f{ 0.0f, 0.0f },
		dls::math::vec2f{ 0.0f, 1.0f },
		dls::math::vec2f{ 1.0f, 1.0f },
		dls::math::vec2f{ 1.0f, 0.0f },
	};

	adaptrice->reserve_uvs(4);

	for (auto const &uv : uvs) {
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
	return dls::math::vec3<T>(
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
	auto const resolution_u = 48;
	auto const resolution_v = resolution_u / 2;
	auto const debut_u = 0.0f;
	auto const debut_v = 0.0f;
	auto const fin_u = static_cast<float>(M_PI * 2);
	auto const fin_v = static_cast<float>(M_PI);

	/* Taille entre deux points de longitude. */
	auto const taille_pas_u = (fin_u - debut_u) / resolution_u;

	/* Taille entre deux points de latitude. */
	auto const taille_pas_v = (fin_v - debut_v) / resolution_v;

	int poly[4] = { 0, 1, 2, 3 };
	const int decalage = 4;

	auto const &centre = dls::math::vec3f(centre_x, centre_y, centre_z);

	for (int i = 0; i < resolution_u; i++) {
		for (int j = 0; j < resolution_v; j++) {
			auto const u = static_cast<float>(i) * taille_pas_u + debut_u;
			auto const v = static_cast<float>(j) * taille_pas_v + debut_v;
			auto const un = (i + 1 == resolution_u) ? fin_u : static_cast<float>(i + 1) * taille_pas_u + debut_u;
			auto const vn = (j + 1 == resolution_v) ? fin_v : static_cast<float>(j + 1) * taille_pas_v + debut_v;

			/* Trouve les quatre points de la grille en évaluant la fonction
			 * paramétrique.
			 *
			 * REMARQUE : pour les sphères, le normal est simplement la version
			 * normalisée du sommet. Ce n'est généralement pas le cas pour
			 * toutes les fonctions paramétriques.
			 */

			auto const p0 = sphere(u, v, rayon) + centre;
			auto const p1 = sphere(un, v, rayon) + centre;
			auto const p2 = sphere(un, vn, rayon) + centre;
			auto const p3 = sphere(u, vn, rayon) + centre;

			adaptrice->ajoute_sommet(p0.x, p0.y, p0.z);
			adaptrice->ajoute_sommet(p1.x, p1.y, p1.z);
			adaptrice->ajoute_sommet(p2.x, p2.y, p2.z);
			adaptrice->ajoute_sommet(p3.x, p3.y, p3.z);

			auto const n0 = normalise(p0);
			auto const n1 = normalise(p1);
			auto const n2 = normalise(p2);
			auto const n3 = normalise(p3);

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
				const unsigned long segment_mineur,
				const unsigned long segment_majeur,
				const float centre_x,
				const float centre_y,
				const float centre_z)
{
	constexpr auto tau = static_cast<float>(M_PI) * 2.0f;

	auto const vertical_angle_stride = tau / static_cast<float>(segment_majeur);
	auto const horizontal_angle_stride = tau / static_cast<float>(segment_mineur);

	unsigned long f1 = 0, f2, f3, f4;
	auto const tot_verts = segment_majeur * segment_mineur;

	adaptrice->reserve_sommets(tot_verts);
	adaptrice->reserve_polygones(tot_verts);

	int poly[4];

	for (unsigned long i = 0; i < segment_majeur; ++i) {
		auto theta = vertical_angle_stride * static_cast<float>(i);

		for (unsigned long j = 0; j < segment_mineur; ++j) {
			auto phi = horizontal_angle_stride * static_cast<float>(j);

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
				poly[0] = static_cast<int>(f1);
				poly[1] = static_cast<int>(f3);
				poly[2] = static_cast<int>(f4);
				poly[3] = static_cast<int>(f2);
			}
			else {
				poly[0] = static_cast<int>(f2);
				poly[1] = static_cast<int>(f1);
				poly[2] = static_cast<int>(f3);
				poly[3] = static_cast<int>(f4);
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
				 const unsigned long lignes,
				 const unsigned long colonnes,
				 const float centre_x,
				 const float centre_y,
				 const float centre_z)
{
	adaptrice->reserve_sommets(lignes * colonnes);
	adaptrice->reserve_uvs(lignes * colonnes);
	adaptrice->reserve_polygones((lignes - 1) * (colonnes - 1));

	float point[3] = { 0.0f, centre_y, 0.0f };

	auto const &increment_x = taille_x * 2.0f / static_cast<float>(lignes - 1);
	auto const &increment_y = taille_y * 2.0f / static_cast<float>(colonnes - 1);
	auto const &debut_x = -taille_x + centre_x;
	auto const &debut_y = -taille_y + centre_z;

	auto const increment_x_uv = 1.0f / static_cast<float>(lignes - 1);
	auto const increment_y_uv = 1.0f / static_cast<float>(colonnes - 1);

	float uv[2] = { 0.0f, 0.0f };

	for (auto y = 0ul; y < colonnes; ++y) {
		point[2] = debut_y + static_cast<float>(y) * increment_y;
		uv[1] = static_cast<float>(y) * increment_y_uv;

		for (auto x = 0ul; x < lignes; ++x) {
			point[0] = debut_x + static_cast<float>(x) * increment_x;
			uv[0] = static_cast<float>(x) * increment_x_uv;

			adaptrice->ajoute_coord_uv_sommet(uv[0], uv[1]);
			adaptrice->ajoute_sommet(point[0], point[1], point[2]);
		}
	}

	adaptrice->ajoute_normal(0.0f, 1.0f, 0.0f);

	int poly[4] = { 0, 0, 0, 0 };
	int normaux[4] = { 0, 0, 0, 0 };

	/* crée une copie pour le lambda */
	auto const tot_x = lignes;

	auto index = [&tot_x](unsigned long x, unsigned long y)
	{
		return x + y * tot_x;
	};

	for (auto y = 1ul; y < colonnes; ++y) {
		for (auto x = 1ul; x < lignes; ++x) {
			poly[0] = static_cast<int>(index(x - 1, y - 1));
			poly[1] = static_cast<int>(index(x,     y - 1));
			poly[2] = static_cast<int>(index(x,     y    ));
			poly[3] = static_cast<int>(index(x - 1, y    ));

			adaptrice->ajoute_polygone(poly, poly, normaux, 4);
		}
	}
}

/* ************************************************************************** */

void cree_cercle(AdaptriceCreationObjet *adaptrice,
				 const unsigned long segments,
				 const float rayon,
				 const float centre_x,
				 const float centre_y,
				 const float centre_z)
{
	auto const phid = 2.0f * static_cast<float>(M_PI) / static_cast<float>(segments);
	auto phi = 0.0f;

	adaptrice->reserve_sommets(segments + 1);
	adaptrice->reserve_polygones(segments);

	float point[3] = { centre_x, centre_y, centre_z };

	adaptrice->ajoute_sommet(point[0], point[1], point[2]);

	for (unsigned long a = 0; a < segments; ++a, phi += phid) {
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
		poly[1] = static_cast<int>(index);
		poly[2] = static_cast<int>(i);

		adaptrice->ajoute_polygone(poly, nullptr, normaux, 3);

		index = i;
	}
}

/* ************************************************************************** */

void cree_cylindre(AdaptriceCreationObjet *adaptrice,
				   const unsigned long segments,
				   const float rayon_mineur,
				   const float rayon_majeur,
				   const float profondeur,
				   const float centre_x,
				   const float centre_y,
				   const float centre_z)
{
	auto const phid = 2.0f * static_cast<float>(M_PI) / static_cast<float>(segments);
	auto phi = 0.0f;

	adaptrice->reserve_sommets((rayon_majeur != 0.0f) ? segments * 2 + 2 : segments + 2);

	float vec[3] = {0.0f, 0.0f, 0.0f};

	auto const cent1 = 0;
	vec[1] = -profondeur;
	adaptrice->ajoute_sommet(vec[0] + centre_x, vec[1] + centre_y, vec[2] + centre_z);

	auto const cent2 = 1;
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

	for (auto a = 0ul; a < segments; ++a, phi += phid) {
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
	auto const rayon_div = rayon / 200.0f;

	adaptrice->reserve_sommets(12);

	dls::math::vec3f vec, nor;

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
