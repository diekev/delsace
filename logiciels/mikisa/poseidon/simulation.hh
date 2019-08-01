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
#include "corps/echantillonnage_volume.hh"
#include "corps/iter_volume.hh"
#include "corps/volume.hh"

#include "fluide.hh"

namespace dls::math {

template <typename T>
inline auto extrait_min_max(T const v, T &min, T &max)
{
	if (v < min) {
		min = v;
	}
	if (v > max) {
		max = v;
	}
}

}

namespace psn {

/* ************************************************************************** */

template <typename T>
auto advection_semi_lagrange(
		GrilleMAC &vel,
		Grille<T> &fwd,
		Grille<T> const &orig,
		float dt)
{
	auto res = orig.resolution();
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

			/* Dû au découplage possible il faut nous assurer que la position
			 * corresponde au niveau mondial. */
			auto pos_mnd_grille = orig.index_vers_monde(pos_iter);
			auto pos_vec = vel.monde_vers_index(pos_mnd_grille);

			auto v = vel.valeur_centree(pos_vec);
			v *= dt;

			auto i = pos_iter.x;
			auto j = pos_iter.y;
			auto k = pos_iter.z;

			auto pos = dls::math::vec3f(
						static_cast<float>(i) + 0.5f,
						static_cast<float>(j) + 0.5f,
						static_cast<float>(k) + 0.5f);

			pos -= v;

			fwd.valeur(pos_iter, echant.echantillone_trilineaire(pos));
		}
	});
}

//! Kernel: Correct based on forward and backward SL steps (for both centered & mac grids)
template<class T>
void MacCormackCorrect(
		const Grille<int>& flags,
		Grille<T>& dst,
		const Grille<T>& old,
		const Grille<T>& fwd,
		const Grille<T>& bwd,
		float strength)
{
	for (auto idx = 0; idx < flags.nombre_voxels(); ++idx) {
		dst.valeur(idx) = fwd.valeur(idx);

		if (est_fluide(flags, idx)) {
			// only correct inside fluid region; note, strenth of correction can be modified here
			dst.valeur(idx) += strength * 0.5f * (old.valeur(idx) - bwd.valeur(idx));
		}
	}
}

//! detect out of bounds value
template<class T> inline bool cmpMinMax(T& minv, T& maxv, const T& val) {
	if (val < minv) return true;
	if (val > maxv) return true;
	return false;
}
template<>
inline bool cmpMinMax<dls::math::vec3f>(dls::math::vec3f& minv, dls::math::vec3f& maxv, const dls::math::vec3f& val)
{
	return( cmpMinMax(minv.x, maxv.x, val.x) | cmpMinMax(minv.y, maxv.y, val.y) | cmpMinMax(minv.z, maxv.z, val.z));
}

#define checkFlag(x,y,z) (flags.valeur((x),(y),(z)) & (TypeFluid|TypeVide))

template<class T>
inline T doClampComponent(
		const dls::math::vec3i& gridSize,
		const Grille<int>& flags,
		T dst,
		const Grille<T>& orig,
		const T fwd,
		const dls::math::vec3f& pos,
		const dls::math::vec3f& vel,
		const int clampMode )
{
	T minv( std::numeric_limits<T>::max()), maxv( -std::numeric_limits<T>::max());
	bool haveFl = false;

	// forward (and optionally) backward
	dls::math::vec3i positions[2];
	int numPos = 1;
	positions[0] = dls::math::continu_vers_discret<int>(pos - vel);

	if(clampMode==1) {
		numPos = 2;
		positions[1] = dls::math::continu_vers_discret<int>(pos + vel);
	}

	for(int l=0; l<numPos; ++l) {
		auto& currPos = positions[l];

		// clamp lookup to grid
		const int i0 = dls::math::restreint(currPos.x, 0, gridSize.x-1); // note! gridsize already has -1 from call
		const int j0 = dls::math::restreint(currPos.y, 0, gridSize.y-1);
		const int k0 = dls::math::restreint(currPos.z, 0, gridSize.z-1);
		const int i1 = i0+1, j1 = j0+1, k1= (k0+1);

		// find min/max around source pos
		if(checkFlag(i0,j0,k0)) { dls::math::extrait_min_max(orig.valeur(i0,j0,k0), minv, maxv);  haveFl=true; }
		if(checkFlag(i1,j0,k0)) { dls::math::extrait_min_max(orig.valeur(i1,j0,k0), minv, maxv);  haveFl=true; }
		if(checkFlag(i0,j1,k0)) { dls::math::extrait_min_max(orig.valeur(i0,j1,k0), minv, maxv);  haveFl=true; }
		if(checkFlag(i1,j1,k0)) { dls::math::extrait_min_max(orig.valeur(i1,j1,k0), minv, maxv);  haveFl=true; }

		if(checkFlag(i0,j0,k1)) { dls::math::extrait_min_max(orig.valeur(i0,j0,k1), minv, maxv); haveFl=true; }
		if(checkFlag(i1,j0,k1)) { dls::math::extrait_min_max(orig.valeur(i1,j0,k1), minv, maxv); haveFl=true; }
		if(checkFlag(i0,j1,k1)) { dls::math::extrait_min_max(orig.valeur(i0,j1,k1), minv, maxv); haveFl=true; }
		if(checkFlag(i1,j1,k1)) { dls::math::extrait_min_max(orig.valeur(i1,j1,k1), minv, maxv); haveFl=true; }
	}

	if(!haveFl) return fwd;
	if(clampMode==1) {
		dst = dls::math::restreint(dst, minv, maxv); // hard clamp
	} else {
		if(cmpMinMax(minv,maxv,dst)) dst = fwd; // recommended in paper, "softer"
	}
	return dst;
}

