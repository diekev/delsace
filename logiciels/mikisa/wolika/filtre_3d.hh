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

#include "iteration.hh"

namespace wlk {

template <typename T, typename type_tuile>
auto floute_volume(
		grille_eparse<T, type_tuile> const &grille_entree,
		int taille_fenetre)
{
	auto grille = memoire::loge<wlk::grille_eparse<T>>("wlk::grille_eparse", grille_entree.desc());
	grille->assure_tuiles(grille_entree.desc().etendue);

	wlk::pour_chaque_tuile_parallele(grille_entree, [&](wlk::tuile_scalaire<T> const *tuile)
	{
		auto min_tuile = tuile->min / wlk::TAILLE_TUILE;
		auto idx_tuile = dls::math::calcul_index(min_tuile, grille_entree.res_tuile());
		auto tuile_b = grille->tuile_par_index(idx_tuile);

		auto index_tuile = 0;
		for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
			for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
				for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
					auto pos_tuile = tuile->min;
					pos_tuile.x += i;
					pos_tuile.y += j;
					pos_tuile.z += k;

					auto valeur = 0.0f;
					auto poids = 0.0f;

					for (auto kk = k - taille_fenetre; kk < k + taille_fenetre; ++kk) {
						for (auto jj = j - taille_fenetre; jj < j + taille_fenetre; ++jj) {
							for (auto ii = i - taille_fenetre; ii < i + taille_fenetre; ++ii) {
								valeur += grille_entree.valeur(ii, jj, kk);
								poids += 1.0f;
							}
						}
					}

					tuile_b->donnees[index_tuile] = valeur / poids;
				}
			}
		}
	});

	grille->elague();

	return grille;
}

template <typename T, typename type_tuile>
auto affine_volume(
		grille_eparse<T, type_tuile> const &grille_entree,
		int taille_fenetre,
		T const &poids_affinage)
{
	auto grille = memoire::loge<wlk::grille_eparse<T>>("wlk::grille_eparse", grille_entree.desc());
	grille->assure_tuiles(grille_entree.desc().etendue);

	wlk::pour_chaque_tuile_parallele(grille_entree, [&](wlk::tuile_scalaire<T> const *tuile)
	{
		auto min_tuile = tuile->min / wlk::TAILLE_TUILE;
		auto idx_tuile = dls::math::calcul_index(min_tuile, grille_entree.res_tuile());
		auto tuile_b = grille->tuile_par_index(idx_tuile);

		auto index_tuile = 0;
		for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
			for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
				for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
					auto pos_tuile = tuile->min;
					pos_tuile.x += i;
					pos_tuile.y += j;
					pos_tuile.z += k;

					auto valeur = 0.0f;
					auto poids = 0.0f;

					for (auto kk = k - taille_fenetre; kk < k + taille_fenetre; ++kk) {
						for (auto jj = j - taille_fenetre; jj < j + taille_fenetre; ++jj) {
							for (auto ii = i - taille_fenetre; ii < i + taille_fenetre; ++ii) {
								valeur += grille_entree.valeur(ii, jj, kk);
								poids += 1.0f;
							}
						}
					}

					auto valeur_orig = tuile->donnees[index_tuile];
					auto valeur_grossiere = valeur / poids;
					auto valeur_fine = valeur_orig - valeur_grossiere;
					auto valeur_affinee = valeur_orig + valeur_fine * poids_affinage;

					tuile_b->donnees[index_tuile] = valeur_affinee;
				}
			}
		}
	});

	grille->elague();

	return grille;
}

}  /* namespace wlk */
