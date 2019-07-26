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

#include "operatrices_poseidon.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/boucle.hh"

#include "corps/volume.hh"

#include "evaluation/reseau.hh"

#include "base_de_donnees.hh"
#include "contexte_evaluation.hh"
#include "donnees_aval.hh"
#include "objet.h"
#include "operatrice_corps.h"
#include "usine_operatrice.h"

#include "fluide.hh"
#include "gradient_conjugue.hh"
#include "iter_volume.hh"

#include "limites_corps.hh"

#define AVEC_POSEIDON

#ifdef AVEC_POSEIDON

/* ************************************************************************** */

struct Solveur {
	dls::math::vec3i res{};
};

enum {
	TYPE_GRILLE_FLAG,
	TYPE_GRILLE_MAC,
	TYPE_GRILLE_REEL,
};

static auto cree_solveur(dls::math::vec3i const &res)
{
	auto s = Solveur{};
	s.res = res;
	return s;
}

static BaseGrille *cree_grille(Solveur const &s, int type)
{
	auto etendu = limites3f{};
	etendu.min = dls::math::vec3f(-1.0f);
	etendu.max = dls::math::vec3f(1.0f);
	auto fenetre_donnees = etendu;
	auto taille_voxel = 1.0f / 64.0f;

	switch (type) {
		case TYPE_GRILLE_FLAG:
		{
			return memoire::loge<Grille<int>>("grilles", etendu, fenetre_donnees, taille_voxel);
		}
		case TYPE_GRILLE_MAC:
		{
			return memoire::loge<GrilleMAC>("grilles", etendu, fenetre_donnees, taille_voxel);
		}
		case TYPE_GRILLE_REEL:
		{
			return memoire::loge<Grille<float>>("grilles", etendu, fenetre_donnees, taille_voxel);
		}
	}

	return nullptr;
}

static auto init_domain(Grille<int> *flags)
{
	auto res = flags->resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;

	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos = iter.suivante();
		flags->valeur(pos, TypeVide);
	}

	int types[6] = {
		TypeObstacle,
		TypeObstacle,
		TypeObstacle,
		TypeObstacle,
		TypeObstacle,
		TypeObstacle,
	};

	/* init boundaries */
	iter = IteratricePosition(limites);

	auto const boundaryWidth = 0;
	auto const w = boundaryWidth;

	while (!iter.fini()) {
		auto pos = iter.suivante();

		auto i = pos.x;
		auto j = pos.y;
		auto k = pos.z;

		if (i <= w) {
			flags->valeur(pos, types[0]);
		}

		if (i >= res.x - 1 - w) {
			flags->valeur(pos, types[1]);
		}

		if (j <= w) {
			flags->valeur(pos, types[2]);
		}

		if (j >= res.y - 1 - w) {
			flags->valeur(pos, types[3]);
		}

		if (k <= w) {
			flags->valeur(pos, types[4]);
		}

		if (k >= res.z - 1 - w) {
			flags->valeur(pos, types[5]);
		}
	}
}

template <typename T>
static auto fill_grid(Grille<T> *flags, T valeur)
{
	std::cerr << __func__ << '\n';

	auto res = flags->resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos = iter.suivante();
		auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);

		flags->valeur(idx) = valeur;
	}
}

static auto fill_grid(Grille<int> *flags, int type)
{
	auto res = flags->resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos = iter.suivante();
		auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);
		auto val = flags->valeur(idx);

		if ((val & TypeObstacle) == 0 && (val & TypeInflow) == 0 && (val & TypeOutflow) == 0 && (val & TypeOpen) == 0) {
			val = (val & ~(TypeVide | TypeFluid)) | type;
			flags->valeur(idx, val);
		}
	}
}

