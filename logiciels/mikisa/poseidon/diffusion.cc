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

#include "diffusion.hh"

#include "fluide.hh"

namespace psn {

void diffuse(
		wlk::grille_dense_3d<float> &champs,
		wlk::grille_dense_3d<float> &b,
		wlk::grille_dense_3d<int> &drapeaux,
		int iterations,
		float precision,
		float diffusion,
		float dt)
{
	auto const res = champs.desc().resolution;
	auto const dx = champs.desc().taille_voxel;
	auto const taille_dalle = res.x * res.y;

	auto const twoxr = 2 * res.x;
	auto const valeur = dt * diffusion / static_cast<float>(dx * dx);

	auto residue   = wlk::grille_dense_3d<float>(champs.desc());
	auto direction = wlk::grille_dense_3d<float>(champs.desc());
	auto q         = wlk::grille_dense_3d<float>(champs.desc());
	auto A_centre  = wlk::grille_dense_3d<float>(champs.desc());

	const int dalles[6] = {
		1, -1, res.x, -res.x, taille_dalle, -taille_dalle
	};

	auto nouveau_delta = 0.0f;

	// r = b - Ax
	auto index = taille_dalle + res.x + 1;
	for (auto z = 1; z < res.z - 1; z++, index += twoxr) {
		for (auto y = 1; y < res.y - 1; y++, index += 2) {
			for (auto x = 1; x < res.x - 1; x++, index++) {
				auto valeur_A = 1.0f;

				if (est_obstacle(drapeaux, index)) {
					residue.valeur(index) = 0.0f;
					A_centre.valeur(index) = valeur_A;
					continue;
				}

				/* si la cellule est une variable */

				/* renseigne la matrice pour le pochoir de Poisson dans l'ordre */
				for (auto d = 0; d < 6; ++d) {
					if (est_obstacle(drapeaux, index + dalles[d])) {
						valeur_A += valeur;
					}
				}

				auto r = b.valeur(index) - valeur_A * champs.valeur(index);

				for (auto d = 0; d < 6; ++d) {
					if (est_obstacle(drapeaux, index + dalles[d])) {
						r -= champs.valeur(index + dalles[d]) * valeur;
					}
				}

				A_centre.valeur(index) = valeur_A;
				direction.valeur(index) = r;
				residue.valeur(index) = r;
				nouveau_delta += r * r;
			}
		}
	}


	// While deltaNew > (eps^2) * delta0
	const float eps  = precision;
	float max_r = 2.0f * eps;
	int i = 0;

	while ((i < iterations) && (max_r > eps)) {
		// q = Ad
		float alpha = 0.0f;

		index = taille_dalle + res.x + 1;
		for (auto z = 1; z < res.z - 1; z++, index += twoxr) {
			for (auto y = 1; y < res.y - 1; y++, index += 2) {
				for (auto x = 1; x < res.x - 1; x++, index++) {
					if (est_obstacle(drapeaux, index)) {
						q.valeur(index) = 0.0f;
						continue;
					}

					/* si la cellule est une variable */

					auto valeur_d = direction.valeur(index);
					auto valeur_q = A_centre.valeur(index) * valeur_d;

					/* renseigne la matrice pour le pochoir de Poisson dans l'ordre */
					for (auto d = 0; d < 6; ++d) {
						if (est_obstacle(drapeaux, index + dalles[d])) {
							valeur_q -= direction.valeur(index + dalles[d]) * valeur;
						}
					}

					alpha += valeur_d * valeur_q;
					q.valeur(index) = valeur_q;
				}
			}
		}

		if (std::abs(alpha) > 0.0f) {
			alpha = nouveau_delta / alpha;
		}

		auto const ancien_delta = nouveau_delta;
		nouveau_delta = 0.0f;

		max_r = 0.0f;

		index = taille_dalle + res.x + 1;
		for (auto z = 1; z < res.z - 1; z++, index += twoxr){
			for (auto y = 1; y < res.y - 1; y++, index += 2){
				for (auto x = 1; x < res.x - 1; x++, index++) {
					champs.valeur(index) += alpha * direction.valeur(index);

					residue.valeur(index) -= alpha * q.valeur(index);
					max_r = (residue.valeur(index) > max_r) ? residue.valeur(index) : max_r;

					nouveau_delta += residue.valeur(index) * residue.valeur(index);
				}
			}
		}

		auto const beta = nouveau_delta / ancien_delta;

		index = taille_dalle + res.x + 1;
		for (auto z = 1; z < res.z - 1; z++, index += twoxr) {
			for (auto y = 1; y < res.y - 1; y++, index += 2) {
				for (auto x = 1; x < res.x - 1; x++, index++) {
					direction.valeur(index) = residue.valeur(index) + beta * direction.valeur(index);
				}
			}
		}

		i++;
	}

	std::cout << "Diffusion : " << i << " iterations converged to " << max_r << std::endl;
}

}  /* namespace psn */
