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

#pragma once

#include "noeud.hh"

namespace kdo {

struct sphere;

struct delegue_sphere {
	sphere const &ptr_sphere;

	delegue_sphere(sphere const &m);

	long nombre_elements() const;

	void coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const;

	dls::phys::esectd intersecte_element(long idx, dls::phys::rayond const &rayon) const;
};

struct sphere : public noeud {
	dls::math::vec3f point{};
	int index = 0;
	float rayon{};
	double rayon2{};

	delegue_sphere delegue;

	sphere();

	void construit_arbre_hbe() override;

	dls::phys::esectd traverse_arbre(dls::phys::rayond const &rayon) override;

	limites3d calcule_limites() override;
};

}  /* namespace kdo */