static auto densityInflow(
		Grille<int> *flags,
		Grille<float> *density,
		Grille<float> *noise,
		Corps const *corps_entree,
		float scale,
		float sigma)
{
	if (corps_entree == nullptr) {
		return;
	}

	/* À FAIRE : converti mesh en sdf */

	auto res = flags->resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
	auto iter = IteratricePosition(limites);

	auto facteur = dls::math::restreint(1.0f - 0.5f / sigma + sigma, 0.0f, 1.0f);

	/* evalue noise */
	auto target = 1.0f * scale * facteur;

	while (!iter.fini()) {
		auto pos = iter.suivante();

		auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);
		auto val = flags->valeur(idx);

		if (val != TypeFluid) {
			continue;
		}

		auto dens = density->valeur(idx);

		if (dens < target) {
			density->valeur(idx, dens);
		}
	}
}

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
static auto advecte_semi_lagrange(
		Grille<int> *flags,
		GrilleMAC *vel,
		Grille<T> *orig,
		int order)
{
	auto dt = 0.1f;

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

static auto ajoute_flottance(
		Grille<float> *density,
		GrilleMAC *vel,
		dls::math::vec3f const &gravity,
		Grille<int> *flags,
		float coefficient = 1.0f)
{
	auto dt = 0.1f; // flags.getParent()->getDt();
	auto dx = 1.0f / 64.0f; // flags.getParent()->getDx()
	auto f = -gravity * dt / dx * coefficient;

	auto res = flags->resolution();

	//auto min_velocite = dls::math::vec3f( 1000.0f);
	//auto max_velocite = dls::math::vec3f(-1000.0f);

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

			if (flags->valeur(i, j, k) != TypeFluid) {
				continue;
			}

			auto &v = vel->valeur(i, j, k);

			if (flags->valeur(i - 1, j, k) == TypeFluid) {
				v.x += (0.5f * f.x) * (density->valeur(i, j, k) + density->valeur(i - 1, j, k));
			}

			if (flags->valeur(i, j - 1, k) == TypeFluid) {
				v.y += (0.5f * f.y) * (density->valeur(i, j, k) + density->valeur(i, j - 1, k));
			}

			if (flags->valeur(i, j, k - 1) == TypeFluid) {
				v.z += (0.5f * f.z) * (density->valeur(i, j, k) + density->valeur(i, j, k - 1));
			}

			//auto velocite = vel->valeur(i, j, k);

			//dls::math::extrait_min_max(velocite, min_velocite, max_velocite);
		}
	});

	//std::cerr << __func__ << " : vélocité max = " << max_velocite << '\n';
}

static auto ajourne_conditions_bordures_murs(
		Grille<int> *flags,
		GrilleMAC *vel)
{
	auto res = flags->resolution();

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

			auto curFluid = flags->valeur(i, j, k) == TypeFluid;
			auto curObs   = flags->valeur(i, j, k) == TypeObstacle;
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
			if (i > 0 && flags->valeur(i-1,j,k) == TypeObstacle) {
				vel->valeur(i,j,k).x = bcsVel.x;
			}

			if (i > 0 && curObs && flags->valeur(i-1,j,k) == TypeFluid) {
				vel->valeur(i,j,k).x = bcsVel.x;
			}

			if (j > 0 && flags->valeur(i,j-1,k) == TypeObstacle) {
				vel->valeur(i,j,k).y = bcsVel.y;
			}

			if (j > 0 && curObs && flags->valeur(i,j-1,k) == TypeFluid) {
				vel->valeur(i,j,k).y = bcsVel.y;
			}

			if (k > 0 && flags->valeur(i,j,k-1) == TypeObstacle) {
				vel->valeur(i,j,k).z = bcsVel.z;
			}

			if (k > 0 && curObs && flags->valeur(i,j,k-1) == TypeFluid) {
				vel->valeur(i,j,k).z = bcsVel.z;
			}

			if (curFluid) {
				if ((i > 0 && flags->valeur(i - 1, j, k) == TypeStick) || (i < static_cast<size_t>(res.x - 1) && flags->valeur(i+1,j,k) == TypeStick)) {
					vel->valeur(i,j,k).y = vel->valeur(i,j,k).z = 0.0f;
				}

				if ((j > 0 && flags->valeur(i, j - 1, k) == TypeStick) || (j < static_cast<size_t>(res.y - 1) && flags->valeur(i,j+1,k) == TypeStick)) {
					vel->valeur(i,j,k).x = vel->valeur(i,j,k).z = 0.0f;
				}

				if ((k > 0 && flags->valeur(i, j, k - 1) == TypeStick) || (k < static_cast<size_t>(res.z - 1) && flags->valeur(i,j,k+1) == TypeStick)) {
					vel->valeur(i,j,k).x = vel->valeur(i,j,k).y = 0.0f;
				}
			}
		}
	});
}

/* ************************************************************************** */

enum Preconditioner { PcNone = 0, PcMIC = 1, PcMGDynamic = 2, PcMGStatic = 3 };

inline static auto thetaHelper(float inside, float outside)
{
	auto denom = inside-outside;

	if (denom > -1e-04f) {
		return 0.5f; // should always be neg, and large enough...
	}

	return std::max(0.0f, std::min(1.0f, inside/denom));
}

// calculate ghost fluid factor, cell at idx should be a fluid cell
inline static auto ghostFluidHelper(long idx, int offset, const Grille<float> &phi, float gfClamp)
{
	auto alpha = thetaHelper(phi.valeur(idx), phi.valeur(idx + offset));

	if (alpha < gfClamp) {
		return alpha = gfClamp;
	}

	return (1.0f - (1.0f / alpha));
}

inline static auto surfTensHelper(const long idx, const int offset, const Grille<float> &phi, const Grille<float> &curv, const float surfTens, const float gfClamp)
{
	return surfTens*(curv.valeur(idx + offset) - ghostFluidHelper(idx, offset, phi, gfClamp) * curv.valeur(idx));
}

