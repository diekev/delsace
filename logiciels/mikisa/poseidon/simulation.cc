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
		wlk::grille_dense_3d<float> &density,
		wlk::GrilleMAC &vel,
		wlk::grille_dense_3d<int> &flags,
		wlk::grille_dense_3d<float> *temperature,
		dls::math::vec3f const &gravity,
		float alpha,
		float beta,
		float temperature_ambiante,
		float dt,
		float coefficient)
{
	auto dx = static_cast<float>(density.desc().taille_voxel);
	auto f = -gravity * dt / dx * coefficient;

	auto res = flags.desc().resolution;

	auto const dalle_x = 1;
	auto const dalle_y = res.x;
	auto const dalle_z = res.x * res.y;

	boucle_parallele(tbb::blocked_range<int>(1, res.z),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto lims = limites3i{};
		lims.min = dls::math::vec3i(1, 1, plage.begin());
		lims.max = dls::math::vec3i(res.x, res.y, plage.end());

		auto iter = wlk::IteratricePosition(lims);

		while (!iter.fini()) {
			auto pos_iter = iter.suivante();
			auto idx = flags.calcul_index(pos_iter);

			if (flags.valeur(idx) != TypeFluid) {
				continue;
			}

			auto &v = vel.valeur(idx);

			if (flags.valeur(idx - dalle_x) == TypeFluid) {
				auto s = 0.5f * (density.valeur(idx) + density.valeur(idx - dalle_x));
				auto T = 0.0f;

				if (temperature) {
					T =  0.5f * (temperature->valeur(idx) + temperature->valeur(idx - dalle_x));
				}

				v.x += f.x * (-alpha * s + beta * (T - temperature_ambiante));
			}

			if (flags.valeur(idx - dalle_y) == TypeFluid) {
				auto s = 0.5f * (density.valeur(idx) + density.valeur(idx - dalle_y));
				auto T = 0.0f;

				if (temperature) {
					T =  0.5f * (temperature->valeur(idx) + temperature->valeur(idx - dalle_y));
				}

				v.y += f.y * (-alpha * s + beta * (T - temperature_ambiante));
			}

			if (flags.valeur(idx - dalle_z) == TypeFluid) {
				auto s = 0.5f * (density.valeur(idx) + density.valeur(idx - dalle_z));
				auto T = 0.0f;

				if (temperature) {
					T =  0.5f * (temperature->valeur(idx) + temperature->valeur(idx - dalle_z));
				}

				v.z += f.z * (-alpha * s + beta * (T - temperature_ambiante));
			}
		}
	});
}

void ajourne_conditions_bordures_murs(wlk::grille_dense_3d<int> &flags, wlk::GrilleMAC &vel)
{
	auto res = flags.desc().resolution;

	auto const dalle_x = 1;
	auto const dalle_y = res.x;
	auto const dalle_z = res.x * res.y;

	boucle_parallele(tbb::blocked_range<int>(0, res.z),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto lims = limites3i{};
		lims.min = dls::math::vec3i(0, 0, plage.begin());
		lims.max = dls::math::vec3i(res.x, res.y, plage.end());

		auto iter = wlk::IteratricePosition(lims);

		while (!iter.fini()) {
			auto pos_iter = iter.suivante();
			auto idx = flags.calcul_index(pos_iter);
			auto i = pos_iter.x;
			auto j = pos_iter.y;
			auto k = pos_iter.z;

			auto curFluid = flags.valeur(idx) == TypeFluid;
			auto curObs   = flags.valeur(idx) == TypeObstacle;
			auto bcsVel = dls::math::vec3f(0.0f, 0.0f, 0.0f);

			if (!curFluid && !curObs) {
				continue;
			}

			//	if (obvel) {
			//		bcsVel.x = (*obvel)(idx).x;
			//		bcsVel.y = (*obvel)(idx).y;
			//		if((*obvel).is3D()) bcsVel.z = (*obvel)(idx).z;
			//	}

			// we use i>0 instead of bnd=1 to check outer wall
			if (i > 0 && flags.valeur(idx - dalle_x) == TypeObstacle) {
				vel.valeur(idx).x = bcsVel.x;
			}

			if (i > 0 && curObs && flags.valeur(idx - dalle_x) == TypeFluid) {
				vel.valeur(idx).x = bcsVel.x;
			}

			if (j > 0 && flags.valeur(idx - dalle_y) == TypeObstacle) {
				vel.valeur(idx).y = bcsVel.y;
			}

			if (j > 0 && curObs && flags.valeur(idx - dalle_y) == TypeFluid) {
				vel.valeur(idx).y = bcsVel.y;
			}

			if (k > 0 && flags.valeur(idx - dalle_z) == TypeObstacle) {
				vel.valeur(idx).z = bcsVel.z;
			}

			if (k > 0 && curObs && flags.valeur(idx - dalle_z) == TypeFluid) {
				vel.valeur(idx).z = bcsVel.z;
			}

			if (curFluid) {
				if ((i > 0 && flags.valeur(idx - dalle_x) == TypeStick) || (i < (res.x - 1) && flags.valeur(idx + dalle_x) == TypeStick)) {
					vel.valeur(idx).y = vel.valeur(idx).z = 0.0f;
				}

				if ((j > 0 && flags.valeur(idx - dalle_y) == TypeStick) || (j < (res.y - 1) && flags.valeur(idx + dalle_y) == TypeStick)) {
					vel.valeur(idx).x = vel.valeur(idx).z = 0.0f;
				}

				if ((k > 0 && flags.valeur(idx - dalle_z) == TypeStick) || (k < (res.z - 1) && flags.valeur(idx + dalle_z) == TypeStick)) {
					vel.valeur(idx).x = vel.valeur(idx).y = 0.0f;
				}
			}
		}
	});
}

}  /* namespace psn */
