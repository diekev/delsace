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

#include "biblinternes/math/outils.hh"

#include "grille_eparse.hh"

namespace wlk {

/**
 * Implémentation de volume temporel basée sur
 * « Efficient Rendering of Volumetric Motion Blur using Temporally Unstructured Volumes »
 * par Magnus Wrenninge, Pixar Animation Studio.
 * http://magnuswrenninge.com/wp-content/uploads/2010/03/Wrenninge-EfficientRenderingOfVolumetricMotionBlur.pdf
 *
 * Ce type de volume est utile pour rendre le flou directionel ou encore
 * réanimer les simumlations de fluide.
 */

struct paire_valeur_temps {
	float valeur{};
	float temps{};
};

using type_courbe = dls::tableau<paire_valeur_temps>;

using grille_auxilliaire = wlk::grille_eparse<type_courbe>;

struct tuile_temporelle {
	using type_valeur = float;

	int decalage[VOXELS_TUILE + 1];
	float *valeurs = nullptr;
	float *temps = nullptr;
	dls::math::vec3i min{};
	dls::math::vec3i max{};

	static void detruit(tuile_temporelle *&t)
	{
		auto nombre_valeurs = t->decalage[VOXELS_TUILE];
		memoire::deloge_tableau("tuile_temp::valeurs", t->valeurs, nombre_valeurs);
		memoire::deloge_tableau("tuile_temp::temps", t->temps, nombre_valeurs);
		memoire::deloge("tuile", t);
	}

	static type_valeur echantillonne(tuile_temporelle *t, long index, float temps)
	{
		/* voisin le plus proche */
		auto deb = t->decalage[index];
		auto dec = t->decalage[index + 1] - deb;

		auto valeur = 0.0f;

		/* trouve le temps */
		for (auto x = deb; x < deb + dec; ++x) {
			/* temps exacte */
			if (dls::math::sont_environ_egaux(t->temps[x], temps)) {
				valeur = t->valeurs[x];
				break;
			}

			/* interpole depuis le premier temps le plus grand */
			if (t->temps[x] >= temps) {
				if (x == deb) {
					valeur = t->valeurs[x];
					break;
				}

				/* fait en sorte que t soit entre 0 et 1 */
				auto fac = (temps - t->temps[x - 1]) / (t->temps[x] - t->temps[x - 1]);
				valeur = t->valeurs[x] * fac + (1.0f - fac) * t->valeurs[x - 1];
				break;
			}

			/* cas où le temps d'échantillonnage est supérieur au
			 * dernier temps */
			if (temps > t->temps[x] && x == deb + dec - 1) {
				valeur = t->valeurs[x];
			}
		}

		return valeur;
	}
};

using grille_temporelle = grille_eparse<float, tuile_temporelle>;

}  /* namespace wlk */