//! Compute rhs for pressure solve
static auto computePressureRhs(
		Grille<float>& rhs,
		const GrilleMAC& vel,
		const Grille<int>& flags,
		const Grille<float>* phi = nullptr,
		const Grille<float>* perCellCorr = nullptr,
		const GrilleMAC* fractions = nullptr,
		float gfClamp = 1e-04f,
		bool enforceCompatibility = false,
		const Grille<float> *curv = nullptr,
		const float surfTens = 0.0f )
{
	std::cerr << __func__ << '\n';

	// compute divergence and init right hand side
	auto res = flags.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
	auto iter = IteratricePosition(limites);

	auto sum = 0.0f;
	auto cnt = 0;

	auto stride_x = 1;
	auto stride_y = res.x;
	auto stride_z = res.x * res.y;

	while (!iter.fini()) {
		auto pos_iter = iter.suivante();
		auto i = static_cast<size_t>(pos_iter.x);
		auto j = static_cast<size_t>(pos_iter.y);
		auto k = static_cast<size_t>(pos_iter.z);

		if (flags.valeur(i,j,k) != TypeFluid) {
			rhs.valeur(i, j, k) = 0.0f;
			continue;
		}

		// compute negative divergence
		// no flag checks: assumes vel at obstacle interfaces is set to zero
		auto set = 0.0f;

		if (!fractions) {
			set = vel.valeur(i,j,k).x - vel.valeur(i+1,j,k).x +
					vel.valeur(i,j,k).y - vel.valeur(i,j+1,k).y;
			set += vel.valeur(i,j,k).z - vel.valeur(i,j,k+1).z;
		}
		else {
			set = fractions->valeur(i,j,k).x * vel.valeur(i,j,k).x - fractions->valeur(i+1,j,k).x * vel.valeur(i+1,j,k).x +
					fractions->valeur(i,j,k).y * vel.valeur(i,j,k).y - fractions->valeur(i,j+1,k).y * vel.valeur(i,j+1,k).y;
			set+=fractions->valeur(i,j,k).z * vel.valeur(i,j,k).z - fractions->valeur(i,j,k+1).z * vel.valeur(i,j,k+1).z;
		}

		// compute surface tension effect (optional)
		if (phi && curv) {
			auto const idx = static_cast<long>(i + (j + k * static_cast<unsigned>(res.y)) * static_cast<unsigned>(res.x));
			auto const X = stride_x, Y = stride_y, Z = stride_z;

			if(flags.valeur(i-1,j,k) == TypeVide) {
				set += surfTensHelper(idx, -X, *phi, *curv, surfTens, gfClamp);
			}

			if(flags.valeur(i+1,j,k) == TypeVide) {
				set += surfTensHelper(idx, +X, *phi, *curv, surfTens, gfClamp);
			}

			if(flags.valeur(i,j-1,k) == TypeVide) {
				set += surfTensHelper(idx, -Y, *phi, *curv, surfTens, gfClamp);
			}

			if(flags.valeur(i,j+1,k) == TypeVide) {
				set += surfTensHelper(idx, +Y, *phi, *curv, surfTens, gfClamp);
			}

			if(flags.valeur(i,j,k-1) == TypeVide) {
				set += surfTensHelper(idx, -Z, *phi, *curv, surfTens, gfClamp);
			}

			if(flags.valeur(i,j,k+1) == TypeVide) {
				set += surfTensHelper(idx, +Z, *phi, *curv, surfTens, gfClamp);
			}
		}

		// per cell divergence correction (optional)
		if(perCellCorr) {
			set += perCellCorr->valeur(i, j, k);
		}

		// obtain sum, cell count
		sum += set;
		cnt++;

		rhs.valeur(i, j, k) = set;
	}

	if (enforceCompatibility) {
		//rhs += (float)(-sum / (float)cnt);
	}
}

static auto compte_cellules_vides(Grille<int> const &drapeaux)
{
	auto res = drapeaux.resolution();

	auto nombre_voxels = static_cast<long>(res.x * res.y * res.z);

	auto compte = 0;

	for (auto i = 0l; i < nombre_voxels; ++i) {
		if (drapeaux.valeur(i) == TypeVide) {
			compte++;
		}
	}

	return compte;
}

