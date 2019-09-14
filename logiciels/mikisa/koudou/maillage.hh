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

#pragma once

#include "biblinternes/structures/tableau.hh"

#include "noeud.hh"
#include "tableau_index.hh"

namespace kdo {

struct maillage;

struct delegue_maillage {
	maillage const &ptr_maillage;

	delegue_maillage(maillage const &m);

	long nombre_elements() const;

	void coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const;

	dls::phys::esectd intersecte_element(long idx, dls::phys::rayond const &rayon) const;
};

struct maillage : public noeud {
	dls::tableau<dls::math::vec3f> points{};
	dls::tableau<dls::math::vec3f> normaux{};

	/* Nous gardons et des triangles et des quads pour économiser la mémoire.
	 * Puisqu'un quad = 2 triangles, pour chaque quad nous avons deux fois moins
	 * de noeuds dans l'arbre_hbe, ainsi que 1.5 fois moins d'index pour les
	 * points et normaux (2x3 pour les triangles vs 1x4 pour les quads).
	 */
	tableau_index triangles{};
	tableau_index quads{};

	tableau_index normaux_triangles{};
	tableau_index normaux_quads{};

	delegue_maillage delegue;

	int nombre_triangles = 0;
	int nombre_quads = 0;
	int index = 0;
	int volume = -1;

	maillage();

	void construit_arbre_hbe() override;

	dls::phys::esectd traverse_arbre(dls::phys::rayond const &rayon) override;

	limites3d calcule_limites() override;
};

}  /* namespace kdo */
