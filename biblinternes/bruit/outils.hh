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

inline auto transforme_valeur(parametres const &params, float &v)
{
	/* mise à l'échelle de la valeur échantillonée */
	v *= params.echelle_valeur;
	v += params.decalage_valeur;
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

/* dérivées bruits tuiles */

inline float evalue_derivee_tuile_x(float *tuile, int n, float p[3])
{
	int c[3], mid[3];
	float w[3][3], t, result = 0;

	mid[0] = static_cast<int>(std::ceil(p[0] - 0.5f));
	t = static_cast<float>(mid[0]) - (p[0] - 0.5f);
	w[0][0] = -t;
	w[0][2] = (1.f - t);
	w[0][1] = 2.0f * t - 1.0f;

	mid[1] = static_cast<int>(std::ceil(p[1] - 0.5f));
	t = static_cast<float>(mid[1]) - (p[1] - 0.5f);
	w[1][0] = t * t / 2;
	w[1][2] = (1 - t) * (1 - t) / 2;
	w[1][1] = 1 - w[1][0] - w[1][2];

	mid[2] = static_cast<int>(std::ceil(p[2] - 0.5f));
	t = static_cast<float>(mid[2]) - (p[2] - 0.5f);
	w[2][0] = t * t / 2;
	w[2][2] = (1 - t) * (1 - t)/2;
	w[2][1] = 1 - w[2][0] - w[2][2];

	// to optimize, explicitly unroll this loop
	for (int z = -1; z <=1; z++) {
		for (int y = -1; y <=1; y++) {
			for (int x = -1; x <=1; x++) {
				float weight = 1.0f;
				c[0] = mod_rapide_128(mid[0] + x);
				weight *= w[0][x+1];
				c[1] = mod_rapide_128(mid[1] + y);
				weight *= w[1][y+1];
				c[2] = mod_rapide_128(mid[2] + z);
				weight *= w[2][z+1];
				result += weight * tuile[c[2]*n*n+c[1]*n+c[0]];
			}
		}
	}
	return result;
}

inline float evalue_derivee_tuile_y(float *tuile, int n, float p[3])
{
	int c[3], mid[3];
	float w[3][3], t, result =0;

	mid[0] = static_cast<int>(std::ceil(p[0] - 0.5f));
	t = static_cast<float>(mid[0])-(p[0] - 0.5f);
	w[0][0] = t * t / 2;
	w[0][2] = (1 - t) * (1 - t) / 2;
	w[0][1] = 1 - w[0][0] - w[0][2];

	mid[1] = static_cast<int>(std::ceil(p[1] - 0.5f));
	t = static_cast<float>(mid[1])-(p[1] - 0.5f);
	w[1][0] = -t;
	w[1][2] = (1.f - t);
	w[1][1] = 2.0f * t - 1.0f;

	mid[2] = static_cast<int>(std::ceil(p[2] - 0.5f));
	t = static_cast<float>(mid[2]) - (p[2] - 0.5f);
	w[2][0] = t * t / 2;
	w[2][2] = (1 - t) * (1 - t)/2;
	w[2][1] = 1 - w[2][0] - w[2][2];

	// to optimize, explicitly unroll this loop
	for (int z = -1; z <=1; z++) {
		for (int y = -1; y <=1; y++) {
			for (int x = -1; x <=1; x++) {
				float weight = 1.0f;
				c[0] = mod_rapide_128(mid[0] + x);
				weight *= w[0][x+1];
				c[1] = mod_rapide_128(mid[1] + y);
				weight *= w[1][y+1];
				c[2] = mod_rapide_128(mid[2] + z);
				weight *= w[2][z+1];
				result += weight * tuile[c[2]*n*n+c[1]*n+c[0]];
			}
		}
	}

	return result;
}

inline float evalue_derivee_tuile_z(float *tuile, int n, float p[3])
{
	int c[3], mid[3];
	float w[3][3], t, result =0;

	mid[0] = static_cast<int>(std::ceil(p[0] - 0.5f));
	t = static_cast<float>(mid[0])-(p[0] - 0.5f);
	w[0][0] = t * t / 2;
	w[0][2] = (1 - t) * (1 - t) / 2;
	w[0][1] = 1 - w[0][0] - w[0][2];

	mid[1] = static_cast<int>(std::ceil(p[1] - 0.5f));
	t = static_cast<float>(mid[1])-(p[1] - 0.5f);
	w[1][0] = t * t / 2;
	w[1][2] = (1 - t) * (1 - t) / 2;
	w[1][1] = 1 - w[1][0] - w[1][2];

	mid[2] = static_cast<int>(std::ceil(p[2] - 0.5f));
	t = static_cast<float>(mid[2]) - (p[2] - 0.5f);
	w[2][0] = -t;
	w[2][2] = (1.f - t);
	w[2][1] = 2.0f * t - 1.0f;

	// to optimize, explicitly unroll this loop
	for (int z = -1; z <=1; z++) {
		for (int y = -1; y <=1; y++) {
			for (int x = -1; x <=1; x++) {
				float weight = 1.0f;
				c[0] = mod_rapide_128(mid[0] + x);
				weight *= w[0][x+1];
				c[1] = mod_rapide_128(mid[1] + y);
				weight *= w[1][y+1];
				c[2] = mod_rapide_128(mid[2] + z);
				weight *= w[2][z+1];
				result += weight * tuile[c[2]*n*n+c[1]*n+c[0]];
			}
		}
	}

	return result;
}

}  /* namespace bruit */