static auto fixPressure(
		int fixPidx,
		float value,
		Grille<float>& rhs,
		Grille<float>& A0,
		Grille<float>& Ai,
		Grille<float>& Aj,
		Grille<float>& Ak)
{
	auto fix_p_idx = static_cast<long>(fixPidx);

	auto res = rhs.resolution();
	auto stride_x = 1l;
	auto stride_y = static_cast<long>(res.x);
	auto stride_z = static_cast<long>(res.x * res.y);

	// Bring to rhs at neighbors
	rhs.valeur(fix_p_idx + stride_x) -= Ai.valeur(fix_p_idx) * value;
	rhs.valeur(fix_p_idx + stride_y) -= Aj.valeur(fix_p_idx) * value;
	rhs.valeur(fix_p_idx - stride_x) -= Ai.valeur(fix_p_idx - stride_x) * value;
	rhs.valeur(fix_p_idx - stride_y) -= Aj.valeur(fix_p_idx - stride_y) * value;
	rhs.valeur(fix_p_idx + stride_z) -= Ak.valeur(fix_p_idx) * value;
	rhs.valeur(fix_p_idx - stride_z) -= Ak.valeur(fix_p_idx - stride_z) * value;

	// Trivialize equation at 'fixPidx' to: pressure.valeur(fixPidx) = value
	rhs.valeur(fix_p_idx) = value;
	A0.valeur(fix_p_idx) = 1.0f;
	Ai.valeur(fix_p_idx) = Aj.valeur(fix_p_idx) = Ak.valeur(fix_p_idx) = 0.0f;
	Ai.valeur(fix_p_idx - stride_x) = 0.0f;
	Aj.valeur(fix_p_idx - stride_y) = 0.0f;
	Ak.valeur(fix_p_idx - stride_z) = 0.0f;
}

static auto MakeLaplaceMatrix(
		Grille<int> const &flags,
		Grille<float>& A0,
		Grille<float>& Ai,
		Grille<float>& Aj,
		Grille<float>& Ak,
		const GrilleMAC* fractions = nullptr)
{
	std::cerr << __func__ << '\n';

	auto res = flags.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(1);
	limites.max = res - dls::math::vec3i(1);
	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos_iter = iter.suivante();
		auto i = static_cast<size_t>(pos_iter.x);
		auto j = static_cast<size_t>(pos_iter.y);
		auto k = static_cast<size_t>(pos_iter.z);

		if (flags.valeur(i,j,k) != TypeFluid) {
			continue;
		}

		if(!fractions) {
			// diagonal, A0
			if (flags.valeur(i-1,j,k) != TypeObstacle) {
				A0.valeur(i,j,k) += 1.0f;
			}

			if (flags.valeur(i+1,j,k) != TypeObstacle) {
				A0.valeur(i,j,k) += 1.0f;
			}

			if (flags.valeur(i,j-1,k) != TypeObstacle) {
				A0.valeur(i,j,k) += 1.0f;
			}

			if (flags.valeur(i,j+1,k) != TypeObstacle) {
				A0.valeur(i,j,k) += 1.0f;
			}

			if (flags.valeur(i,j,k-1) != TypeObstacle) {
				A0.valeur(i,j,k) += 1.0f;
			}

			if (flags.valeur(i,j,k+1) != TypeObstacle) {
				A0.valeur(i,j,k) += 1.0f;
			}

			// off-diagonal entries
			if (flags.valeur(i+1,j,k) == TypeFluid) {
				Ai.valeur(i,j,k) = -1.0f;
			}

			if (flags.valeur(i,j+1,k) == TypeFluid) {
				Aj.valeur(i,j,k) = -1.0f;
			}

			if (flags.valeur(i,j,k+1) == TypeFluid) {
				Ak.valeur(i,j,k) = -1.0f;
			}
		}
		else {
			// diagonal
			A0.valeur(i,j,k) += fractions->valeur(i  ,j,k).x;
			A0.valeur(i,j,k) += fractions->valeur(i+1,j,k).x;
			A0.valeur(i,j,k) += fractions->valeur(i,j  ,k).y;
			A0.valeur(i,j,k) += fractions->valeur(i,j+1,k).y;
			A0.valeur(i,j,k) += fractions->valeur(i,j,k  ).z;
			A0.valeur(i,j,k) += fractions->valeur(i,j,k+1).z;

			// off-diagonal entries
			if (flags.valeur(i+1,j,k) == TypeFluid) {
				Ai.valeur(i,j,k) = -fractions->valeur(i+1,j,k).x;
			}

			if (flags.valeur(i,j+1,k) == TypeFluid) {
				Aj.valeur(i,j,k) = -fractions->valeur(i,j+1,k).y;
			}

			if (flags.valeur(i,j,k+1) == TypeFluid) {
				Ak.valeur(i,j,k) = -fractions->valeur(i,j,k+1).z;
			}
		}
	}

}

