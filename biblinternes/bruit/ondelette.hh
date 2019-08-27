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

#include "parametres.hh"

namespace bruit {

struct ondelette {
private:
	float *m_donnees = nullptr;

	/* décalage aléatoire dans la tuile pour simuler différentes graines */
	dls::math::vec3f m_decalage_graine{};

public:
	/* taille d'un voxel */
	float dx = 1.0f;

	/* normalisation de la taille */
	float taille_grille_inv = 1.0f;

	/* décalage et échelle de la position */
	dls::math::vec3f decalage_pos{0.0f};
	dls::math::vec3f echelle_pos{1.0f};

	/* décalage et échelle de la valeur */
	float decalage_valeur = 0.0f;
	float echelle_valeur = 1.0f;

	/* restriction de la valeur de sortie */
	bool restreint = false;
	float restreint_neg = 0.0f;
	float restraint_pos = 1.0f;

	/* animation */
	float temps_anim = 0.0f;

	ondelette() = default;

	ondelette(ondelette const &) = default;
	ondelette &operator=(ondelette const &) = default;

	static void construit(parametres &parms, int graine);

	static float evalue(parametres const &params, dls::math::vec3f pos);

	static inline dls::math::vec2f limites()
	{
		return dls::math::vec2f(-1.0f, 1.0f);
	}
};

}  /* namespace bruit */
