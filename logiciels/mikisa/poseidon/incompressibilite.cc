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

#include "corps/iter_volume.hh"

#include "fluide.hh"
#include "gradient_conjugue.hh"

/* À FAIRE : déduplique et nettoye tout ça.
 * Ce fichier contient énormément de code provenant de sources variées. Ceci
 * étant car des bugs incompréhensibles sont présent et j'ai essayé plusieurs
 * implémentations de différents projets jusqu'à la découverte d'une
 * implémentation donnant des résultats correctes.
 *
 * Les bugs semblent se trouver dans la formation des préconditionneurs du code
 * original de Poséidon et de Mantaflow. La version de Construct fonctionne mais
 * est instable, tandis que celle de Blender donne le résultat attendu.
 *
 * En tout et pour tout il faudra dédupliquer les différentes fonctions tout en
 * traquant le bug.
 */

namespace psn {

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

template <typename TypeGrille, typename Op>
static void applique_parallele(
		TypeGrille &grille,
		limites3i const &limites,
		Op &&op)
{
	auto min = limites.min;
	auto max = limites.max;

	boucle_parallele(tbb::blocked_range<int>(min.z, max.z),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto lims = limites3i{};
		lims.min = dls::math::vec3i(min.x, min.y, plage.begin());
		lims.max = dls::math::vec3i(max.x, max.y, plage.end());

		auto iter = IteratricePosition(lims);

		while (!iter.fini()) {
			auto pos_iter = iter.suivante();
			auto i = static_cast<size_t>(pos_iter.x);
			auto j = static_cast<size_t>(pos_iter.y);
			auto k = static_cast<size_t>(pos_iter.z);

			op(grille, i, j, k);
		}
	});
}

namespace construct {

#define EN_SERIE

static void rend_incompressible(
		GrilleMAC &vel,
		int iterations = 100)
{
	auto p = Grille<float>(vel.etendu(), vel.fenetre_donnees(), vel.taille_voxel());
	auto divergence = Grille<float>(vel.etendu(), vel.fenetre_donnees(), vel.taille_voxel());

	auto r = Grille<float>(vel.etendu(), vel.fenetre_donnees(), vel.taille_voxel());
	auto d = Grille<float>(vel.etendu(), vel.fenetre_donnees(), vel.taille_voxel());
	auto q = Grille<float>(vel.etendu(), vel.fenetre_donnees(), vel.taille_voxel());
	auto skip = Grille<float>(vel.etendu(), vel.fenetre_donnees(), vel.taille_voxel());

	auto res = vel.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;

	// Outside "skip" area
	{
#ifdef EN_SERIE
		auto iter = IteratricePosition(limites);
		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto i = static_cast<size_t>(pos.x);
			auto j = static_cast<size_t>(pos.y);
			auto k = static_cast<size_t>(pos.z);

			if (i == 0 || i == static_cast<size_t>(res[0]-1) || j==0 || j== static_cast<size_t>(res[1]-1) || k==0 || k== static_cast<size_t>(res[2]-1))
				skip.valeur(i, j, k, 1.0f);
			else
				skip.valeur(i, j, k, 0.0f);

			// Add in extra boundaries
//			if (boundary.eval(position(i, j, k)) > 0)
//				skip.valeur(i, j, k, 1);
		}
#else
		applique_parallele(skip, limites,
						   [&](Grille<float> &grille, size_t i, size_t j, size_t k)
		{
			if (i == 0 || i == static_cast<size_t>(res[0]-1) || j==0 || j== static_cast<size_t>(res[1]-1) || k==0 || k== static_cast<size_t>(res[2]-1))
				grille.valeur(i, j, k, 1.0f);
			else
				grille.valeur(i, j, k, 0.0f);
		});
#endif
	}


	// TODO: Generalize this for other boundaries and conditions (Dirichlet, etc)
	// Set no flux for velocity
	{
#ifdef EN_SERIE
		auto iter = IteratricePosition(limites);
		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto i = static_cast<size_t>(pos.x);
			auto j = static_cast<size_t>(pos.y);
			auto k = static_cast<size_t>(pos.z);

			if (i==0 || i== static_cast<size_t>(res[0]-1)) {
				vel.valeur(i, j, k).x = 0.0f;
			}

			if (j==0 || j== static_cast<size_t>(res[1]-1)) {
				vel.valeur(i, j, k).y = 0.0f;
			}

			if (k==0 || k== static_cast<size_t>(res[2]-1)) {
				vel.valeur(i, j, k).z = 0.0f;
			}
		}
#else
		applique_parallele(vel, limites,
						   [&](GrilleMAC &grille, size_t i, size_t j, size_t k)
		{
			if (i==0 || i== static_cast<size_t>(res[0]-1)) {
				grille.valeur(i, j, k).x = 0.0f;
			}

			if (j==0 || j== static_cast<size_t>(res[1]-1)) {
				grille.valeur(i, j, k).y = 0.0f;
			}

			if (k==0 || k== static_cast<size_t>(res[2]-1)) {
				grille.valeur(i, j, k).z = 0.0f;
			}
		});
#endif
	}

	// Compute divergence of non-boundary cells
	{
		limites.min = dls::math::vec3i(1);
		limites.max = res - dls::math::vec3i(1);
#ifdef EN_SERIE
		auto iter = IteratricePosition(limites);
		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto i = static_cast<size_t>(pos.x);
			auto j = static_cast<size_t>(pos.y);
			auto k = static_cast<size_t>(pos.z);

			auto D = vel.valeur(i+1, j, k)[0] + vel.valeur(i, j+1, k)[1] + vel.valeur(i, j, k+1)[2];
			D -= vel.valeur(i-1, j, k)[0] + vel.valeur(i, j-1, k)[1] + vel.valeur(i, j, k-1)[2];
			D *= .5f;
			divergence.valeur(i, j, k, D); // ASSUMED CUBIC CELLS!
		}