static auto solvePressureSystem(
		Grille<float>& rhs,
		Grille<float>& pressure,
		const Grille<int>& flags,
		float cgAccuracy = 1e-3f,
		const Grille<float>* phi = nullptr,
		const GrilleMAC* fractions = nullptr,
		float gfClamp = 1e-04f,
		float cgMaxIterFac = 1.5f,
		int preconditioner = PcMIC,
		bool useL2Norm = false,
		bool zeroPressureFixing = false)
{
	std::cerr << __func__ << '\n';

	// reserve temp grids
	auto etendu = pressure.etendu();
	auto fenetre_donnees = pressure.fenetre_donnees();
	auto taille_voxel = pressure.taille_voxel();
	Grille<float> residual(etendu, fenetre_donnees, taille_voxel);
	Grille<float> search(etendu, fenetre_donnees, taille_voxel);
	Grille<float> A0(etendu, fenetre_donnees, taille_voxel);
	Grille<float> Ai(etendu, fenetre_donnees, taille_voxel);
	Grille<float> Aj(etendu, fenetre_donnees, taille_voxel);
	Grille<float> Ak(etendu, fenetre_donnees, taille_voxel);
	Grille<float> tmp(etendu, fenetre_donnees, taille_voxel);

	// setup matrix and boundaries
	MakeLaplaceMatrix(flags, A0, Ai, Aj, Ak, fractions);

	if (phi) {
	//	ApplyGhostFluidDiagonal(A0, flags, *phi, gfClamp);
	}

	// check whether we need to fix some pressure value...
	// (manually enable, or automatically for high accuracy, can cause asymmetries otherwise)
	if (zeroPressureFixing || cgAccuracy < 1e-07f) {
	//	if(FLOATINGPOINT_PRECISION==1) debMsg("Warning - high CG accuracy with single-precision floating point accuracy might not converge...", 2);

		auto numEmpty = compte_cellules_vides(flags);
		auto fixPidx = -1;

		if (numEmpty == 0) {
			auto const res = flags.resolution();
			// Determine appropriate fluid cell for pressure fixing
			// 1) First check some preferred positions for approx. symmetric zeroPressureFixing
			auto topCenter = dls::math::vec3i(res.x / 2, res.y - 1, res.z / 2);

			dls::math::vec3i preferredPos[] = {
				topCenter,
				topCenter - dls::math::vec3i(0, 1, 0),
				topCenter - dls::math::vec3i(0, 2, 0)
			};

			for (auto const &pos : preferredPos) {
				if (flags.valeur(static_cast<size_t>(pos.x), static_cast<size_t>(pos.y), static_cast<size_t>(pos.z)) == TypeFluid) {
					fixPidx = pos.x + (pos.y + pos.z * res.y) * res.x;
					break;
				}
			}

			// 2) Then search whole domain
			if (fixPidx == -1) {
				auto limites = limites3i{};
				limites.min = dls::math::vec3i(0);
				limites.max = res;
				auto iter = IteratricePosition(limites);

				while (!iter.fini()) {
					auto pos_iter = iter.suivante();
					auto i = pos_iter.x;
					auto j = pos_iter.y;
					auto k = pos_iter.z;

					if (flags.valeur(static_cast<size_t>(i), static_cast<size_t>(j), static_cast<size_t>(k)) == TypeFluid) {
						fixPidx = i + (j + k * res.y) * res.x;
						break;
					}
				}
			}
			//debMsg("No empty cells! Fixing pressure of cell "<<fixPidx<<" to zero",1);
		}

		if (fixPidx >= 0) {
			fixPressure(fixPidx, 0.0f, rhs, A0, Ai, Aj, Ak);
//			static bool msgOnce = false;

//			if(!msgOnce) {
//				debMsg("Pinning pressure of cell "<<fixPidx<<" to zero", 2);
//				msgOnce=true;
//			}
		}
	}

	// CG setup
	// note: the last factor increases the max iterations for 2d, which right now can't use a preconditioner
	auto gcg = new GridCg(pressure, rhs, residual, search, flags, tmp, &A0, &Ai, &Aj, &Ak);
	gcg->setAccuracy( cgAccuracy );
	gcg->setUseL2Norm( useL2Norm );

	int maxIter = 0;

	Grille<float> *pca0 = nullptr, *pca1 = nullptr, *pca2 = nullptr, *pca3 = nullptr;
	GridMg* pmg = nullptr;

	// optional preconditioning
	if (preconditioner == PcNone || preconditioner == PcMIC) {
		maxIter = static_cast<int>(cgMaxIterFac * static_cast<float>(max(flags.resolution())) * 4.0f);

		pca0 = new Grille<float>(etendu, fenetre_donnees, taille_voxel);
		pca1 = new Grille<float>(etendu, fenetre_donnees, taille_voxel);
		pca2 = new Grille<float>(etendu, fenetre_donnees, taille_voxel);
		pca3 = new Grille<float>(etendu, fenetre_donnees, taille_voxel);

		gcg->setICPreconditioner( preconditioner == PcMIC ? GridCgInterface::PC_mICP : GridCgInterface::PC_None,
			pca0, pca1, pca2, pca3);
	}
	else if (preconditioner == PcMGDynamic || preconditioner == PcMGStatic) {
		maxIter = 100;

//		pmg = gMapMG[parent];
//		if (!pmg) {
//			pmg = new GridMg(pressure.resolution());
//			gMapMG[parent] = pmg;
//		}

		gcg->setMGPreconditioner( GridCgInterface::PC_MGP, pmg);
	}

	std::cerr << "Lance CG solver...." << '\n';
	// CG solve
	for (int iter = 0; iter < maxIter; iter++) {
		if (!gcg->iterate()) {
			iter = maxIter;
		}

//		if (iter < maxIter) {
//			debMsg("FluidSolver::solvePressure iteration "<<iter<<", residual: "<<gcg->getResNorm(), 9);
//		}
	}
	//debMsg("FluidSolver::solvePressure done. Iterations:"<<gcg->getIterations()<<", residual:"<<gcg->getResNorm(), 2);

	// Cleanup
	delete gcg;
	delete pca0;
	delete pca1;
	delete pca2;
	delete pca3;

	// PcMGDynamic: always delete multigrid solver after use
	// PcMGStatic: keep multigrid solver for next solve
	if (pmg && preconditioner == PcMGDynamic) {
//		releaseMG(parent);
	}
}

