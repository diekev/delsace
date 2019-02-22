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

#include "normaux.hh"

#include "../corps/corps.h"

void calcul_normaux(Corps &corps, bool plats, bool inverse_normaux)
{
	auto attr_normaux = corps.ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT, true);

	if (attr_normaux->taille() != 0l) {
		attr_normaux->reinitialise();
	}

	auto liste_points = corps.points();
	auto liste_prims = corps.prims();
	auto nombre_prims = liste_prims->taille();

	if (plats) {
		attr_normaux->reserve(nombre_prims);
		attr_normaux->portee = portee_attr::PRIMITIVE;

		for (auto ip = 0; ip < liste_prims->taille(); ++ip) {
			auto prim = liste_prims->prim(ip);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);

			if (poly->type != type_polygone::FERME || poly->nombre_sommets() < 3) {
				attr_normaux->pousse_vec3(dls::math::vec3f(0.0f));
				continue;
			}

			auto const &v0 = liste_points->point(poly->index_point(0));
			auto const &v1 = liste_points->point(poly->index_point(1));
			auto const &v2 = liste_points->point(poly->index_point(2));

			auto const e1 = v1 - v0;
			auto const e2 = v2 - v0;

			auto const nor = normalise(produit_croix(e1, e2));

			if (inverse_normaux) {
				attr_normaux->pousse_vec3(-nor);
			}
			else {
				attr_normaux->pousse_vec3(nor);
			}
		}
	}
	else {
		auto nombre_sommets = liste_points->taille();
		attr_normaux->redimensionne(nombre_sommets);
		attr_normaux->portee = portee_attr::POINT;

		/* calcul le normal de chaque polygone */
		for (auto ip = 0; ip < liste_prims->taille(); ++ip) {
			auto prim = liste_prims->prim(ip);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);

			if (poly->type != type_polygone::FERME || poly->nombre_sommets() < 3) {
				poly->nor = dls::math::vec3f(0.0f);
				continue;
			}

			auto const &v0 = liste_points->point(poly->index_point(0));
			auto const &v1 = liste_points->point(poly->index_point(1));
			auto const &v2 = liste_points->point(poly->index_point(2));

			auto const e1 = v1 - v0;
			auto const e2 = v2 - v0;

			poly->nor = normalise(produit_croix(e1, e2));
		}

		for (auto ip = 0; ip < liste_prims->taille(); ++ip) {
			auto prim = liste_prims->prim(ip);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);

			if (poly->type == type_polygone::OUVERT || poly->nombre_sommets() < 3) {
				continue;
			}

			for (long i = 0; i < poly->nombre_segments(); ++i) {
				auto const index_sommet = poly->index_point(i);

				auto nor = attr_normaux->vec3(index_sommet);
				nor += poly->nor;
				attr_normaux->vec3(index_sommet, nor);
			}
		}

		/* normalise les normaux */
		for (long n = 0; n < nombre_sommets; ++n) {
			auto nor = attr_normaux->vec3(n);
			nor = normalise(nor);

			if (inverse_normaux) {
				attr_normaux->vec3(n, -nor);
			}
			else {
				attr_normaux->vec3(n, nor);
			}
		}
	}
}