#else
		applique_parallele(divergence, limites,
						   [&](Grille<float> &grille, size_t i, size_t j, size_t k)
		{
			auto D = vel.valeur(i+1, j, k)[0] + vel.valeur(i, j+1, k)[1] + vel.valeur(i, j, k+1)[2];
			D -= vel.valeur(i-1, j, k)[0] + vel.valeur(i, j-1, k)[1] + vel.valeur(i, j, k-1)[2];
			D *= .5f;
			grille.valeur(i, j, k, D); // ASSUMED CUBIC CELLS!
		});
#endif
	}

	// Conjugate Gradient
	// (http://en.wikipedia.org/wiki/Conjugate_gradient_method)
	// r = b - Ax
	{
		limites.min = dls::math::vec3i(1);
		limites.max = res - dls::math::vec3i(1);

#ifdef EN_SERIE
		auto iter = IteratricePosition(limites);
		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto i = static_cast<size_t>(pos.x);
			auto j = static_cast<size_t>(pos.y);
			auto k = static_cast<size_t>(pos.z);
			auto center = 0.0f, R = 0.0f;

			if (skip.valeur(i-1, j, k)!=1.0f) {
				center += 1; R += p.valeur(i-1, j, k);
			}
			if (skip.valeur(i, j-1, k)!=1.0f) {
				center += 1; R += p.valeur(i, j-1, k);
			}

			if (skip.valeur(i, j, k-1)!=1.0f) {
				center += 1; R += p.valeur(i, j, k-1);
			}

			if (skip.valeur(i+1, j, k)!=1.0f) {
				center += 1; R += p.valeur(i+1, j, k);
			}

			if (skip.valeur(i, j+1, k)!=1.0f) {
				center += 1; R += p.valeur(i, j+1, k);
			}

			if (skip.valeur(i, j, k+1)!=1.0f) {
				center += 1; R += p.valeur(i, j, k+1);
			}

			R = -divergence.valeur(i, j, k) - (center * p.valeur(i, j, k) - R);

			r.valeur(i, j, k, skip.valeur(i, j, k)==1.0f ? 0.0f : R);
		}
#else
		applique_parallele(r, limites,
						   [&](Grille<float> &grille, size_t i, size_t j, size_t k)
		{
			auto center = 0.0f, R = 0.0f;

			if (skip.valeur(i-1, j, k)!=1.0f) {
				center += 1; R += p.valeur(i-1, j, k);
			}
			if (skip.valeur(i, j-1, k)!=1.0f) {
				center += 1; R += p.valeur(i, j-1, k);
			}

			if (skip.valeur(i, j, k-1)!=1.0f) {
				center += 1; R += p.valeur(i, j, k-1);
			}

			if (skip.valeur(i+1, j, k)!=1.0f) {
				center += 1; R += p.valeur(i+1, j, k);
			}

			if (skip.valeur(i, j+1, k)!=1.0f) {
				center += 1; R += p.valeur(i, j+1, k);
			}

			if (skip.valeur(i, j, k+1)!=1.0f) {
				center += 1; R += p.valeur(i, j, k+1);
			}

			R = -divergence.valeur(i, j, k) - (center * p.valeur(i, j, k) - R);

			grille.valeur(i, j, k, skip.valeur(i, j, k)==1.0f ? 0.0f : R);
		});
#endif
	}

	// d = r
	{
		limites.min = dls::math::vec3i(1);
		limites.max = res - dls::math::vec3i(1);
#ifdef EN_SERIE
		auto iter = IteratricePosition(limites);
		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto i = static_cast<size_t>(pos.x);
			auto j = static_cast<size_t>(pos.y);
			auto k = static_cast<size_t>(pos.z);

			d.valeur(i, j, k, r.valeur(i, j, k));
		}
#else
		applique_parallele(d, limites,
						   [&](Grille<float> &grille, size_t i, size_t j, size_t k)
		{
			grille.valeur(i, j, k, r.valeur(i, j, k));
		});
#endif
	}

	// deltaNew = transpose(r) * r
	auto deltaNew = 0.0f;
	{
		limites.min = dls::math::vec3i(1);
		limites.max = res - dls::math::vec3i(1);

		auto iter = IteratricePosition(limites);
		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto i = static_cast<size_t>(pos.x);
			auto j = static_cast<size_t>(pos.y);
			auto k = static_cast<size_t>(pos.z);
			deltaNew += r.valeur(i, j, k) * r.valeur(i, j, k);
		}
	}

	// delta = deltaNew
	const auto eps = 1.e-4f;
	auto maxR = 1.0f;
	int iteration = 0 ;
	while((iteration<iterations) && (maxR > eps)) {
		// q = A d
		{
			limites.min = dls::math::vec3i(1);
			limites.max = res - dls::math::vec3i(1);
#ifdef EN_SERIE
			auto iter = IteratricePosition(limites);
			while (!iter.fini()) {
				auto pos = iter.suivante();
				auto i = static_cast<size_t>(pos.x);
				auto j = static_cast<size_t>(pos.y);
				auto k = static_cast<size_t>(pos.z);

				auto center = 0.0f, R = 0.0f;

				if (skip.valeur(i-1, j, k)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i-1, j, k);
				}

				if (skip.valeur(i, j-1, k)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i, j-1, k);
				}

				if (skip.valeur(i, j, k-1)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i, j, k-1);
				}

				if (skip.valeur(i+1, j, k)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i+1, j, k);
				}

				if (skip.valeur(i, j+1, k)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i, j+1, k);
				}

				if (skip.valeur(i, j, k+1)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i, j, k+1);
				}

				R = (center * d.valeur(i, j, k) - R);
				q.valeur(i, j, k, skip.valeur(i, j, k) == 1.0f ? 0.0f : R);
			}
