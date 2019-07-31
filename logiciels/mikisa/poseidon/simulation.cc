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

#include "simulation.hh"

namespace psn {

void ajoute_flottance(
		Grille<float> &density,
		GrilleMAC &vel,
		Grille<int> &flags,
		dls::math::vec3f const &gravity,
		float dt,
		float coefficient)
{
	auto dx = density.taille_voxel();
	auto f = -gravity * dt / dx * coefficient;

	auto res = flags.resolution();

	boucle_parallele(tbb::blocked_range<int>(1, res.z),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto lims = limites3i{};
		lims.min = dls::math::vec3i(1, 1, plage.begin());
		lims.max = dls::math::vec3i(res.x, res.y, plage.end());

		auto iter = IteratricePosition(lims);

		while (!iter.fini()) {
			auto pos_iter = iter.suivante();
			auto i = static_cast<size_t>(pos_iter.x);
			auto j = static_cast<size_t>(pos_iter.y);
			auto k = static_cast<size_t>(pos_iter.z);

			if (flags.valeur(i, j, k) != TypeFluid) {
				continue;
			}

			auto &v = vel.valeur(i, j, k);

			if (flags.valeur(i - 1, j, k) == TypeFluid) {
				v.x += (0.5f * f.x) * (density.valeur(i, j, k) + density.valeur(i - 1, j, k));
			}

			if (flags.valeur(i, j - 1, k) == TypeFluid) {
				v.y += (0.5f * f.y) * (density.valeur(i, j, k) + density.valeur(i, j - 1, k));
			}

			if (flags.valeur(i, j, k - 1) == TypeFluid) {
				v.z += (0.5f * f.z) * (density.valeur(i, j, k) + density.valeur(i, j, k - 1));
			}
		}
	});
}

void ajourne_conditions_bordures_murs(Grille<int> &flags, GrilleMAC &vel)
{
	auto res = flags.resolution();

	boucle_parallele(tbb::blocked_range<int>(0, res.z),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto lims = limites3i{};
		lims.min = dls::math::vec3i(0, 0, plage.begin());
		lims.max = dls::math::vec3i(res.x, res.y, plage.end());

		auto iter = IteratricePosition(lims);

		while (!iter.fini()) {
			auto pos_iter = iter.suivante();
			auto i = static_cast<size_t>(pos_iter.x);
			auto j = static_cast<size_t>(pos_iter.y);
			auto k = static_cast<size_t>(pos_iter.z);

			auto curFluid = flags.valeur(i, j, k) == TypeFluid;
			auto curObs   = flags.valeur(i, j, k) == TypeObstacle;
			auto bcsVel = dls::math::vec3f(0.0f, 0.0f, 0.0f);

			if (!curFluid && !curObs) {
				continue;
			}

			//	if (obvel) {
			//		bcsVel.x = (*obvel)(i,j,k).x;
			//		bcsVel.y = (*obvel)(i,j,k).y;
			//		if((*obvel).is3D()) bcsVel.z = (*obvel)(i,j,k).z;
			//	}

			// we use i>0 instead of bnd=1 to check outer wall
			if (i > 0 && flags.valeur(i-1,j,k) == TypeObstacle) {
				vel.valeur(i,j,k).x = bcsVel.x;
			}

			if (i > 0 && curObs && flags.valeur(i-1,j,k) == TypeFluid) {
				vel.valeur(i,j,k).x = bcsVel.x;
			}

			if (j > 0 && flags.valeur(i,j-1,k) == TypeObstacle) {
				vel.valeur(i,j,k).y = bcsVel.y;
			}

			if (j > 0 && curObs && flags.valeur(i,j-1,k) == TypeFluid) {
				vel.valeur(i,j,k).y = bcsVel.y;
			}

			if (k > 0 && flags.valeur(i,j,k-1) == TypeObstacle) {
				vel.valeur(i,j,k).z = bcsVel.z;
			}

			if (k > 0 && curObs && flags.valeur(i,j,k-1) == TypeFluid) {
				vel.valeur(i,j,k).z = bcsVel.z;
			}

			if (curFluid) {
				if ((i > 0 && flags.valeur(i - 1, j, k) == TypeStick) || (i < static_cast<size_t>(res.x - 1) && flags.valeur(i+1,j,k) == TypeStick)) {
					vel.valeur(i,j,k).y = vel.valeur(i,j,k).z = 0.0f;
				}

				if ((j > 0 && flags.valeur(i, j - 1, k) == TypeStick) || (j < static_cast<size_t>(res.y - 1) && flags.valeur(i,j+1,k) == TypeStick)) {
					vel.valeur(i,j,k).x = vel.valeur(i,j,k).z = 0.0f;
				}

				if ((k > 0 && flags.valeur(i, j, k - 1) == TypeStick) || (k < static_cast<size_t>(res.z - 1) && flags.valeur(i,j,k+1) == TypeStick)) {
					vel.valeur(i,j,k).x = vel.valeur(i,j,k).y = 0.0f;
				}
			}
		}
	});
}

}  /* namespace psn */
