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

#include "corps/volume.hh"

#include "contexte_evaluation.hh"
#include "operatrice_corps.h"
#include "usine_operatrice.h"

#include "bibloc/logeuse_memoire.hh"

#undef AVEC_POSEIDON

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

enum {
	TypeNone     = 0,
	TypeFluid    = 1,
	TypeObstacle = 2,
	TypeVide     = 4,
	TypeInflow   = 8,
	TypeOutflow  = 16,
	TypeOpen     = 32,
	TypeStick    = 64,
	// internal use only, for fast marching
	TypeReserved = 256,
	// 2^10 - 2^14 reserved for moving obstacles
};

class IteratricePosition {
	limites3i m_lim;
	dls::math::vec3i m_etat;

public:
	IteratricePosition(limites3i const &lim)
		: m_lim(lim)
		, m_etat(lim.min)
	{}

	dls::math::vec3i suivante()
	{
		auto etat = m_etat;

		m_etat.x += 1;

		if (m_etat.x >= m_lim.max.x) {
			m_etat.x = m_lim.min.x;

			m_etat.y += 1;

			if (m_etat.y >= m_lim.max.y) {
				m_etat.y = m_lim.min.y;

				m_etat.z += 1;
			}
		}

		return etat;
	}

	bool fini() const
	{
		return m_etat.z >= m_lim.max.z;
	}
};

