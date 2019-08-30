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

#include "biblinternes/outils/gna.hh"

namespace bruit {

inline auto transforme_point(parametres const &params, dls::math::vec3f &p)
{
	/* mise à l'échelle du point d'échantillonage */
	p += params.decalage_graine;
	p *= params.echelle_pos;
	p += params.decalage_pos;
}

inline auto construit_defaut(parametres &params, int graine)
{
	/* nous pourrions avoir une table de permutation par bruit par graine, mais
	 * cela consommerait bien trop de mémoire donc nous utilisons un décalage
	 * aléatoire pour simuler différentes graines */
	auto gna = GNA{graine};
	auto rand_x = gna.uniforme(0.0f, 1.0f);
	auto rand_y = gna.uniforme(0.0f, 1.0f);
	auto rand_z = gna.uniforme(0.0f, 1.0f);

	params.decalage_graine = normalise(dls::math::vec3f(rand_x, rand_y, rand_z));
}

/* fonctions d'évaluation pour les bruits ondelette et fourier */

inline auto mod_lent(int x, int n)
{
	int m = x % n;
	return (m < 0) ? m + n : m;
}

inline auto mod_rapide_128(int n)
{
	return n & 127;
}

/* Bruit 3D non-projetté. */
float evalue_tuile(float *tuile, int n, float p[3]);

/* Bruit 3D projetté en 2D. */
float evalue_tuile_projette(float *tuile, int n, float p[3], float normal[3]);

float evalue_multi_bande(
		float *tuile,
		int n,
		float p[3],
		float s,
		float *normal,
		int firstBand,
		int nbands,
		float *w);

}  /* namespace bruit */
