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

#include "biblinternes/math/vecteur.hh"

namespace bruit {

enum class type : char {
	CELLULE,
	FOURIER,
	ONDELETTE,
	PERLIN_LONG,
	SIMPLEX,
	VORONOI_F1,
	VORONOI_F2,
	VORONOI_F3,
	VORONOI_F4,
	VORONOI_F1F2,
	VORONOI_CR,
};

struct parametres {
	/* décalage aléatoire simuler différentes graines */
	dls::math::vec3f decalage_graine{0.0f};

	/* décalage et échelle de la position d'entrée, définies réélement la
	 * matrice du bruit, sans rotation */
	dls::math::vec3f decalage_pos{0.0f};
	dls::math::vec3f echelle_pos{1.0f};

	/* décalage et échelle de la valeur */
	float decalage_valeur = 0.0f;
	float echelle_valeur = 1.0f;

	/* restriction de la valeur de sortie */
	bool restreint = false;
	type type_bruit{};
	float restreint_neg = 0.0f;
	float restraint_pos = 1.0f;

	/* animation */
	float temps_anim = 0.0f;

	/* tuile pour le bruit ondelette */
	float *tuile = nullptr;

	/* table pour les bruits de type Perlin */
//	int *table_int = nullptr;
//	unsigned char *table_uchar = nullptr;

	parametres() = default;

	parametres(parametres const &) = default;
	parametres &operator=(parametres const &) = default;
};

}  /* namespace bruit */