static auto knCorrectVelocity(const Grille<int>& flags, GrilleMAC& vel, const Grille<float>& pressure)
{
	auto res = flags.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(1);
	limites.max = res;
	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos_iter = iter.suivante();
		auto i = static_cast<size_t>(pos_iter.x);
		auto j = static_cast<size_t>(pos_iter.y);
		auto k = static_cast<size_t>(pos_iter.z);

		auto idx = flags.calcul_index(i, j, k);

		if (est_fluide(flags, idx)) {
			if (est_fluide(flags, i-1,j,k)) {
				vel.valeur(idx).x -= (pressure.valeur(idx) - pressure.valeur(i-1,j,k));
			}

			if (est_fluide(flags, i,j-1,k)) {
				vel.valeur(idx).y -= (pressure.valeur(idx) - pressure.valeur(i,j-1,k));
			}

			if (est_fluide(flags, i,j,k-1)) {
				vel.valeur(idx).z -= (pressure.valeur(idx) - pressure.valeur(i,j,k-1));
			}

			if (est_vide(flags, i-1,j,k)) {
				vel.valeur(idx).x -= pressure.valeur(idx);
			}

			if (est_vide(flags, i,j-1,k)) {
				vel.valeur(idx).y -= pressure.valeur(idx);
			}

			if (est_vide(flags, i,j,k-1)) {
				vel.valeur(idx).z -= pressure.valeur(idx);
			}
		}
		/* Ne changeons pas les vélocités dans les cellules d'outflow. */
		else if (est_vide(flags, idx) && !est_outflow(flags, idx)) {
			if (est_fluide(flags, i-1,j,k)) {
				vel.valeur(idx).x += pressure.valeur(i-1,j,k);
			}
			else {
				vel.valeur(idx).x  = 0.f;
			}

			if (est_fluide(flags, i,j-1,k)) {
				vel.valeur(idx).y += pressure.valeur(i,j-1,k);
			}
			else {
				vel.valeur(idx).y  = 0.f;
			}

			if (est_fluide(flags, i,j,k-1)) {
				vel.valeur(idx).z += pressure.valeur(i,j,k-1);
			}
			else {
				vel.valeur(idx).z  = 0.f;
			}
		}
	}
}

static auto correctVelocity(
		GrilleMAC& vel,
		Grille<float>& pressure,
		const Grille<int>& flags,
		const Grille<float>* phi = nullptr,
		float gfClamp = 1e-04f,
		const Grille<float> *curv = nullptr,
		const float surfTens = 0.0f)
{
	std::cerr << __func__ << '\n';
	knCorrectVelocity(flags, vel, pressure);

	if (phi) {
//		knCorrectVelocityGhostFluid (vel, flags, pressure, *phi, gfClamp,  curv, surfTens );
//		// improve behavior of clamping for large time steps:
//		knReplaceClampedGhostFluidVels (vel, flags, pressure, *phi, gfClamp);
	}
}