#else
			applique_parallele(q, limites,
							   [&](Grille<float> &grille, size_t i, size_t j, size_t k)
			{
				auto center = 0.0f, R = 0.0f;

				if (skip.valeur(i-1, j, k)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i-1, j, k);
				}

				if (skip.valeur(i, j-1, k)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i, j-1, k);
				}

				if (skip.valeur(i, j, k-1)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i, j, k-1);
				}

				if (skip.valeur(i+1, j, k)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i+1, j, k);
				}

				if (skip.valeur(i, j+1, k)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i, j+1, k);
				}

				if (skip.valeur(i, j, k+1)!=1.0f) {
					center += 1.0f;
					R += d.valeur(i, j, k+1);
				}

				R = (center * d.valeur(i, j, k) - R);
				grille.valeur(i, j, k, skip.valeur(i, j, k) == 1.0f ? 0.0f : R);
			});
#endif
		}

		// alpha = deltaNew / (d'q)
		auto alpha = 0.0f;
		{
			limites.min = dls::math::vec3i(1);
			limites.max = res - dls::math::vec3i(1);

			auto iter = IteratricePosition(limites);
			while (!iter.fini()) {
				auto pos = iter.suivante();
				auto i = static_cast<size_t>(pos.x);
				auto j = static_cast<size_t>(pos.y);
				auto k = static_cast<size_t>(pos.z);

				alpha += d.valeur(i, j, k) * q.valeur(i, j, k);
			}
		}

		if (std::abs(alpha) > 0.0f) {
			alpha = deltaNew / alpha;
		}

		// x = x + alpha * d
		{
			limites.min = dls::math::vec3i(1);
			limites.max = res - dls::math::vec3i(1);

#ifdef EN_SERIE
			auto iter = IteratricePosition(limites);
			while (!iter.fini()) {
				auto pos = iter.suivante();
				auto i = static_cast<size_t>(pos.x);
				auto j = static_cast<size_t>(pos.y);
				auto k = static_cast<size_t>(pos.z);

				p.valeur(i, j, k, p.valeur(i, j, k) + alpha * d.valeur(i, j, k));
				r.valeur(i, j, k, r.valeur(i, j, k) - alpha * q.valeur(i, j, k));
			}
#else
			applique_parallele(p, limites,
							   [&](Grille<float> &grille, size_t i, size_t j, size_t k)
			{
				grille.valeur(i, j, k, grille.valeur(i, j, k) + alpha * d.valeur(i, j, k));
				r.valeur(i, j, k, r.valeur(i, j, k) - alpha * q.valeur(i, j, k));
			});
#endif
		}

		// r = r - alpha * q
		maxR = 0.;

		//#pragma omp parallel for reduction(+:maxR)
		{
			limites.min = dls::math::vec3i(1);
			limites.max = res - dls::math::vec3i(1);

			auto iter = IteratricePosition(limites);
			while (!iter.fini()) {
				auto pos = iter.suivante();
				auto i = static_cast<size_t>(pos.x);
				auto j = static_cast<size_t>(pos.y);
				auto k = static_cast<size_t>(pos.z);

				maxR = r.valeur(i, j, k) > maxR ? r.valeur(i, j, k) : maxR;
			}
		}

		auto deltaOld = deltaNew;

		// deltaNew = r'r
		deltaNew = 0.0f;
		{
			limites.min = dls::math::vec3i(1);
			limites.max = res - dls::math::vec3i(1);

			auto iter = IteratricePosition(limites);
			while (!iter.fini()) {
				auto pos = iter.suivante();
				auto i = static_cast<size_t>(pos.x);
				auto j = static_cast<size_t>(pos.y);
				auto k = static_cast<size_t>(pos.z);

				deltaNew += r.valeur(i, j, k) * r.valeur(i, j, k);
			}
		}

		auto beta = deltaNew / deltaOld;

		// d = r + beta * d
		{
			limites.min = dls::math::vec3i(1);
			limites.max = res - dls::math::vec3i(1);

#ifdef EN_SERIE
			auto iter = IteratricePosition(limites);
			while (!iter.fini()) {
				auto pos = iter.suivante();
				auto i = static_cast<size_t>(pos.x);
				auto j = static_cast<size_t>(pos.y);
				auto k = static_cast<size_t>(pos.z);

				d.valeur(i, j, k, r.valeur(i, j, k) + beta * d.valeur(i, j, k));
			}
#else
			applique_parallele(d, limites,
							   [&](Grille<float> &grille, size_t i, size_t j, size_t k)
			{
				grille.valeur(i, j, k, r.valeur(i, j, k) + beta * grille.valeur(i, j, k));
			});
#endif
		}

		// Next iteration...
		++iteration;
#if 1
		if (iteration%10==0) {
			using namespace std;
			cout << "Iteration " << iteration << " -- Error: " << maxR << endl;
		}