template <typename T>
auto restriction_maccormarck(
		const Grille<int>& flags,
		const GrilleMAC& vel,
		Grille<T>& dst,
		const Grille<T>& orig,
		const Grille<T>& fwd,
		float dt,
		const int clampMode)
{
	auto res = flags.resolution();
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

			auto i = static_cast<size_t>(pos_iter.x);
			auto j = static_cast<size_t>(pos_iter.y);
			auto k = static_cast<size_t>(pos_iter.z);

			auto pos_cont = dls::math::vec3f(i,j,k);

			auto dval       = dst.valeur(i,j,k);
			auto gridUpper  = flags.resolution() - dls::math::vec3i(1);

			dval = doClampComponent<T>(gridUpper, flags, dval, orig, fwd.valeur(i,j,k), pos_cont, vel.valeur_centree(pos_iter) * dt, clampMode );

			if (clampMode == 1) {
				// lookup forward/backward , round to closest NB
				auto posFwd = dls::math::continu_vers_discret<int>(pos_cont + dls::math::vec3f(0.5f) - vel.valeur_centree(pos_iter) * dt );
				auto posBwd = dls::math::continu_vers_discret<int>(pos_cont + dls::math::vec3f(0.5f) + vel.valeur_centree(pos_iter) * dt );

				// test if lookups point out of grid or into obstacle (note doClampComponent already checks sides, below is needed for valid flags access)
				if (posFwd.x < 0 || posFwd.y < 0 || posFwd.z < 0 ||
					posBwd.x < 0 || posBwd.y < 0 || posBwd.z < 0 ||
					posFwd.x > gridUpper.x || posFwd.y > gridUpper.y || ((posFwd.z > gridUpper.z)) ||
					posBwd.x > gridUpper.x || posBwd.y > gridUpper.y || ((posBwd.z > gridUpper.z)) ||
					est_obstacle(flags, posFwd.x, posFwd.y, posFwd.z) || est_obstacle(flags, posBwd.x, posBwd.y, posBwd.z) )
				{
					dval = fwd.valeur(i,j,k);
				}
			}
			// clampMode 2 handles flags in doClampComponent call

			dst.valeur(i,j,k) = dval;
		}
	});
}

template <typename T>
auto advecte_semi_lagrange(
		Grille<int> &flags,
		GrilleMAC &vel,
		Grille<T> &orig,
		float dt,
		int order)
{
	auto fwd = Grille<T>(orig.desc());

	advection_semi_lagrange(vel, fwd, orig, dt);

	if (order == 1) {
		orig.echange(fwd);
	}
	/* MacCormack */
	else if (order == 2) {
		auto bwd = Grille<T>(orig.desc());
		auto newGrid = Grille<T>(orig.desc());

		// bwd <- backwards step
		advection_semi_lagrange(vel, bwd, fwd, -dt/*, levelset, orderSpace*/);

		// newGrid <- compute correction
		MacCormackCorrect(flags, newGrid, orig, fwd, bwd, 1.0f/*, levelset*/);

		// clamp values
		restriction_maccormarck(flags, vel, newGrid, orig, fwd, dt, 1);

		orig.echange(newGrid);
	}
}

void ajoute_flottance(
		Grille<float> &density,
		GrilleMAC &vel,
		Grille<int> &flags,
		dls::math::vec3f const &gravity,
		float dt,
		float coefficient);

void ajourne_conditions_bordures_murs(
		Grille<int> &flags,
		GrilleMAC &vel);

}  /* namespace psn */
