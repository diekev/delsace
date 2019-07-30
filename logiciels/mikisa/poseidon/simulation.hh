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

#include "biblinternes/moultfilage/boucle.hh"

#include "corps/corps.h"
#include "corps/iter_volume.hh"
#include "corps/volume.hh"

#include "fluide.hh"

namespace psn {

/* ************************************************************************** */

template <typename T>
static auto SemiLagrange(
		Grille<int> *flags,
		GrilleMAC *vel,
		Grille<T> &fwd,
		Grille<T> const &orig,
		float dt)
{
	auto res = flags->resolution();
	auto echant = Echantilloneuse(orig);

	boucle_parallele(tbb::blocked_range<int>(0, res.z - 1),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto lims = limites3i{};
		lims.min = dls::math::vec3i(0, 0, plage.begin());
		lims.max = dls::math::vec3i(res.x - 1, res.y - 1, plage.end());

		auto iter = IteratricePosition(lims);

		while (!iter.fini()) {
			auto pos_iter = iter.suivante();

			auto i = pos_iter.x;
			auto j = pos_iter.y;
			auto k = pos_iter.z;

			auto pos = dls::math::vec3f(
						static_cast<float>(i) + 0.5f,
						static_cast<float>(j) + 0.5f,
						static_cast<float>(k) + 0.5f);

			auto v = vel->valeur_centree(pos_iter);
			v *= dt;

			pos -= v;

			fwd.valeur(pos_iter, echant.echantillone_trilineaire(pos));
		}
	});
}

template <typename T>
auto advecte_semi_lagrange(
		Grille<int> *flags,
		GrilleMAC *vel,
		Grille<T> *orig,
		float dt,
		int order)
{
	auto fwd = Grille<T>(flags->etendu(), flags->fenetre_donnees(), flags->taille_voxel());

	SemiLagrange(flags, vel, fwd, *orig, dt);

	if (order == 1) {
		orig->echange(fwd);
	}
	/* MacCormack */
	else if (order == 2) {
		auto bwd = Grille<T>(flags->etendu(), flags->fenetre_donnees(), flags->taille_voxel());
		auto newGrid = Grille<T>(flags->etendu(), flags->fenetre_donnees(), flags->taille_voxel());

		// bwd <- backwards step
		SemiLagrange(flags, vel, bwd, fwd, -dt/*, levelset, orderSpace*/);

		// newGrid <- compute correction
		//MacCormackCorrect(flags, newGrid, orig, fwd, bwd/*, strength, levelset*/);

		// clamp values
		//MacCormackClamp(flags, vel, newGrid, orig, fwd, dt/*, clampMode*/);

		orig->echange(newGrid);
	}
}

void ajoute_flottance(
		Grille<float> *density,
		GrilleMAC *vel,
		Grille<int> *flags,
		dls::math::vec3f const &gravity,
		float dt,
		float coefficient);

void ajourne_conditions_bordures_murs(
		Grille<int> *flags,
		GrilleMAC *vel);

}  /* namespace psn */