#endif
	}

	{
		limites.min = dls::math::vec3i(0);
		limites.max = res;

		auto iter = IteratricePosition(limites);
		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto i = static_cast<size_t>(pos.x);
			auto j = static_cast<size_t>(pos.y);
			auto k = static_cast<size_t>(pos.z);

			if (i==0) p.valeur(i, j, k, p.valeur(1, j, k));
			if (j==0) p.valeur(i, j, k, p.valeur(i, 1, k));
			if (k==0) p.valeur(i, j, k, p.valeur(i, j, 1));
			if (i==static_cast<size_t>(res[0]-1)) p.valeur(i, j, k, p.valeur(static_cast<size_t>(res[0]-2), j, k));
			if (j==static_cast<size_t>(res[1]-1)) p.valeur(i, j, k, p.valeur(i, static_cast<size_t>(res[1]-2), k));
			if (k==static_cast<size_t>(res[2]-1)) p.valeur(i, j, k, p.valeur(i, j, static_cast<size_t>(res[2]-2)));
		}
	}

	// Subtract gradient of "pressure"
	{
		limites.min = dls::math::vec3i(1);
		limites.max = res - dls::math::vec3i(1);

		auto iter = IteratricePosition(limites);
		while (!iter.fini()) {
			auto pos = iter.suivante();
			auto i = static_cast<size_t>(pos.x);
			auto j = static_cast<size_t>(pos.y);
			auto k = static_cast<size_t>(pos.z);

			auto V = vel.valeur(i, j, k);
			V[0] -= (p.valeur(i+1, j, k) - p.valeur(i-1, j, k)) * .5f;
			V[1] -= (p.valeur(i, j+1, k) - p.valeur(i, j-1, k)) * .5f;
			V[2] -= (p.valeur(i, j, k+1) - p.valeur(i, j, k-1)) * .5f;
			vel.valeur(i, j, k, V);
		}
	}
}

}  /* namespace construct */

#undef SOLVEUR_POSEIDON

#ifdef SOLVEUR_POSEIDON
namespace poseidon {

struct PCGSolver {
	Grille<float> M;
	Grille<float> Adiag;
	Grille<float> Aplusi;
	Grille<float> Aplusj;
	Grille<float> Aplusk;

	/* Pression */
	Grille<float> p;

	/* Résidus, initiallement la divergence du champs de vélocité. */
	Grille<float> r;

	/* Vecteur auxiliaire */
	Grille<float> z;

	/* Vecteur de recherche */
	Grille<float> s;

