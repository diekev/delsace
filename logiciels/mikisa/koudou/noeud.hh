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

#include "biblinternes/math/limites.hh"
#include "biblinternes/math/transformation.hh"

#include "coeur/arbre_hbe.hh"

namespace kdo {

class Nuanceur;

enum class type_noeud {
	MAILLAGE,
	LUMIERE,
};

struct noeud {
	math::transformation tranformation{};
	bli::BVHTree *arbre_hbe = nullptr;

	Nuanceur *nuanceur = nullptr;

	type_noeud type{};

	noeud(type_noeud t);

	COPIE_CONSTRUCT(noeud);

	virtual ~noeud();

	virtual void construit_arbre_hbe();

	virtual dls::phys::esectd traverse_arbre(dls::phys::rayond const &rayon);

	virtual limites3d calcule_limites();
};

}  /* namespace kdo */