static auto init_domain(Grille<int> *flags)
{
	auto res = flags->resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;

	/* set const */
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

static auto fill_grid(Grille<int> *flags, int type)
{
	auto res = flags->resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos = iter.suivante();
		auto idx = static_cast<size_t>(pos.x + (pos.y + pos.z * res.y) * res.x);
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

		auto idx = static_cast<size_t>(pos.x + (pos.y + pos.z * res.y) * res.x);
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
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
	auto iter = IteratricePosition(limites);

	auto echant = Echantilloneuse(orig);

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
}

template <typename T>
static auto advectSemiLagrange(
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

static auto addBuoyancy(
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
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(1);
	limites.max = res;
	auto iter = IteratricePosition(limites);

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
	}
}

static auto setWallBcs(
		Grille<int> *flags,
		GrilleMAC *vel)
{
	auto res = flags->resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
	auto iter = IteratricePosition(limites);

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
inline static auto ghostFluidHelper(size_t idx, int offset, const Grille<float> &phi, float gfClamp)
{
	auto alpha = thetaHelper(phi.valeur(idx), phi.valeur(idx + static_cast<size_t>(offset)));

	if (alpha < gfClamp) {
		return alpha = gfClamp;
	}

	return (1.0f - (1.0f / alpha));
}

inline static auto surfTensHelper(const size_t idx, const int offset, const Grille<float> &phi, const Grille<float> &curv, const float surfTens, const float gfClamp)
{
	return surfTens*(curv.valeur(idx + static_cast<size_t>(offset)) - ghostFluidHelper(idx, offset, phi, gfClamp) * curv.valeur(idx));
}

//! Compute rhs for pressure solve
static auto computePressureRhs(
		Grille<float>& rhs,
		const GrilleMAC& vel,
		const Grille<float>& pressure,
		const Grille<int>& flags,
		float cgAccuracy = 1e-3f,
		const Grille<float>* phi = nullptr,
		const Grille<float>* perCellCorr = nullptr,
		const GrilleMAC* fractions = nullptr,
		float gfClamp = 1e-04f,
		float cgMaxIterFac = 1.5f,
		bool precondition = true, // Deprecated, use preconditioner instead
		int preconditioner = PcMIC,
		bool enforceCompatibility = false,
		bool useL2Norm = false,
		bool zeroPressureFixing = false,
		const Grille<float> *curv = nullptr,
		const float surfTens = 0.0f )
{
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
			auto const idx = i + (j + k * static_cast<size_t>(res.y)) * static_cast<size_t>(res.x);
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

	auto nombre_voxels = static_cast<size_t>(res.x * res.y * res.z);

	auto compte = 0;

	for (auto i = 0ul; i < nombre_voxels; ++i) {
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
	auto fix_p_idx = static_cast<size_t>(fixPidx);

	auto res = rhs.resolution();
	auto stride_x = 1ul;
	auto stride_y = static_cast<size_t>(res.x);
	auto stride_z = static_cast<size_t>(res.x * res.y);

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
	auto res = flags.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;
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
		GrilleMAC& vel,
		Grille<float>& pressure,
		const Grille<int>& flags,
		float cgAccuracy = 1e-3f,
		const Grille<float>* phi = nullptr,
		const Grille<float>* perCellCorr = nullptr,
		const GrilleMAC* fractions = nullptr,
		float gfClamp = 1e-04f,
		float cgMaxIterFac = 1.5f,
		bool precondition = true, // Deprecated, use preconditioner instead
		int preconditioner = PcMIC,
		bool enforceCompatibility = false,
		bool useL2Norm = false,
		bool zeroPressureFixing = false,
		const Grille<float> *curv = nullptr,
		const float surfTens = 0.0f)
{
	if (precondition == false) {
		preconditioner = PcNone; // for backwards compatibility
	}

	// reserve temp grids
	auto etendu = limites3f{};
	etendu.min = dls::math::vec3f(-1.0f);
	etendu.max = dls::math::vec3f(1.0f);
	auto fenetre_donnees = etendu;
	auto taille_voxel = 1.0f / 64.0f;
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
	GridCgInterface *gcg = new GridCg<ApplyMatrix>(pressure, rhs, residual, search, flags, tmp, &A0, &Ai, &Aj, &Ak);

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

		pmg = gMapMG[parent];
		if (!pmg) {
			pmg = new GridMg(pressure.resolution());
			gMapMG[parent] = pmg;
		}

		gcg->setMGPreconditioner( GridCgInterface::PC_MGP, pmg);
	}

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
		releaseMG(parent);
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
		bool precondition = true, // Deprecated, use preconditioner instead
		int preconditioner = PcMIC,
		bool enforceCompatibility = false,
		bool useL2Norm = false,
		bool zeroPressureFixing = false,
		const Grille<float> *curv = nullptr,
		const float surfTens = 0.0f,
		Grille<float>* retRhs = nullptr)
{
	auto etendu = limites3f{};
	etendu.min = dls::math::vec3f(-1.0f);
	etendu.max = dls::math::vec3f(1.0f);
	auto fenetre_donnees = etendu;
	auto taille_voxel = 1.0f / 64.0f;
	auto rhs = Grille<float>(etendu, fenetre_donnees, taille_voxel);

	computePressureRhs(rhs, vel, pressure, flags, cgAccuracy,
		phi, perCellCorr, fractions, gfClamp,
		cgMaxIterFac, precondition, preconditioner, enforceCompatibility,
		useL2Norm, zeroPressureFixing, curv, surfTens);

	solvePressureSystem(rhs, vel, pressure, flags, cgAccuracy,
		phi, perCellCorr, fractions, gfClamp,
		cgMaxIterFac, precondition, preconditioner, enforceCompatibility,
		useL2Norm, zeroPressureFixing, curv, surfTens);

	correctVelocity(vel, pressure, flags, cgAccuracy,
		phi, perCellCorr, fractions, gfClamp,
		cgMaxIterFac, precondition, preconditioner, enforceCompatibility,
		useL2Norm, zeroPressureFixing, curv, surfTens);

	// optionally , return RHS
//	if(retRhs) {
//		retRhs->copyFrom( rhs );
//	}
}

/* ************************************************************************** */

class OpSimulationGaz : public OperatriceCorps {
public:
	static constexpr auto NOM = "Simulation Gaz";
	static constexpr auto AIDE = "";

	explicit OpSimulationGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
	}

	const char *chemin_entreface() const override
	{
		return "";
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		/* paramètres solver */
		auto res = 64;
		auto gs = dls::math::vec3i(res, res * 3 / 2, res);
		auto s = cree_solveur(gs);

		/* prépare les grilles */
		auto flags    = cree_grille(s, TYPE_GRILLE_FLAG);
		auto vel      = cree_grille(s, TYPE_GRILLE_MAC);
		auto density  = cree_grille(s, TYPE_GRILLE_REEL);
		auto pressure = cree_grille(s, TYPE_GRILLE_REEL);

		/* noise field */

		/* À FAIRE : réinitialisation. */
		if (contexte.temps_courant == 1) {
			init_domain(dynamic_cast<Grille<int> *>(flags));
			fill_grid(dynamic_cast<Grille<int> *>(flags), TypeFluid);
		}

		auto corps_entree = entree(0)->requiers_corps(contexte, nullptr);

		if (contexte.temps_courant < 100) {
			densityInflow(dynamic_cast<Grille<int> *>(flags), dynamic_cast<Grille<float> *>(density), nullptr, corps_entree, 1.0f, 0.5f);
		}

		advectSemiLagrange(
					dynamic_cast<Grille<int> *>(flags),
					dynamic_cast<GrilleMAC *>(vel),
					dynamic_cast<Grille<float> *>(density),
					1);

		advectSemiLagrange(
					dynamic_cast<Grille<int> *>(flags),
					dynamic_cast<GrilleMAC *>(vel),
					dynamic_cast<GrilleMAC *>(vel),
					1);

		/* À FAIRE : plus de paramètres, voir MF. */
		setWallBcs(dynamic_cast<Grille<int> *>(flags), dynamic_cast<GrilleMAC *>(vel));

		addBuoyancy(
					dynamic_cast<Grille<float> *>(density),
					dynamic_cast<GrilleMAC *>(vel),
					dls::math::vec3f(0,-6e-4,0),
					dynamic_cast<Grille<int> *>(flags));

		solvePressure(
					*dynamic_cast<GrilleMAC *>(vel),
					*dynamic_cast<Grille<float> *>(pressure),
					*dynamic_cast<Grille<int> *>(flags));

		return EXECUTION_REUSSIE;
	}
};

#endif

/* ************************************************************************** */

void enregistre_operatrices_poseidon(UsineOperatrice &usine)
{
#ifdef AVEC_POSEIDON
	usine.enregistre_type(cree_desc<OpSimulationGaz>());
#endif
}