	PCGSolver(limites3f const &etendu, limites3f const &fenetre_donnees, float taille_voxel)
		: M(etendu, fenetre_donnees, taille_voxel)
		, Adiag(etendu, fenetre_donnees, taille_voxel)
		, Aplusi(etendu, fenetre_donnees, taille_voxel)
		, Aplusj(etendu, fenetre_donnees, taille_voxel)
		, Aplusk(etendu, fenetre_donnees, taille_voxel)
		, p(etendu, fenetre_donnees, taille_voxel)
		, r(etendu, fenetre_donnees, taille_voxel)
		, z(etendu, fenetre_donnees, taille_voxel)
		, s(etendu, fenetre_donnees, taille_voxel)
	{}
};

float calcul_divergence(
		Grille<float> const &grille_x,
		Grille<float> const &grille_y,
		Grille<float> const &grille_z,
		Grille<float> &d)
{
	auto const res = d.resolution();
	float max_divergence = 0.0f;

	for (long z = 0; z < res.z; ++z) {
		for (long y = 0; y < res.y; ++y) {
			for (long x = 0; x < res.x; ++x) {
				auto const x0 = grille_x.valeur(x - 1, y, z);
				auto const x1 = grille_x.valeur(x + 1, y, z);
				auto const y0 = grille_y.valeur(x, y - 1, z);
				auto const y1 = grille_y.valeur(x, y + 1, z);
				auto const z0 = grille_z.valeur(x, y, z - 1);
				auto const z1 = grille_z.valeur(x, y, z + 1);

				auto const divergence = (x1 - x0) + (y1 - y0) + (z1 - z0);

				max_divergence = std::max(max_divergence, divergence);

				d.valeur(x, y, z, divergence);
			}
		}
	}

	return max_divergence;
}

float calcul_divergence(
		Grille<dls::math::vec3f> const &grille,
		Grille<float> &d)
{
	auto const res = d.resolution();
	float max_divergence = 0.0f;

	for (long z = 0; z < res.z; ++z) {
		for (long y = 0; y < res.y; ++y) {
			for (long x = 0; x < res.x; ++x) {
				auto const x0 = grille.valeur(x - 1, y, z);
				auto const x1 = grille.valeur(x + 1, y, z);
				auto const y0 = grille.valeur(x, y - 1, z);
				auto const y1 = grille.valeur(x, y + 1, z);
				auto const z0 = grille.valeur(x, y, z - 1);
				auto const z1 = grille.valeur(x, y, z + 1);

				auto const divergence = (x1.x - x0.x) + (y1.y - y0.y) + (z1.z - z0.z);

				max_divergence = std::max(max_divergence, divergence);

				d.valeur(x, y, z, divergence);
			}
		}
	}

	return max_divergence;
}

void construit_preconditionneur(
		PCGSolver &pcg_solver,
		Grille<char> const &drapeaux)
{
	auto const res = drapeaux.resolution();

	constexpr auto T = 0.97f;

	for (long z = 0; z < res.z; ++z) {
		for (long y = 0; y < res.y; ++y) {
			for (long x = 0; x < res.x; ++x) {
				if (drapeaux.valeur(x, y, z) == 0) {
					pcg_solver.M.valeur(x, y, z, 0.0f);
					continue;
				}

				auto const pi = pcg_solver.M.valeur(x - 1, y, z);
				auto const pj = pcg_solver.M.valeur(x, y - 1, z);
				auto const pk = pcg_solver.M.valeur(x, y, z - 1);

				auto const Ad = pcg_solver.Adiag.valeur(x, y, z);

				auto const Aii = pcg_solver.Aplusi.valeur(x - 1, y, z);
				auto const Aij = pcg_solver.Aplusi.valeur(x, y - 1, z);
				auto const Aik = pcg_solver.Aplusi.valeur(x, y, z - 1);

				auto const Aji = pcg_solver.Aplusj.valeur(x - 1, y, z);
				auto const Ajj = pcg_solver.Aplusj.valeur(x, y - 1, z);
				auto const Ajk = pcg_solver.Aplusj.valeur(x, y, z - 1);

				auto const Aki = pcg_solver.Aplusk.valeur(x - 1, y, z);
				auto const Akj = pcg_solver.Aplusk.valeur(x, y - 1, z);
				auto const Akk = pcg_solver.Aplusk.valeur(x, y, z - 1);

				auto const e = Ad
							   - (Aii * pi)*(Aii * pi)
							   - (Ajj * pj)*(Ajj * pj)
							   - (Akk * pk)*(Akk * pk)
							   - T*(
								   Aii * (Aji + Aki) * pi * pi
								   + Aji * (Aij + Akj) * pj * pj
								   + Aki * (Aik + Ajk) * pk * pk);

				pcg_solver.M.valeur(x, y, z, 1.0f / std::sqrt(e + 10e-30f));
			}
		}
	}
}

void applique_preconditionneur(
		PCGSolver &pcg_solver,
		Grille<char> const &drapeaux)
{
	auto const res = pcg_solver.M.resolution();

	Grille<float> q(pcg_solver.M.etendu(), pcg_solver.M.fenetre_donnees(), pcg_solver.M.taille_voxel());
//	q.initialise(res.x, res.y, res.z);

	/* Résoud Lq = r */
	for (long z = 0; z < res.z; ++z) {
		for (long y = 0; y < res.y; ++y) {
			for (long x = 0; x < res.x; ++x) {
				if (drapeaux.valeur(x, y, z) == 0) {
					q.valeur(x, y, z, 0.0f);
					continue;
				}

				auto const r_ijk = pcg_solver.r.valeur(x, y, z);
				auto const M_ijk = pcg_solver.M.valeur(x, y, z);

				auto const A_i0jk = pcg_solver.Aplusi.valeur(x - 1, y, z);
				auto const A_ij0k = pcg_solver.Aplusj.valeur(x, y - 1, z);
				auto const A_ijk0 = pcg_solver.Aplusk.valeur(x, y, z - 1);

				auto const M_i0jk = pcg_solver.M.valeur(x - 1, y, z);
				auto const M_ij0k = pcg_solver.M.valeur(x, y - 1, z);
				auto const M_ijk0 = pcg_solver.M.valeur(x, y, z - 1);

				auto const q_i0jk = q.valeur(x - 1, y, z);
				auto const q_ij0k = q.valeur(x, y - 1, z);
				auto const q_ijk0 = q.valeur(x, y, z - 1);

				auto const t = r_ijk
							   - A_i0jk*M_i0jk*q_i0jk
							   - A_ij0k*M_ij0k*q_ij0k
							   - A_ijk0*M_ijk0*q_ijk0;


				q.valeur(x, y, z, t * M_ijk);
			}
		}
	}

	/* Résoud L^Tz = q */
	for (long z = res.z - 1; z >= 0; --z) {
		for (long y = res.y - 1; y >= 0; --y) {
			for (long x = res.x - 1; x >= 0; --x) {
				if (drapeaux.valeur(x, y, z) == 0) {
					q.valeur(x, y, z, 0.0f);
					continue;
				}

				auto const q_ijk = q.valeur(x, y, z);
				auto const M_ijk = pcg_solver.M.valeur(x, y, z);

				auto const Ai_ijk = pcg_solver.Aplusi.valeur(x, y, z);
				auto const Aj_ijk = pcg_solver.Aplusj.valeur(x, y, z);
				auto const Ak_ijk = pcg_solver.Aplusk.valeur(x, y, z);

				auto const z_i1jk = pcg_solver.z.valeur(x + 1, y, z);
				auto const z_ij1k = pcg_solver.z.valeur(x, y + 1, z);
				auto const z_ijk1 = pcg_solver.z.valeur(x, y, z + 1);

				auto const t = q_ijk
							   - Ai_ijk * M_ijk * z_i1jk
							   - Aj_ijk * M_ijk * z_ij1k
							   - Ak_ijk * M_ijk * z_ijk1;


				q.valeur(x, y, z, t * M_ijk);
			}
		}
	}
}

float produit_scalaire(Grille<float> const &a, Grille<float> const &b)
{
	auto const res_x = a.resolution().x;
	auto const res_y = a.resolution().y;
	auto const res_z = a.resolution().z;

	auto valeur = 0.0f;

	for (long z = 0; z < res_z; ++z) {
		for (long y = 0; y < res_y; ++y) {
			for (long x = 0; x < res_x; ++x) {
				valeur += a.valeur(x, y, z) * b.valeur(x, y, z);
			}
		}
	}

	return valeur;
}

float maximum(Grille<float> const &a)
{
	auto const res_x = a.resolution().x;
	auto const res_y = a.resolution().y;
	auto const res_z = a.resolution().z;

	auto max = std::numeric_limits<float>::min();

	for (long x = 0; x < res_x; ++x) {
		for (long y = 0; y < res_y; ++y) {
			for (long z = 0; z < res_z; ++z) {
				auto const v = std::abs(a.valeur(x, y, z));
				if (v > max) {
					max = v;
				}
			}
		}
	}

	return max;
}

void ajourne_pression_residus(const float alpha, Grille<float> &p, Grille<float> &r, Grille<float> const &a, Grille<float> const &b)
{
	auto const res_x = a.resolution().x;
	auto const res_y = a.resolution().y;
	auto const res_z = a.resolution().z;

	for (long x = 0; x < res_x; ++x) {
		for (long y = 0; y < res_y; ++y) {
			for (long z = 0; z < res_z; ++z) {
				auto vp = p.valeur(x, y, z);
				auto vr = r.valeur(x, y, z);
				auto vz = a.valeur(x, y, z);
				auto vs = b.valeur(x, y, z);

				p.valeur(x, y, z, vp + alpha * vs);
				r.valeur(x, y, z, vr - alpha * vz);
			}
		}
	}
}

void ajourne_vecteur_recherche(Grille<float> &s, Grille<float> const &a, const float beta)
{
	auto const res_x = s.resolution().x;
	auto const res_y = s.resolution().y;
	auto const res_z = s.resolution().z;

	for (long x = 0; x < res_x; ++x) {
		for (long y = 0; y < res_y; ++y) {
			for (long z = 0; z < res_z; ++z) {
				auto vs = s.valeur(x, y, z);
				auto vz = a.valeur(x, y, z);

				s.valeur(x, y, z, vz + beta * vs);
			}
		}
	}
}

/* z = A*s */
void applique_A(PCGSolver &pcg_solver)
{
	auto const res = pcg_solver.M.resolution();

	for (long z = 0; z < res.z; ++z) {
		for (long y = 0; y < res.y; ++y) {
			for (long x = 0; x < res.x; ++x) {
				auto const coef = pcg_solver.Adiag.valeur(x, y, z);

				auto const s_i0jk = pcg_solver.s.valeur(x - 1, y, z);
				auto const s_i1jk = pcg_solver.s.valeur(x + 1, y, z);
				auto const s_ij0k = pcg_solver.s.valeur(x, y - 1, z);
				auto const s_ij1k = pcg_solver.s.valeur(x, y + 1, z);
				auto const s_ijk0 = pcg_solver.s.valeur(x, y, z - 1);
				auto const s_ijk1 = pcg_solver.s.valeur(x, y, z + 1);

				auto const A_i0jk = pcg_solver.Aplusi.valeur(x - 1, y, z);
				auto const A_i1jk = pcg_solver.Aplusi.valeur(x, y, z);
				auto const A_ij0k = pcg_solver.Aplusj.valeur(x, y - 1, z);
				auto const A_ij1k = pcg_solver.Aplusj.valeur(x, y, z);
				auto const A_ijk0 = pcg_solver.Aplusk.valeur(x, y, z - 1);
				auto const A_ijk1 = pcg_solver.Aplusk.valeur(x, y, z);

				auto const v = coef * (s_i0jk*A_i0jk+s_i1jk*A_i1jk+s_ij0k*A_ij0k+s_ij1k*A_ij1k+s_ijk0*A_ijk0+s_ijk1*A_ijk1);

				pcg_solver.z.valeur(x, y, z, v);
			}
		}
	}
}

// https://www.cs.ubc.ca/~rbridson/fluidsimulation/fluids_notes.pdf page 34
void solve_pressure(PCGSolver &pcg_solver, Grille<char> const &drapeaux)
{
	construit_preconditionneur(pcg_solver, drapeaux);
	applique_preconditionneur(pcg_solver, drapeaux);

	pcg_solver.s.copie_donnees(pcg_solver.z);

	auto sigma = produit_scalaire(pcg_solver.z, pcg_solver.r);

	auto const tol = 1e-6f;
	auto const rho = 1.0f;
	auto const max_iter = 100;

	long i = 0;
	auto max_divergence = 0.0f;

	for (; i < max_iter; ++i) {
		applique_A(pcg_solver);

		auto ps = produit_scalaire(pcg_solver.z, pcg_solver.s);
		auto alpha = rho / ((ps == 0.0f) ? 1.0f : ps);

		std::cerr << "Itération : " << i << ", alpha " << alpha << '\n';
		std::cerr << "Itération : " << i << ", sigma " << sigma << '\n';

		ajourne_pression_residus(alpha, pcg_solver.p, pcg_solver.r, pcg_solver.z, pcg_solver.s);

		max_divergence = maximum(pcg_solver.r);

		if (max_divergence <= tol) {
			break;
		}

		std::cerr << "Nombre d'itération : " << i << ", max_divergence " << max_divergence << '\n';

		applique_preconditionneur(pcg_solver, drapeaux);

		auto sigma_new = produit_scalaire(pcg_solver.z, pcg_solver.r);

		auto beta = sigma_new / rho;

		/* s = z + beta*s */
		ajourne_vecteur_recherche(pcg_solver.s, pcg_solver.z, beta);

		sigma = sigma_new;
	}

	std::cerr << "Nombre d'itération : " << i << ", max_divergence " << max_divergence << '\n';
}

void construit_A(PCGSolver &pcg_solver, Grille<char> const &drapeaux)
{
	auto const &res = drapeaux.resolution();

	for (long z = 0; z < res.z; ++z) {
		for (long y = 0; y < res.y; ++y) {
			for (long x = 0; x < res.x; ++x) {
				//auto const p_i0jk = drapeaux.valeur(x - 1, y, z);
				auto const p_i1jk = drapeaux.valeur(x + 1, y, z);
				//auto const p_ij0k = drapeaux.valeur(x, y - 1, z);
				auto const p_ij1k = drapeaux.valeur(x, y + 1, z);
				//auto const p_ijk0 = drapeaux.valeur(x, y, z - 1);
				auto const p_ijk1 = drapeaux.valeur(x, y, z + 1);

				/* À FAIRE : compte le nombre de cellule qui ne sont pas solide */
				auto const coeff = 6.0f;

				pcg_solver.Adiag.valeur(x, y, z, coeff);

				pcg_solver.Aplusi.valeur(x, y, z, (p_i1jk == 0) ? -1.0f : 0.0f);
				pcg_solver.Aplusj.valeur(x, y, z, (p_ij1k == 0) ? -1.0f : 0.0f);
				pcg_solver.Aplusk.valeur(x, y, z, (p_ijk1 == 0) ? -1.0f : 0.0f);
			}
		}
	}
}

void soustrait_gradient_pression(GrilleMAC &grille, PCGSolver const &pcg_solver)
{
	auto const &res = grille.resolution();

	for (long z = 0; z < res.z; ++z) {
		for (long y = 0; y < res.y; ++y) {
			for (long x = 0; x < res.x; ++x) {
				auto const p_i0jk = pcg_solver.p.valeur(x - 1, y, z);
				auto const p_i1jk = pcg_solver.p.valeur(x + 1, y, z);
				auto const p_ij0k = pcg_solver.p.valeur(x, y - 1, z);
				auto const p_ij1k = pcg_solver.p.valeur(x, y + 1, z);
				auto const p_ijk0 = pcg_solver.p.valeur(x, y, z - 1);
				auto const p_ijk1 = pcg_solver.p.valeur(x, y, z + 1);

				auto const px = p_i1jk - p_i0jk;
				auto const py = p_ij1k - p_ij0k;
				auto const pz = p_ijk1 - p_ijk0;

//				auto const vx = grille.valeur(x + 1, y, z);
//				auto const vy = grille.valeur(x, y + 1, z);
//				auto const vz = grille.valeur(x, y, z + 1);

				grille.valeur(x + 1, y, z) -= px;
				grille.valeur(x, y + 1, z) -= py;
				grille.valeur(x, y, z + 1) -= pz;
			}
		}
	}
}

void rend_imcompressible(GrilleMAC &grille, Grille<char> const &drapeaux)
{
	auto const &res = grille.resolution();

	auto pcg_solver = PCGSolver(grille.etendu(), grille.fenetre_donnees(), grille.taille_voxel());

	//auto max_divergence = calcul_divergence(grille, grille, grille, pcg_solver.r);
	auto max_divergence = calcul_divergence(grille, pcg_solver.r);

	std::cerr << "---------------------------------------------------\n";
	std::cerr << "max_divergence avant PCG : " << max_divergence << '\n';

	/* Vérifie si le champs de vélocité est déjà (presque) non-divergent. */
	if (max_divergence <= 1e-6f) {
		std::cerr << "La vélocité n'est pas divergeante !\n";
		return;
	}

	construit_A(pcg_solver, drapeaux);

	solve_pressure(pcg_solver, drapeaux);

	soustrait_gradient_pression(grille, pcg_solver);
}

}  /* namespace poseidon */
#endif