static auto solvePressure(
		GrilleMAC &vel,
		Grille<float> &pressure,
		Grille<int> const &flags,
		float cgAccuracy = 1e-3f,
		const Grille<float>* phi = nullptr,
		const Grille<float>* perCellCorr = nullptr,
		const GrilleMAC* fractions = nullptr,
		float gfClamp = 1e-04f,
		float cgMaxIterFac = 1.5f,
		int preconditioner = PcNone,
		bool enforceCompatibility = false,
		bool useL2Norm = false,
		bool zeroPressureFixing = false,
		const Grille<float> *curv = nullptr,
		const float surfTens = 0.0f,
		Grille<float>* retRhs = nullptr)
{
	std::cerr << __func__ << '\n';
	auto etendu = limites3f{};
	etendu.min = dls::math::vec3f(-1.0f);
	etendu.max = dls::math::vec3f(1.0f);
	auto fenetre_donnees = etendu;
	auto taille_voxel = 1.0f / 64.0f;
	auto rhs = Grille<float>(etendu, fenetre_donnees, taille_voxel);

	computePressureRhs(rhs, vel, flags, phi, perCellCorr, fractions, gfClamp,
					   enforceCompatibility, curv, surfTens);

	solvePressureSystem(rhs, pressure, flags, cgAccuracy, phi, fractions,
						gfClamp, cgMaxIterFac, preconditioner, useL2Norm,
						zeroPressureFixing);

	correctVelocity(vel, pressure, flags, phi, gfClamp, curv, surfTens);

	// optionally , return RHS
	if (retRhs) {
		retRhs->copie_donnees(rhs);
	}
}

/* ************************************************************************** */

struct PoseidonGaz {
	Grille<int> *drapeaux = nullptr;
	Grille<float> *densite = nullptr;
	Grille<float> *pression = nullptr;
	GrilleMAC *velocite = nullptr;
};

static inline auto extrait_poseidon(DonneesAval *da)
{
	return std::any_cast<PoseidonGaz *>(da->table["poseidon_gaz"]);
}

class OpEntreeGaz : public OperatriceCorps {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Entrée Gaz";
	static constexpr auto AIDE = "";

	explicit OpEntreeGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
	}

	OpEntreeGaz(OpEntreeGaz const &) = default;
	OpEntreeGaz &operator=(OpEntreeGaz const &) = default;

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_entree_gaz.jo";
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	Objet *trouve_objet(ContexteEvaluation const &contexte)
	{
		auto nom_objet = evalue_chaine("nom_objet");

		if (nom_objet.est_vide()) {
			return nullptr;
		}

		if (nom_objet != m_nom_objet || m_objet == nullptr) {
			m_nom_objet = nom_objet;
			m_objet = contexte.bdd->objet(nom_objet);
		}

		return m_objet;
	}

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet == nullptr) {
				return;
			}
		}

		compilatrice.ajoute_dependance(noeud, m_objet);
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_objet") {
			for (auto &objet : contexte.bdd->objets()) {
				liste.pousse(objet->nom);
			}
		}
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		if (!donnees_aval || !donnees_aval->possede("poseidon_gaz")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		/* passe à notre exécution */

		m_objet = trouve_objet(contexte);

		if (m_objet == nullptr) {
			this->ajoute_avertissement("Aucun objet sélectionné");
			return EXECUTION_ECHOUEE;
		}

		if (contexte.temps_courant > 100) {
			return EXECUTION_REUSSIE;
		}

		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto grille = poseidon_gaz->densite;

		if (grille == nullptr) {
			this->ajoute_avertissement("La simulation n'est pas encore commencée");
			return EXECUTION_ECHOUEE;
		}

		/* copie par convénience */
		m_objet->corps.accede_lecture([this](Corps const &_corps_)
		{
			_corps_.copie_vers(&m_corps);
		});

		/* À FAIRE : considère la surface des maillages. */
		auto res = grille->resolution();
		auto lim = calcule_limites_mondiales_corps(m_corps);
		auto min_idx = grille->monde_vers_unit(lim.min);
		auto max_idx = grille->monde_vers_unit(lim.max);

		auto limites = limites3i{};
		limites.min.x = static_cast<int>(static_cast<float>(res.x) * min_idx.x);
		limites.min.y = static_cast<int>(static_cast<float>(res.y) * min_idx.y);
		limites.min.z = static_cast<int>(static_cast<float>(res.z) * min_idx.z);
		limites.max.x = static_cast<int>(static_cast<float>(res.x) * max_idx.x);
		limites.max.y = static_cast<int>(static_cast<float>(res.y) * max_idx.y);
		limites.max.z = static_cast<int>(static_cast<float>(res.z) * max_idx.z);
		auto iter = IteratricePosition(limites);

		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto idx = static_cast<long>(pos.x + (pos.y + pos.z * res.y) * res.x);

			grille->valeur(idx) = 1.0f;
		}

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

#include "outils_visualisation.hh"

class OpSimulationGaz : public OperatriceCorps {
	PoseidonGaz m_poseidon{};

public:
	static constexpr auto NOM = "Simulation Gaz";
	static constexpr auto AIDE = "";

