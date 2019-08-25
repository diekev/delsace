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

#include "vorticite.hh"

#include "fluide.hh"
#include "monde.hh"

namespace psn {

/* À FAIRE : instable. */
void ajoute_vorticite(
		Poseidon &poseidon,
		float quantite_vorticite,
		float vorticite_flamme,
		float dt)
{
	auto &drapeaux = *poseidon.drapeaux;
	auto &velocite = poseidon.velocite;
	/* À FAIRE */
	auto fioul = static_cast<wlk::grille_dense_3d<float> *>(nullptr);
	auto desc = velocite->desc();
	auto res = desc.resolution;
	auto dalle_x = 1;
	auto dalle_y = res.x;
	auto dalle_z = res.x + res.y;
	auto mi_dx_inv = static_cast<float>(0.5 / desc.taille_voxel);
	auto taill_voxel = static_cast<float>(desc.taille_voxel);
	auto dx_inv = static_cast<float>(1.0 / desc.taille_voxel);
	auto max_res = max(res);

	auto echelle_constante = 64.0 / static_cast<double>(max_res);
	echelle_constante = (echelle_constante < 1.0) ? 1.0 : echelle_constante;

	vorticite_flamme /= static_cast<float>(echelle_constante);

	if (quantite_vorticite + vorticite_flamme <= 0.0f) {
		return;
	}

	auto vorticite_x = wlk::grille_dense_3d<float>(desc);
	auto vorticite_y = wlk::grille_dense_3d<float>(desc);
	auto vorticite_z = wlk::grille_dense_3d<float>(desc);
	auto vorticite   = wlk::grille_dense_3d<float>(desc);

//	objvelocity[0] = _xVelocityOb;
//	objvelocity[1] = _yVelocityOb;
//	objvelocity[2] = _zVelocityOb;

	auto vort_vel = [&](int obpos[6], int i, int j)
	{
		/* obstacle animé */
//		if (_obstacles[obpos[(i)]] & 8) {
//			((abs(objvelocity[(j)][obpos[(i)]]) > FLT_EPSILON) ? objvelocity[(j)][obpos[(i)]] : velocity[(j)][index]);
//		}

		return velocite->valeur(obpos[i])[static_cast<size_t>(j)];
	};

	auto index = dalle_x + dalle_y + dalle_z;

	for (int z = 1; z < res.z - 1; z++, index += res.x * res.x) {
		for (int y = 1; y < res.y - 1; y++, index += 2) {
			for (int x = 1; x < res.x - 1; x++, index++) {
				if (est_obstacle(drapeaux, index)) {
					continue;
				}

				int obpos[6];

				obpos[0] = (est_obstacle(drapeaux, index + dalle_y)) ? index : index + dalle_y; // up
				obpos[1] = (est_obstacle(drapeaux, index - dalle_y)) ? index : index - dalle_y; // down
				float dy = (obpos[0] == index || obpos[1] == index) ? dx_inv : mi_dx_inv;

				obpos[2]  = (est_obstacle(drapeaux, index + dalle_z)) ? index : index + dalle_z; // out
				obpos[3]  = (est_obstacle(drapeaux, index - dalle_z)) ? index : index - dalle_z; // in
				float dz  = (obpos[2] == index || obpos[3] == index) ? dx_inv : mi_dx_inv;

				obpos[4] = (est_obstacle(drapeaux, index + dalle_x)) ? index : index + dalle_x; // right
				obpos[5] = (est_obstacle(drapeaux, index - dalle_x)) ? index : index - dalle_x; // left
				float dx = (obpos[4] == index || obpos[5] == index) ? dx_inv : mi_dx_inv;

				float xV[2], yV[2], zV[2];

				zV[1] = vort_vel(obpos, 0, 2);
				zV[0] = vort_vel(obpos, 1, 2);
				yV[1] = vort_vel(obpos, 2, 1);
				yV[0] = vort_vel(obpos, 3, 1);
				vorticite_x.valeur(index) = (zV[1] - zV[0]) * dy + (-yV[1] + yV[0]) * dz;

				xV[1] = vort_vel(obpos, 2, 0);
				xV[0] = vort_vel(obpos, 3, 0);
				zV[1] = vort_vel(obpos, 4, 2);
				zV[0] = vort_vel(obpos, 5, 2);
				vorticite_y.valeur(index) = (xV[1] - xV[0]) * dz + (-zV[1] + zV[0]) * dx;

				yV[1] = vort_vel(obpos, 4, 1);
				yV[0] = vort_vel(obpos, 5, 1);
				xV[1] = vort_vel(obpos, 0, 0);
				xV[0] = vort_vel(obpos, 1, 0);
				vorticite_z.valeur(index) = (yV[1] - yV[0]) * dx + (-xV[1] + xV[0])* dy;

				vorticite.valeur(index) = sqrtf(vorticite_x.valeur(index) * vorticite_x.valeur(index) +
												 vorticite_y.valeur(index) * vorticite_y.valeur(index) +
												 vorticite_z.valeur(index) * vorticite_z.valeur(index));
			}
		}
	}

	// calculate normalized vorticity vectors
	float eps = quantite_vorticite;

	index = dalle_x + dalle_y + dalle_z;

	for (int z = 1; z < res.z - 1; z++, index += res.x * res.x) {
		for (int y = 1; y < res.y - 1; y++, index += 2) {
			for (int x = 1; x < res.x - 1; x++, index++) {
				if (est_obstacle(drapeaux, index)) {
					continue;
				}

				float N[3];

				auto up    = (est_obstacle(drapeaux, index + dalle_y)) ? index : index + dalle_y;
				auto down  = (est_obstacle(drapeaux, index - dalle_y)) ? index : index - dalle_y;
				auto dy  = (up == index || down == index) ? dx_inv : mi_dx_inv;

				auto out   = (est_obstacle(drapeaux, index + dalle_z)) ? index : index + dalle_z;
				auto in    = (est_obstacle(drapeaux, index - dalle_z)) ? index : index - dalle_z;
				auto dz  = (out == index || in == index) ? dx_inv : mi_dx_inv;

				auto right = (est_obstacle(drapeaux, index + dalle_x)) ? index : index + dalle_x;
				auto left  = (est_obstacle(drapeaux, index - dalle_x)) ? index : index - dalle_x;
				auto dx  = (right == index || left == index) ? dx_inv : mi_dx_inv;

				N[0] = (vorticite.valeur(right) - vorticite.valeur(left)) * dx;
				N[1] = (vorticite.valeur(up) - vorticite.valeur(down)) * dy;
				N[2] = (vorticite.valeur(out) - vorticite.valeur(in)) * dz;

				auto magnitude = std::sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]);

				if (magnitude > std::numeric_limits<float>::epsilon()) {
					auto flame_vort = (fioul != nullptr) ? fioul->valeur(index) * vorticite_flamme : 0.0f;
					magnitude = 1.0f / magnitude;
					N[0] *= magnitude;
					N[1] *= magnitude;
					N[2] *= magnitude;

					auto &vel = velocite->valeur(index);

					vel.x += (N[1] * vorticite_z.valeur(index) - N[2] * vorticite_y.valeur(index)) * taill_voxel * (eps + flame_vort) * dt;
					vel.y += (N[2] * vorticite_x.valeur(index) - N[0] * vorticite_z.valeur(index)) * taill_voxel * (eps + flame_vort) * dt;
					vel.z += (N[0] * vorticite_y.valeur(index) - N[1] * vorticite_x.valeur(index)) * taill_voxel * (eps + flame_vort) * dt;
				}
			}
		}
	}
}

}  /* namespace psn */