static auto calcul_divergence(
		GrilleMAC const &velocite,
		Grille<int> const &drapeaux)
{
	auto divergence = Grille<float>(velocite.etendu(), velocite.fenetre_donnees(), velocite.taille_voxel());
	auto const res = velocite.resolution();
	auto const taille_dalle = res.x * res.y;
	auto const dx = velocite.taille_voxel();

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
		Grille<float> &pression,
		Grille<float> const &divergence,
		Grille<int> const &drapeaux)
{
	auto const iterations = 100;
	auto const res = pression.resolution();
	auto const taille_dalle = res.x * res.y;

	auto residue   = Grille<float>(pression.etendu(), pression.fenetre_donnees(), pression.taille_voxel());
	auto direction = Grille<float>(pression.etendu(), pression.fenetre_donnees(), pression.taille_voxel());
	auto q         = Grille<float>(pression.etendu(), pression.fenetre_donnees(), pression.taille_voxel());
	auto h         = Grille<float>(pression.etendu(), pression.fenetre_donnees(), pression.taille_voxel());
	auto precond   = Grille<float>(pression.etendu(), pression.fenetre_donnees(), pression.taille_voxel());

	auto nouveau_delta = tbb::parallel_reduce(
				tbb::blocked_range<int>(1, res.z - 1),
				0.0f,
				[&](tbb::blocked_range<int> const &plage, float init)
	{
			auto delta = init;

			for (auto z = plage.begin(); z < plage.end(); ++z) {
				for (auto y = 1; y < res.y - 1; ++y) {
					for (auto x = 1; x < res.x - 1; ++x) {
						auto index = x + (y + z * res.y) * res.x;
						// if the cell is a variable
						float Acenter = 0.0f;

						if (!est_obstacle(drapeaux, index)) {
							// set the matrix to the Poisson stencil in order
							if (!est_obstacle(drapeaux, index + 1)) Acenter += 1.0f;
							if (!est_obstacle(drapeaux, index - 1)) Acenter += 1.0f;
							if (!est_obstacle(drapeaux, index + res.x)) Acenter += 1.0f;
							if (!est_obstacle(drapeaux, index - res.x)) Acenter += 1.0f;
							if (!est_obstacle(drapeaux, index + taille_dalle)) Acenter += 1.0f;
							if (!est_obstacle(drapeaux, index - taille_dalle)) Acenter += 1.0f;

							residue.valeur(index) = divergence.valeur(index) - (Acenter * pression.valeur(index) +
																		 pression.valeur(index - 1) * (est_obstacle(drapeaux, index - 1) ? 0.0f : -1.0f) +
																		 pression.valeur(index + 1) * (est_obstacle(drapeaux, index + 1) ? 0.0f : -1.0f) +
																		 pression.valeur(index - res.x) * (est_obstacle(drapeaux, index - res.x) ? 0.0f : -1.0f)+
																		 pression.valeur(index + res.x) * (est_obstacle(drapeaux, index + res.x) ? 0.0f : -1.0f)+
																		 pression.valeur(index - taille_dalle) * (est_obstacle(drapeaux, index - taille_dalle) ? 0.0f : -1.0f)+
																		 pression.valeur(index + taille_dalle) * (est_obstacle(drapeaux, index + taille_dalle) ? 0.0f : -1.0f) );
						}
						else
						{
							residue.valeur(index) = 0.0f;
						}

						// P^-1
						if(Acenter < 1.0f)
							precond.valeur(index) = 0.0;
						else
							precond.valeur(index) = 1.0f / Acenter;

						// p = P^-1 * r
						direction.valeur(index) = residue.valeur(index) * precond.valeur(index);

						delta += residue.valeur(index) * direction.valeur(index);
					}
				}
			}

		return delta;
	},
	std::plus<float>());

	/* résoud r = b - Ax */

	auto const eps  = 1e-06f;
	auto residue_max = 2.0f * eps;
	auto i = 0;

	while ((i < iterations) && (residue_max > 0.001f * eps)) {
		auto alpha = tbb::parallel_reduce(
					tbb::blocked_range<int>(1, res.z - 1),
					0.0f,
					[&](tbb::blocked_range<int> const &plage, float init)
		{
				auto alpha_loc = init;

				for (auto z = plage.begin(); z < plage.end(); ++z) {
					for (auto y = 1; y < res.y - 1; ++y) {
						for (auto x = 1; x < res.x - 1; ++x) {
							auto index = x + (y + z * res.y) * res.x;
							// if the cell is a variable
							auto Acenter = 0.0f;

							if (!est_obstacle(drapeaux, index)) {
								// set the matrix to the Poisson stencil in order
								if (!est_obstacle(drapeaux, index + 1)) Acenter += 1.0f;
								if (!est_obstacle(drapeaux, index - 1)) Acenter += 1.0f;
								if (!est_obstacle(drapeaux, index + res.x)) Acenter += 1.0f;
								if (!est_obstacle(drapeaux, index - res.x)) Acenter += 1.0f;
								if (!est_obstacle(drapeaux, index + taille_dalle)) Acenter += 1.0f;
								if (!est_obstacle(drapeaux, index - taille_dalle)) Acenter += 1.0f;

								q.valeur(index) = Acenter * direction.valeur(index) +
										direction.valeur(index - 1) * (est_obstacle(drapeaux, index - 1) ? 0.0f : -1.0f) +
										direction.valeur(index + 1) * (est_obstacle(drapeaux, index + 1) ? 0.0f : -1.0f) +
										direction.valeur(index - res.x) * (est_obstacle(drapeaux, index - res.x) ? 0.0f : -1.0f) +
										direction.valeur(index + res.x) * (est_obstacle(drapeaux, index + res.x) ? 0.0f : -1.0f)+
										direction.valeur(index - taille_dalle) * (est_obstacle(drapeaux, index - taille_dalle) ? 0.0f : -1.0f) +
										direction.valeur(index + taille_dalle) * (est_obstacle(drapeaux, index + taille_dalle) ? 0.0f : -1.0f);
							}
							else {
								q.valeur(index) = 0.0f;
							}

							alpha_loc += direction.valeur(index) * q.valeur(index);
						}
					}
				}

			return alpha_loc;
		},
		std::plus<float>());

		if (std::abs(alpha) > 0.0f) {
			alpha = nouveau_delta / alpha;
		}

		auto const ancien_delta = nouveau_delta;

		/* x = x + alpha * d */
		auto p_dm = tbb::parallel_reduce(
					tbb::blocked_range<int>(1, res.z - 1),
					std::pair(0.0f, 0.0f),
					[&](tbb::blocked_range<int> const &plage, std::pair<float, float> init)
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
		},
		[](std::pair<float, float> p1, std::pair<float, float> p2)
{
	return std::pair(p1.first + p2.first, std::max(p1.second, p2.second));
});

		nouveau_delta = p_dm.first;
		residue_max = p_dm.second;

		auto const beta = nouveau_delta / ancien_delta;

		// d = h + beta * d
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
		GrilleMAC &velocite,
		Grille<float> &pression,
		Grille<int> const &drapeaux)
{
	auto res = pression.resolution();
	auto taille_dalle = res.x * res.y;
	auto dx_inv = 1.0f / pression.taille_voxel();

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
		GrilleMAC &velocite,
		Grille<float> &pression,
		Grille<int> const &drapeaux)
{
	//solvePressure(velocite, pression, drapeaux);

	//construct::rend_incompressible(velocite, 300);

#ifdef SOLVEUR_POSEIDON
	auto drapeaux = Grille<char>(velocite.etendu(), velocite.fenetre_donnees(), velocite.taille_voxel());

	auto res = velocite->resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;

	auto iter = IteratricePosition(limites);
	while (!iter.fini()) {
		auto pos = iter.suivante();
		auto i = static_cast<size_t>(pos.x);
		auto j = static_cast<size_t>(pos.y);
		auto k = static_cast<size_t>(pos.z);

		if (i == 0 || i == static_cast<size_t>(res[0]-1) || j==0 || j== static_cast<size_t>(res[1]-1) || k==0 || k== static_cast<size_t>(res[2]-1))
			drapeaux.valeur(i, j, k, '\1');
		else
			drapeaux.valeur(i, j, k, '\0');
	}

	poseidon::rend_imcompressible(velocite, drapeaux);
#endif

	auto res = velocite.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res;

	auto iter = IteratricePosition(limites);
	while (!iter.fini()) {
		auto pos = iter.suivante();
		auto i = static_cast<size_t>(pos.x);
		auto j = static_cast<size_t>(pos.y);
		auto k = static_cast<size_t>(pos.z);

		if (i == 0 || i == static_cast<size_t>(res[0]-1)) {
			velocite.valeur(i, j, k).x = 0.0f;
		}

		if (j == 0 || j == static_cast<size_t>(res[1]-1)) {
			velocite.valeur(i, j, k).y = 0.0f;
		}

		if (k == 0 || k == static_cast<size_t>(res[2]-1)) {
			velocite.valeur(i, j, k).z = 0.0f;
		}
	}

	auto divergence = calcul_divergence(velocite, drapeaux);

	resoud_pression(pression, divergence, drapeaux);

	projette_solution(velocite, pression, drapeaux);
}

}  /* namespace psn */
