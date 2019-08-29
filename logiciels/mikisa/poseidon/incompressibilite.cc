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

#include "incompressibilite.hh"

#include <tbb/parallel_reduce.h>

#include "biblinternes/moultfilage/boucle.hh"

#include "wolika/iteration.hh"

#include "fluide.hh"
#include "gradient_conjugue.hh"

/* Ce fichier contient du code provenant de Blender. Une version maison basée
 * sur [1] était implémentée mais contenant un bug m'étant alors
 * incompréhensible, et semblant se trouver dans la précondition du gradient
 * conjugué (GC).
 *
 * De plus, ce code est optimisé pour fusionner différentes boucles. La plupart
 * des implémentation (dont celle maison) utilisent différentes boucle pour
 * chaque opération du GC, ce qui est lent même pour des grilles à basse
 * résolution.
 *
 * L'exécution en parallèle des boucles vient de moi.
 *
 * [1] https://www.cs.ubc.ca/~rbridson/fluidsimulation/fluids_notes.pdf, page 34
 */

static auto calcul_divergence(
		wlk::GrilleMAC const &velocite,
		wlk::grille_dense_3d<int> const &drapeaux)
{
	auto divergence = wlk::grille_dense_3d<float>(velocite.desc());
	auto const res = velocite.desc().resolution;
	auto const taille_dalle = res.x * res.y;
	auto const dx = static_cast<float>(velocite.desc().taille_voxel);

	boucle_parallele(tbb::blocked_range<int>(1, res.z - 1),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (auto z = plage.begin(); z < plage.end(); ++z) {
			for (auto y = 1; y < res.y - 1; ++y) {
				for (auto x = 1; x < res.x - 1; ++x) {
					auto const index = x + (y + z * res.y) * res.x;

					if (est_obstacle(drapeaux, index)) {
						divergence.valeur(index) = 0.0f;
						continue;
					}

					float xright = velocite.valeur(index + 1).x;
					float xleft  = velocite.valeur(index - 1).x;
					float yup    = velocite.valeur(index + res.x).y;
					float ydown  = velocite.valeur(index - res.x).y;
					float ztop   = velocite.valeur(index + taille_dalle).z;
					float zbottom = velocite.valeur(index - taille_dalle).z;

					if (est_obstacle(drapeaux, index+1)) xright = - velocite.valeur(index).x; // DG: +=
					if (est_obstacle(drapeaux, index-1)) xleft  = - velocite.valeur(index).x;
					if (est_obstacle(drapeaux, index+res.x)) yup    = - velocite.valeur(index).y;
					if (est_obstacle(drapeaux, index-res.x)) ydown  = - velocite.valeur(index).y;
					if (est_obstacle(drapeaux, index+taille_dalle)) ztop    = - velocite.valeur(index).z;
					if (est_obstacle(drapeaux, index-taille_dalle)) zbottom = - velocite.valeur(index).z;

					//				if(_obstacles[index+1] & 8)			xright	+= _xVelocityOb.valeur(index + 1);
					//				if(_obstacles[index-1] & 8)			xleft	+= _xVelocityOb.valeur(index - 1);
					//				if(_obstacles[index+res.x] & 8)		yup		+= _yVelocityOb.valeur(index + res.x);
					//				if(_obstacles[index-res.x] & 8)		ydown	+= _yVelocityOb.valeur(index - res.x);
					//				if(_obstacles[index+taille_dalle] & 8) ztop    += _zVelocityOb.valeur(index + taille_dalle);
					//				if(_obstacles[index-taille_dalle] & 8) zbottom += _zVelocityOb.valeur(index - taille_dalle);

					divergence.valeur(index) = -dx * 0.5f * (
								xright - xleft +
								yup - ydown +
								ztop - zbottom );

					// Pressure is zero anyway since now a local array is used
					//				_pressure.valeur(index) = 0.0f;
				}
			}
		}
	});

	return divergence;
}

