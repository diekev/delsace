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

#include "arbre_octernaire.hh"

#include "bibliotheques/outils/constantes.h"

#include "bibloc/logeuse_memoire.hh"

#include "corps/corps.h"

ArbreOcternaire::Noeud::~Noeud()
{
	for (int i = 0; i < 8; ++i) {
		memoire::deloge(enfants[i]);
	}
}

ArbreOcternaire::ArbreOcternaire(limites3f const &limites)
	: m_racine()
{
	m_racine.limites = limites;
	m_racine.profondeur = 0;
	m_racine.est_feuille = false;

	construit_enfants(&m_racine);
}

void ArbreOcternaire::ajoute_triangle(const Triangle &triangle)
{
	auto limites = limites3f(
				dls::math::vec3f( constantes<float>::INFINITE),
				dls::math::vec3f(-constantes<float>::INFINITE));

	extrait_min_max(triangle.v0, limites.min, limites.max);
	extrait_min_max(triangle.v1, limites.min, limites.max);
	extrait_min_max(triangle.v2, limites.min, limites.max);

	insert_triangle(&m_racine, triangle, limites);
}

void ArbreOcternaire::insert_triangle(
		ArbreOcternaire::Noeud *noeud,
		Triangle const &triangle,
		limites3f const &limites_enfant)
{
	if (noeud->est_feuille) {
		noeud->triangles.push_back(triangle);
		return;
	}

	/* Algorithme : ajout du triangle dans toutes les feuilles le contenant */
	for (auto i = 0; i < 8; ++i) {
		auto enfant = noeud->enfants[i];

		if (!enfant->limites.chevauchent(limites_enfant)) {
			continue;
		}

		if (enfant->enfants[0] == nullptr) {
			construit_enfants(enfant);
		}

		insert_triangle(enfant, triangle, limites_enfant);
	}
}

void ArbreOcternaire::construit_enfants(ArbreOcternaire::Noeud *noeud)
{
	auto const &min = noeud->limites.min;
	auto const &max = noeud->limites.max;

	auto centre = min + (max - min) * 0.5f;

	limites3f limites[8] = {
		limites3f(min, centre),
		limites3f(dls::math::vec3f(centre.x, min.y, min.z), dls::math::vec3f(max.x, centre.y, centre.z)),
		limites3f(dls::math::vec3f(centre.x, min.y, centre.z), dls::math::vec3f(max.x, centre.y, max.z)),
		limites3f(dls::math::vec3f(min.x, min.y, centre.z), dls::math::vec3f(centre.x, centre.y, max.z)),
		limites3f(dls::math::vec3f(min.x, centre.y, min.z), dls::math::vec3f(centre.x, max.y, centre.z)),
		limites3f(dls::math::vec3f(centre.x, centre.y, min.z), dls::math::vec3f(max.x, max.y, centre.z)),
		limites3f(centre, max),
		limites3f(dls::math::vec3f(min.x, centre.y, centre.z), dls::math::vec3f(centre.x, max.y, max.z)),
	};

	for (auto i = 0; i < 8; ++i) {
		auto enfant = memoire::loge<Noeud>();
		enfant->limites = limites[i];
		enfant->profondeur = noeud->profondeur + 1;
		enfant->est_feuille = (enfant->profondeur == m_profondeur_max);

		noeud->enfants[i] = enfant;
	}
}

ArbreOcternaire::Noeud *ArbreOcternaire::racine()
{
	return &m_racine;
}

void rassemble_topologie(ArbreOcternaire::Noeud *noeud, Corps &corps)
{
	if (noeud->est_feuille && noeud->triangles.empty()) {
		return;
	}

	auto const &min = noeud->limites.min;
	auto const &max = noeud->limites.max;

	dls::math::vec3f sommets[8] = {
		dls::math::vec3f(min.x, min.y, min.z),
		dls::math::vec3f(min.x, min.y, max.z),
		dls::math::vec3f(max.x, min.y, max.z),
		dls::math::vec3f(max.x, min.y, min.z),
		dls::math::vec3f(min.x, max.y, min.z),
		dls::math::vec3f(min.x, max.y, max.z),
		dls::math::vec3f(max.x, max.y, max.z),
		dls::math::vec3f(max.x, max.y, min.z),
	};

	long cotes[12][2] = {
		{ 0, 1 },
		{ 1, 2 },
		{ 2, 3 },
		{ 3, 0 },
		{ 0, 4 },
		{ 1, 5 },
		{ 2, 6 },
		{ 3, 7 },
		{ 4, 5 },
		{ 5, 6 },
		{ 6, 7 },
		{ 7, 4 },
	};

	auto decalage = corps.points()->taille();

	for (int i = 0; i < 8; ++i) {
		corps.ajoute_point(sommets[i].x, sommets[i].y, sommets[i].z);
	}

	for (int i = 0; i < 12; ++i) {
		auto poly = Polygone::construit(&corps, type_polygone::OUVERT, 2);
		poly->ajoute_sommet(decalage + cotes[i][0]);
		poly->ajoute_sommet(decalage + cotes[i][1]);
	}

	if (noeud->est_feuille) {
		return;
	}

	for (int i = 0; i < 8; ++i) {
		if (noeud->enfants[i] == nullptr) {
			continue;
		}

		rassemble_topologie(noeud->enfants[i], corps);
	}
}
