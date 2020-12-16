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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "grille_particules.hh"

#include "biblinternes/structures/ensemble.hh"

GrilleParticules::GrilleParticules(const dls::math::point3d &min, const dls::math::point3d &max, float distance)
	: m_min(min)
	, m_max(max)
	, m_dim(max - min)
	, m_distance(static_cast<double>(distance) * 10.0)
{
	m_res_x = std::max(1l, static_cast<long>(m_dim.x / m_distance));
	m_res_y = std::max(1l, static_cast<long>(m_dim.y / m_distance));
	m_res_z = std::max(1l, static_cast<long>(m_dim.z / m_distance));

//	std::cerr << "Résolution Grille Particules : " << m_res_x << ", " << m_res_y << ", " << m_res_z << '\n';

	m_grille.redimensionne(m_res_x * m_res_y * m_res_z);
}

void GrilleParticules::ajoute(const dls::math::vec3f &position)
{
	auto index = calcul_index_pos(position);
	m_grille[index].ajoute(position);
}

bool GrilleParticules::verifie_distance_minimal(const dls::math::vec3f &point, float distance)
{
#if 1
	auto moitie = distance * 0.5f;
	dls::ensemble<long> indices;

	for (auto dx = -moitie; dx <= moitie; dx += moitie) {
		for (auto dy = -moitie; dy <= moitie; dy += moitie) {
			for (auto dz = -moitie; dz <= moitie; dz += moitie) {
				auto const p = dls::math::vec3f{point.x + dx, point.y + dy, point.z + dz};
				auto index_pos = calcul_index_pos(p);
				indices.insere(index_pos);
			}
		}
	}

	for (auto &index : indices) {
		auto const &points = m_grille[index];

		for (auto p = 0l; p < points.taille(); ++p) {
			if (longueur(point - points[p]) < distance) {
				return false;
			}
		}
	}

	return true;
#else
	auto index = calcul_index_pos(point);
	auto const &points = m_grille[index];

	for (auto p = 0ul; p < points.taille(); ++p) {
		if (longueur(point - points[p]) < distance) {
			return false;
		}
	}

	return true;
#endif
}

bool GrilleParticules::triangle_couvert(const dls::math::vec3f &v0, const dls::math::vec3f &v1, const dls::math::vec3f &v2, const float radius)
{
#if 1
	dls::ensemble<long> indices;
	indices.insere(calcul_index_pos(v0));
	indices.insere(calcul_index_pos(v1));
	indices.insere(calcul_index_pos(v2));

	for (auto &index : indices) {
		auto const &points = m_grille[index];

		for (auto p = 0l; p < points.taille(); ++p) {
			if (longueur(v0 - points[p]) <= radius
					&& longueur(v1 - points[p]) <= radius
					&& longueur(v2 - points[p]) <= radius)
			{
				return true;
			}
		}
	}
#else
	auto const centre_triangle = (v0 + v1 + v2) / 3.0f;
	auto const index_triangle = calcul_index_pos(centre_triangle);
	auto const points = m_grille[index_triangle];

	for (auto p = 0l; p < points.taille(); ++p) {
		if (longueur(v0 - points[p]) <= radius
				&& longueur(v1 - points[p]) <= radius
				&& longueur(v2 - points[p]) <= radius)
		{
			return true;
		}
	}
#endif

	return false;
}

long GrilleParticules::calcul_index_pos(const dls::math::vec3f &point)
{
	auto px = dls::math::restreint(static_cast<double>(point.x), m_min.x, m_max.x);
	auto py = dls::math::restreint(static_cast<double>(point.y), m_min.y, m_max.y);
	auto pz = dls::math::restreint(static_cast<double>(point.z), m_min.z, m_max.z);

	auto index_x = static_cast<long>((px - m_min.x) / m_distance);
	auto index_y = static_cast<long>((py - m_min.y) / m_distance);
	auto index_z = static_cast<long>((pz - m_min.z) / m_distance);

	index_x = dls::math::restreint(index_x, 0l, m_res_x - 1);
	index_y = dls::math::restreint(index_y, 0l, m_res_y - 1);
	index_z = dls::math::restreint(index_z, 0l, m_res_z - 1);

	return index_x + index_y * m_res_x + index_z * m_res_x * m_res_y;
}

/* ************************************************************************** */

GrillePoint::GrillePoint(float taille_cellule)
	: m_taille_cellule(taille_cellule)
{}

GrillePoint GrillePoint::construit_avec_fonction(std::function<dls::math::vec3f (long)> points, long nombre_points, float taille_cellule)
{
	auto grille = GrillePoint(taille_cellule);

	/* calcul des limites et de la résolution de la grille */
	auto limites = limites3f{};
	limites.min = dls::math::vec3f( std::numeric_limits<float>::max());
	limites.max = dls::math::vec3f(-std::numeric_limits<float>::max());

	for (auto i = 0; i < nombre_points; ++i) {
		auto p = points(i);
		extrait_min_max(p, limites.min, limites.max);
	}

	grille.m_coord_min = limites.min;
	grille.m_resolution = grille.pos_grille(limites.max) + coord(1);

	grille.m_index_cellules.redimensionne(grille.nombre_elements() + 1);

	for (auto i = 0; i < nombre_points; ++i) {
		auto idx = grille.index_cellule(points(i));
		grille.m_index_cellules[idx + 1] += 1;
	}

	/* accumule les sommes pour avoir des index dans le tableau de points */
	for (unsigned i = 1; i <= grille.nombre_elements(); ++i) {
		grille.m_index_cellules[i] += grille.m_index_cellules[i - 1];
	}

	/* remplis le tableau de points */
	tableau_index compte_points(grille.nombre_elements(), 0);
	grille.m_points.redimensionne(grille.m_index_cellules.back());
	for (auto i = 0; i < nombre_points; ++i) {
		auto p = points(i);
		auto idx = grille.index_cellule(p);
		*(grille.debut_points(idx) + compte_points[idx]) = DonneesPoint(p, i);
		++compte_points[idx];
	}

	for (unsigned i = 0; i < grille.nombre_elements(); ++i) {
		assert(compte_points[i] == grille.m_index_cellules[i+1] - grille.m_index_cellules[i]);
	}

	return grille;
}

GrillePoint::coord GrillePoint::pos_grille(const dls::math::vec3f &p) const
{
	auto pp = (p - m_coord_min) / m_taille_cellule;
	return dls::math::converti_type<int>(pp);
}

long GrillePoint::index_cellule(const dls::math::vec3f &p) const
{
	return dls::math::calcul_index(pos_grille(p), m_resolution);
}

long GrillePoint::index_cellule(const GrillePoint::coord &c) const
{
	return dls::math::calcul_index(c, m_resolution);
}

long GrillePoint::nombre_elements() const
{
	return produit_interne(m_resolution);
}

GrillePoint::DonneesPoint *GrillePoint::debut_points(long idx)
{
	return &m_points[0] + m_index_cellules[idx];
}

GrillePoint::DonneesPoint *GrillePoint::fin_points(long idx)
{
	return &m_points[0] + m_index_cellules[idx + 1];
}

GrillePoint::bloc GrillePoint::cellules_autour(const dls::math::vec3f &p, float rayon)
{
	return bloc(std::max(pos_grille(p - dls::math::vec3f(rayon)), coord(0)),
				std::min(pos_grille(p + dls::math::vec3f(rayon)) + coord(1), m_resolution));
}