static auto resoud_pression(
		wlk::grille_dense_3d<float> &pression,
		wlk::grille_dense_3d<float> const &divergence,
		wlk::grille_dense_3d<int> const &drapeaux,
		int iterations,
		float precision)
{
	auto const res = pression.desc().resolution;
	auto const taille_dalle = res.x * res.y;

	auto residue   = wlk::grille_dense_3d<float>(pression.desc());
	auto direction = wlk::grille_dense_3d<float>(pression.desc());
	auto q         = wlk::grille_dense_3d<float>(pression.desc());
	auto h         = wlk::grille_dense_3d<float>(pression.desc());
	auto precond   = wlk::grille_dense_3d<float>(pression.desc());

	const int dalles[6] = {
		1, -1, res.x, -res.x, taille_dalle, -taille_dalle
	};

	auto calcul_delta_precond = [&](tbb::blocked_range<int> const &plage, float init)
	{
		auto delta = init;

		for (auto z = plage.begin(); z < plage.end(); ++z) {
			for (auto y = 1; y < res.y - 1; ++y) {
				for (auto x = 1; x < res.x - 1; ++x) {
					auto const index = x + (y + z * res.y) * res.x;

					if (est_obstacle(drapeaux, index)) {
						continue;
					}

					/* si la cellule est une variable */
					auto A_centre = 0.0f;

					/* renseigne la matrice pour le pochoir de Poisson dans l'ordre */
					for (auto d = 0; d < 6; ++d) {
						if (!est_obstacle(drapeaux, index + dalles[d])) {
							A_centre += 1.0f;
						}
					}

					if (A_centre < 1.0f) {
						continue;
					}

					auto r = divergence.valeur(index);
					r -= A_centre * pression.valeur(index);

					for (auto d = 0; d < 6; ++d) {
						if (!est_obstacle(drapeaux, index + dalles[d])) {
							r += pression.valeur(index + dalles[d]);
						}
					}

					/* P^-1 */
					auto const p = 1.0f / A_centre;

					/* p = P^-1 * r */
					auto const d = r * p;

					delta += r * d;

					direction.valeur(index) = d;
					precond.valeur(index) = p;
					residue.valeur(index) = r;
				}
			}
		}

		return delta;
	};

	auto nouveau_delta = tbb::parallel_reduce(
				tbb::blocked_range<int>(1, res.z - 1),
				0.0f,
				calcul_delta_precond,
				std::plus<float>());

	/* résoud r = b - Ax */

	auto const eps = precision;
	auto residue_max = 2.0f * eps;
	auto i = 0;

	while ((i < iterations) && (residue_max > 0.001f * eps)) {
		auto calcul_alpha_loc = [&](tbb::blocked_range<int> const &plage, float init)
		{
			auto alpha_loc = init;

			for (auto z = plage.begin(); z < plage.end(); ++z) {
				for (auto y = 1; y < res.y - 1; ++y) {
					for (auto x = 1; x < res.x - 1; ++x) {
						auto const index = x + (y + z * res.y) * res.x;

						if (est_obstacle(drapeaux, index)) {
							q.valeur(index) = 0.0f;
							continue;
						}

						/* si la cellule est une variable */
						auto A_centre = 0.0f;

						/* renseigne la matrice pour le pochoir de Poisson dans l'ordre */
						for (auto d = 0; d < 6; ++d) {
							if (!est_obstacle(drapeaux, index + dalles[d])) {
								A_centre += 1.0f;
							}
						}

						auto valeur_d = direction.valeur(index);
						auto valeur_q = A_centre * valeur_d;

						for (auto d = 0; d < 6; ++d) {
							if (!est_obstacle(drapeaux, index + dalles[d])) {
								valeur_q -= direction.valeur(index + dalles[d]);
							}
						}

						alpha_loc += valeur_d * valeur_q;

						q.valeur(index) = valeur_q;
					}
				}
			}

			return alpha_loc;
		};

		auto alpha = tbb::parallel_reduce(
					tbb::blocked_range<int>(1, res.z - 1),
					0.0f,
					calcul_alpha_loc,
					std::plus<float>());

		if (std::abs(alpha) > 0.0f) {
			alpha = nouveau_delta / alpha;
		}

		auto const ancien_delta = nouveau_delta;

		/* x = x + alpha * d */
		auto calcul_delta_max_r = [&](tbb::blocked_range<int> const &plage, std::pair<float, float> init)
		{
			auto delta = init.first;
			auto max_r = init.second;

			for (auto z = plage.begin(); z < plage.end(); ++z) {
				for (auto y = 1; y < res.y - 1; ++y) {
					for (auto x = 1; x < res.x - 1; ++x) {
						auto index = x + (y + z * res.y) * res.x;

						pression.valeur(index) += alpha * direction.valeur(index);

						residue.valeur(index) -= alpha * q.valeur(index);

						h.valeur(index) = precond.valeur(index) * residue.valeur(index);

						auto tmp = residue.valeur(index) * h.valeur(index);
						delta += tmp;
						max_r = (tmp > max_r) ? tmp : max_r;
					}
				}
			}

			return std::pair<float, float>(delta, max_r);
		};

		auto reduction_delta_max_r = [](std::pair<float, float> p1, std::pair<float, float> p2)
		{
			return std::pair(p1.first + p2.first, std::max(p1.second, p2.second));
		};

		auto p_dm = tbb::parallel_reduce(
					tbb::blocked_range<int>(1, res.z - 1),
					std::pair(0.0f, 0.0f),
					calcul_delta_max_r,
					reduction_delta_max_r);

		nouveau_delta = p_dm.first;
		residue_max = p_dm.second;

		auto const beta = nouveau_delta / ancien_delta;

		/* d = h + beta * d */
		boucle_parallele(tbb::blocked_range<int>(1, res.z - 1),
						 [&](tbb::blocked_range<int> const &plage)
		{
			for (auto z = plage.begin(); z < plage.end(); ++z) {
				for (auto y = 1; y < res.y - 1; ++y) {
					for (auto x = 1; x < res.x - 1; ++x) {
						auto const index = x + (y + z * res.y) * res.x;
						direction.valeur(index) = h.valeur(index) + beta * direction.valeur(index);
					}
				}
			}
		});

		i++;
	}

	std::cout << i << " iterations converged to " << std::sqrt(residue_max) << '\n';
}

