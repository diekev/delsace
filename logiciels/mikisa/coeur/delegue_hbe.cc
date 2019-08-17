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

#include "delegue_hbe.hh"

#include "biblinternes/phys/collision.hh"

#include "corps/corps.h"
#include "corps/limites_corps.hh"

DeleguePrim::DeleguePrim(const Corps &corps)
	: m_corps(corps)
{}

long DeleguePrim::nombre_elements() const
{
	return m_corps.prims()->taille();
}

void DeleguePrim::coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const
{
	auto prim = m_corps.prims()->prim(idx);
	auto poly = dynamic_cast<Polygone *>(prim);

	cos.efface();

	for (auto i = 0; i < poly->nombre_sommets(); ++i) {
		auto p = m_corps.point_transforme(poly->index_point(i));
		cos.pousse(p);
	}
}

BoiteEnglobante DeleguePrim::boite_englobante(long idx) const
{
	auto prim = m_corps.prims()->prim(idx);
	auto poly = dynamic_cast<Polygone *>(prim);

	auto boite = BoiteEnglobante{};

	for (auto i = 0; i < poly->nombre_sommets(); ++i) {
		auto p = m_corps.point_transforme(poly->index_point(i));

		for (auto j = 0ul; j < 3; ++j) {
			boite.min[j] = std::min(boite.min[j], static_cast<double>(p[j]));
			boite.max[j] = std::max(boite.max[j], static_cast<double>(p[j]));
		}
	}

	boite.centroide = (boite.min + boite.max) * 0.5;
	boite.id = idx;

	return boite;
}

dls::phys::esectd DeleguePrim::intersecte_element(long idx, const dls::phys::rayond &r) const
{
	auto intersection = dls::phys::esectd{};
	auto prim = m_corps.prims()->prim(idx);
	auto points = m_corps.points_pour_lecture();

	if (prim->type_prim() != type_primitive::POLYGONE) {
		return intersection;
	}

	auto poly = dynamic_cast<Polygone *>(prim);

	if (poly->type != type_polygone::FERME) {
		return intersection;
	}

	for (auto j = 2; j < poly->nombre_sommets(); ++j) {
		auto const &v0 = points->point(poly->index_point(0));
		auto const &v1 = points->point(poly->index_point(j - 1));
		auto const &v2 = points->point(poly->index_point(j));

		auto const &v0_d = m_corps.transformation(dls::math::point3d(v0));
		auto const &v1_d = m_corps.transformation(dls::math::point3d(v1));
		auto const &v2_d = m_corps.transformation(dls::math::point3d(v2));

		auto dist = 1000.0;

		if (entresecte_triangle(v0_d, v1_d, v2_d, r, dist)) {
			intersection.touche = true;
			intersection.point = r.origine + dist * r.direction;
			intersection.idx = idx;
			intersection.distance = dist;
			break;
		}
	}

	return intersection;
}

DonneesPointPlusProche DeleguePrim::calcule_point_plus_proche(long idx, const dls::math::point3d &p) const
{
	auto prim = m_corps.prims()->prim(idx);
	auto points = m_corps.points_pour_lecture();

	auto donnees = DonneesPointPlusProche{};

	if (prim->type_prim() != type_primitive::POLYGONE) {
		return donnees;
	}

	auto poly = dynamic_cast<Polygone *>(prim);

	if (poly->type != type_polygone::FERME) {
		return donnees;
	}

	auto distance_min = std::numeric_limits<double>::max();

	for (auto j = 2; j < poly->nombre_sommets(); ++j) {
		auto const &v0 = points->point(poly->index_point(0));
		auto const &v1 = points->point(poly->index_point(j - 1));
		auto const &v2 = points->point(poly->index_point(j));

		auto const &v0_d = m_corps.transformation(dls::math::point3d(v0));
		auto const &v1_d = m_corps.transformation(dls::math::point3d(v1));
		auto const &v2_d = m_corps.transformation(dls::math::point3d(v2));

		auto pnt_pls_prch = plus_proche_point_triangle(
					dls::math::vec3d(p),
					dls::math::vec3d(v0_d),
					dls::math::vec3d(v1_d),
					dls::math::vec3d(v2_d));

		auto distance = longueur_carree(dls::math::vec3d(p) - pnt_pls_prch);

		if (distance < distance_min) {
			distance_min = distance;
			donnees.distance_carree = distance;
			donnees.index = idx;
			donnees.point = dls::math::point3d(pnt_pls_prch);
		}
	}

	return donnees;
}

/* ************************************************************************** */

delegue_arbre_octernaire::delegue_arbre_octernaire(const Corps &c)
	: corps(c)
{}

long delegue_arbre_octernaire::nombre_elements() const
{
	return corps.prims()->taille();
}

limites3f delegue_arbre_octernaire::limites_globales() const
{
	return calcule_limites_mondiales_corps(corps);
}

limites3f delegue_arbre_octernaire::calcule_limites(long idx) const
{
	auto prim = corps.prims()->prim(idx);
	auto poly = dynamic_cast<Polygone *>(prim);

	auto limites = limites3f(
				dls::math::vec3f( constantes<float>::INFINITE),
				dls::math::vec3f(-constantes<float>::INFINITE));

	for (auto i = 0; i < poly->nombre_sommets(); ++i) {
		auto const &p = corps.point_transforme(poly->index_point(i));
		extrait_min_max(p, limites.min, limites.max);
	}

	return limites;
}
