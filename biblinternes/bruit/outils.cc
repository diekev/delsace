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

#include "outils.hh"

namespace bruit {

/* Bruit 3D non-projetté. */
float evalue_tuile(float *tuile, int n, float p[3])
{
	int f[3], c[3], mid[3]; /* f, c = filter, noise coeff indices */
	float w[3][3];

	/* Evaluate quadratic B-spline basis functions */
	for (int i = 0; i < 3; i++) {
		mid[i] = static_cast<int>(std::ceil(p[i] - 0.5f));
		auto const t = static_cast<float>(mid[i]) - (p[i] - 0.5f);

		w[i][0] = t * t / 2.0f;
		w[i][2] = (1 - t) * (1 - t) / 2.0f;
		w[i][1]= 1.0f - w[i][0] - w[i][2];
	}

	/* Evaluate noise by weighting noise coefficients by basis function values */
	auto result = 0.0f;

	for (f[2] = -1; f[2] <= 1; f[2]++) {
		for (f[1] = -1; f[1] <= 1; f[1]++) {
			for (f[0] = -1; f[0] <= 1; f[0]++) {
				float weight = 1.0f;

				for (int i=0; i<3; i++) {
					c[i] = mod_lent(mid[i] + f[i], n);
					weight *= w[i][f[i] + 1];
				}

				result += weight * tuile[c[2]*n*n+c[1]*n+c[0]];
			}
		}
	}

	return result;
}

/* Bruit 3D projetté en 2D. */
float evalue_tuile_projette(float *tuile, int n, float p[3], float normal[3])
{
	int c[3], min[3], max[3]; /* c = noise coeff location */
	auto resultat = 0.0f;

	/* Borne le support des fonctions de bases pour la direction de cette projection. */
	for (auto i = 0; i < 3; i++) {
		auto support = 3.0f * std::abs(normal[i]) + 3.0f * std::sqrt((1.0f - normal[i] * normal[i]) / 2.0f);
		min[i] = static_cast<int>(std::ceil(p[i] - (support)));
		max[i] = static_cast<int>(std::floor(p[i] + (support)));
	}

	/* Boucle sur les coefficients à l'intérieur des bornes. */
	for (c[2] = min[2]; c[2] <= max[2]; c[2]++) {
		for (c[1] = min[1]; c[1] <= max[1]; c[1]++) {
			for (c[0] = min[0]; c[0] <= max[0]; c[0]++) {
				auto dot = 0.0f;
				auto poids = 1.0f;

				/* Produit scalaire du normal avec le vecteur allant de c à p. */
				for (auto i = 0; i < 3; i++) {
					dot += normal[i] * (p[i] - static_cast<float>(c[i]));
				}

				/* Évalue la fonction de base à c déplacé à mi-chemin vers p
				 * sur le normal. */
				for (auto i = 0; i < 3; i++) {
					auto t = (static_cast<float>(c[i]) + normal[i] * dot / 2.0f) - (p[i] - 1.5f);
					auto t1 = t - 1.0f;
					auto t2 = 2.0f - t;
					auto t3 = 3.0f - t;

					poids *= (t <= 0.0f || t >= 3.0f)
							? 0
							: (t < 1.0f)
							  ? t * t / 2.0f
							  : (t < 2.0f )
								? 1.0f - (t1 * t1 + t2 * t2) / 2.0f
								: t3 * t3 / 2.0f;
				}

				/* Évalue le bruit en pondérant les coefficients de bruit par
				 * les valeurs de la fonction de base. */
				resultat += poids * tuile[mod_lent(c[2], n) * n * n + mod_lent(c[1], n) * n + mod_lent(c[0],n)];
			}
		}
	}

	return resultat;
}

float evalue_multi_bande(
		float *tuile,
		int n,
		float p[3],
		float s,
		float *normal,
		int firstBand,
		int nbands,
		float *w)
{
	float q[3], resultat = 0.0f, variance = 0.0f;

	for (auto b = 0; b < nbands && s + static_cast<float>(firstBand + b) < 0.0f; b++) {
		for (auto i = 0; i <= 2; i++) {
			q[i] = 2.0f * p[i] * std::pow(2.0f, static_cast<float>(firstBand + b));
		}

		resultat += (normal) ? w[b] * evalue_tuile_projette(tuile, n, q, normal)
							 : w[b] * evalue_tuile(tuile, n, q);
	}

	for (auto b = 0; b < nbands; b++) {
		variance += w[b] * w[b];
	}

	/* Adjustement du bruit pour que sa variance soit de 1.0f. */
	if (variance != 0.0f) {
		resultat /= std::sqrt(variance * ((normal) ? 0.296f : 0.210f));
	}

	return resultat;
}

}  /* namespace bruit */