	explicit OpSimulationGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
	}

	~OpSimulationGaz() override
	{
		memoire::deloge("grilles", m_poseidon.densite);
		memoire::deloge("grilles", m_poseidon.pression);
		memoire::deloge("grilles", m_poseidon.drapeaux);
		memoire::deloge("grilles", m_poseidon.velocite);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);

		m_corps.reinitialise();

		/* init domaine */
		auto res = 64;

		auto etendu = limites3f{};
		etendu.min = dls::math::vec3f(-5.0f, -1.0f, -5.0f);
		etendu.max = dls::math::vec3f( 5.0f,  9.0f,  5.0f);
		auto fenetre_donnees = etendu;
		auto taille_voxel = 10.0f / static_cast<float>(res);

		if (m_poseidon.densite == nullptr) {
			m_poseidon.densite = memoire::loge<Grille<float>>("grilles", etendu, fenetre_donnees, taille_voxel);
			m_poseidon.pression = memoire::loge<Grille<float>>("grilles", etendu, fenetre_donnees, taille_voxel);
			m_poseidon.drapeaux = memoire::loge<Grille<int>>("grilles", etendu, fenetre_donnees, taille_voxel);
			m_poseidon.velocite = memoire::loge<GrilleMAC>("grilles", etendu, fenetre_donnees, taille_voxel);
		}

		fill_grid(m_poseidon.drapeaux, TypeFluid);

		/* init simulation */

		auto da = DonneesAval{};
		da.table.insere({ "poseidon_gaz", &m_poseidon });

		entree(0)->requiers_corps(contexte, &da);

		/* lance simulation */
		entree(1)->requiers_corps(contexte, &da);

		/* sauve données */

		auto volume = memoire::loge<Volume>("Volume");
		volume->grille = m_poseidon.densite->copie();

		/* visualise domaine */
		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);
		dessine_boite(m_corps, attr_C, etendu.min, etendu.max, dls::math::vec3f(0.0f, 1.0f, 0.0f));
		dessine_boite(m_corps, attr_C, etendu.min, etendu.min + dls::math::vec3f(taille_voxel), dls::math::vec3f(0.0f, 1.0f, 0.0f));

		m_corps.ajoute_primitive(volume);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};
#endif

/* ************************************************************************** */

class OpAdvectionGaz : public OperatriceCorps {
public:
	static constexpr auto NOM = "Advection Gaz";
	static constexpr auto AIDE = "";

	explicit OpAdvectionGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		if (!donnees_aval || !donnees_aval->possede("poseidon_gaz")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto ordre = 1;
		auto densite = poseidon_gaz->densite;
		auto velocite = poseidon_gaz->velocite;
		auto drapeaux = poseidon_gaz->drapeaux;

		auto vieille_vel = memoire::loge<GrilleMAC>("grilles", velocite->etendu(), velocite->fenetre_donnees(), velocite->taille_voxel());
		vieille_vel->copie_donnees(*velocite);

		advecte_semi_lagrange(drapeaux, vieille_vel, densite, ordre);

		advecte_semi_lagrange(drapeaux, vieille_vel, velocite, ordre);

		/* À FAIRE : plus de paramètres, voir MF. */
		ajourne_conditions_bordures_murs(drapeaux, velocite);

		memoire::deloge("grilles", vieille_vel);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

class OpFlottanceGaz : public OperatriceCorps {
public:
	static constexpr auto NOM = "Flottance Gaz";
	static constexpr auto AIDE = "";

	explicit OpFlottanceGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		if (!donnees_aval || !donnees_aval->possede("poseidon_gaz")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto gravite = dls::math::vec3f(0.0f, -1.0f, 0.0f);
		auto densite = poseidon_gaz->densite;
		auto velocite = poseidon_gaz->velocite;
		auto drapeaux = poseidon_gaz->drapeaux;

		ajoute_flottance(densite, velocite, gravite, drapeaux);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

class OpIncompressibiliteGaz : public OperatriceCorps {
public:
	static constexpr auto NOM = "Incompressibilité Gaz";
	static constexpr auto AIDE = "";

	explicit OpIncompressibiliteGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		if (!donnees_aval || !donnees_aval->possede("poseidon_gaz")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		std::cerr << "------------------------------------------------\n";
		std::cerr << "Incompressibilité, image " << contexte.temps_courant << '\n';

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto pression = poseidon_gaz->pression;
		auto velocite = poseidon_gaz->velocite;
		auto drapeaux = poseidon_gaz->drapeaux;

		solvePressure(*velocite, *pression, *drapeaux);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_poseidon(UsineOperatrice &usine)
{
#ifdef AVEC_POSEIDON
	usine.enregistre_type(cree_desc<OpEntreeGaz>());
	usine.enregistre_type(cree_desc<OpSimulationGaz>());
	usine.enregistre_type(cree_desc<OpAdvectionGaz>());
	usine.enregistre_type(cree_desc<OpFlottanceGaz>());
	usine.enregistre_type(cree_desc<OpIncompressibiliteGaz>());
#endif
}