static auto projette_solution(
		wlk::GrilleMAC &velocite,
		wlk::grille_dense_3d<float> &pression,
		wlk::grille_dense_3d<int> const &drapeaux)
{
	auto res = pression.desc().resolution;
	auto taille_dalle = res.x * res.y;
	auto dx_inv = static_cast<float>(1.0 / pression.desc().taille_voxel);

	boucle_parallele(tbb::blocked_range<int>(1, res.z - 1),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (auto z = plage.begin(); z < plage.end(); ++z) {
			for (auto y = 1; y < res.y - 1; ++y) {
				for (auto x = 1; x < res.x - 1; ++x) {
					auto const index = x + (y + z * res.y) * res.x;

					float vMask[3] = {1.0f, 1.0f, 1.0f}, vObst[3] = {0, 0, 0};

					auto p_centre   = pression.valeur(index);
					auto p_droite   = pression.valeur(index + 1);
					auto p_gauche   = pression.valeur(index - 1);
					auto p_devant   = pression.valeur(index + res.x);
					auto p_derriere = pression.valeur(index - res.x);
					auto p_dessus   = pression.valeur(index + taille_dalle);
					auto p_dessous  = pression.valeur(index - taille_dalle);

					if (!est_obstacle(drapeaux, index)) {
						// DG TODO: What if obstacle is left + right and one of them is moving?
						if (est_obstacle(drapeaux, index + 1)) {
							p_droite = p_centre;
							/*vObst[0] = _xVelocityOb.valeur(index + 1);*/
							vMask[0] = 0.0f;
						}

						if (est_obstacle(drapeaux, index - 1)) {
							p_gauche = p_centre;
							/*vObst[0]	= _xVelocityOb.valeur(index - 1);*/
							vMask[0] = 0.0f;
						}

						if (est_obstacle(drapeaux, index + res.x)) {
							p_devant = p_centre;
							/*vObst[1]	= _yVelocityOb.valeur(index + res.x);*/
							vMask[1] = 0.0f;
						}

						if (est_obstacle(drapeaux, index - res.x))		{
							p_derriere = p_centre;
							/*vObst[1]	= _yVelocityOb.valeur(index - res.x);*/
							vMask[1] = 0.0f;
						}

						if (est_obstacle(drapeaux, index + taille_dalle)) {
							p_dessus = p_centre;
							/*vObst[2] = _zVelocityOb.valeur(index + taille_dalle);*/
							vMask[2] = 0.0f;
						}

						if (est_obstacle(drapeaux, index - taille_dalle)) {
							p_dessous = p_centre;
							/*vObst[2]	= _zVelocityOb.valeur(index - taille_dalle);*/
							vMask[2] = 0.0f;
						}

						velocite.valeur(index).x -= 0.5f * (p_droite - p_gauche) * dx_inv;
						velocite.valeur(index).y -= 0.5f * (p_devant - p_derriere) * dx_inv;
						velocite.valeur(index).z -= 0.5f * (p_dessus - p_dessous) * dx_inv;

						velocite.valeur(index).x = (vMask[0] * velocite.valeur(index).x) + vObst[0];
						velocite.valeur(index).y = (vMask[1] * velocite.valeur(index).y) + vObst[1];
						velocite.valeur(index).z = (vMask[2] * velocite.valeur(index).z) + vObst[2];
					}
					else {
						//					_xVelocity.valeur(index) = _xVelocityOb.valeur(index);
						//					_yVelocity.valeur(index) = _yVelocityOb.valeur(index);
						//					_zVelocity.valeur(index) = _zVelocityOb.valeur(index);
					}
				}
			}
		}
	});
}

void projette_velocite(
		wlk::GrilleMAC &velocite,
		wlk::grille_dense_3d<float> &pression,
		wlk::grille_dense_3d<int> const &drapeaux,
		int iterations,
		float precision)
{
	auto divergence = calcul_divergence(velocite, drapeaux);

	resoud_pression(pression, divergence, drapeaux, iterations, precision);

	projette_solution(velocite, pression, drapeaux);
}

}  /* namespace psn */
