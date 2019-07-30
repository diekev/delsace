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

#include "gradient_conjugue.hh"

#include "biblinternes/math/outils.hh"

#include "corps/iter_volume.hh"

static void verifie_nans(Grille<float> const &grille, char const *message0, char const *nom_grille)
{
	for (auto i = 0; i < grille.nombre_voxels(); ++i) {
		if (std::isnan(grille.valeur(i))) {
			std::cerr << message0 << " : " << nom_grille << " a des NANs !\n";
			break;
		}
	}
}

static void verifie_infs(Grille<float> const &grille, char const *message0, char const *nom_grille)
{
	for (auto i = 0; i < grille.nombre_voxels(); ++i) {
		if (std::isinf(grille.valeur(i))) {
			std::cerr << message0 << " : " << nom_grille << " a des INFs !\n";
			break;
		}
	}
}

void InvertCheckFluid (const Grille<int>& flags, Grille<float>& grid)
{
	for (auto idx = 0; idx < flags.nombre_voxels(); ++idx) {
		if (est_fluide(flags, idx) && grid.valeur(idx) > 0.0f){
			grid.valeur(idx) = 1.0f / grid.valeur(idx);
		}
	}
}

//! Kernel: Squared sum over grid
double somme_carree(const Grille<float>& grid)
{
	auto somme = 0.0;

	for (auto idx = 0; idx < grid.nombre_voxels(); ++idx) {
		somme += dls::math::carre(static_cast<double>(grid.valeur(idx)));
	}

	return somme;
}

template <typename T>
auto gridScaledAdd(Grille<T>& grid1, const Grille<T>& grid2, T facteur)
{
	for (auto idx = 0; idx < grid1.nombre_voxels(); ++idx) {
		grid1.valeur(idx) += grid2.valeur(idx) * facteur;
	}
}

template <typename T>
auto efface(Grille<T> &grille)
{
	for (auto idx = 0; idx < grille.nombre_voxels(); ++idx) {
		grille.valeur(idx) = static_cast<T>(0);
	}
}

//*****************************************************************************
//  Precondition helpers

//! Preconditioning a la Wavelet Turbulence (needs 4 add. grids)
void InitPreconditionIncompCholesky(const Grille<int>& flags,
									Grille<float>& A0, Grille<float>& Ai, Grille<float>& Aj, Grille<float>& Ak,
									Grille<float>& orgA0, Grille<float>& orgAi, Grille<float>& orgAj, Grille<float>& orgAk)
{
	// compute IC according to Golub and Van Loan
	A0.copie_donnees(orgA0);
	Ai.copie_donnees(orgAi);
	Aj.copie_donnees(orgAj);
	Ak.copie_donnees(orgAk);

	auto res = A0.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res - dls::math::vec3i(1);

	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos_iter = iter.suivante();
		auto i = static_cast<size_t>(pos_iter.x);
		auto j = static_cast<size_t>(pos_iter.y);
		auto k = static_cast<size_t>(pos_iter.z);

		if (est_fluide(flags, i,j,k)) {
			auto idx = A0.calcul_index(i,j,k);
			A0.valeur(idx) = std::sqrt(A0.valeur(idx));

			// correct left and top stencil in other entries
			// for i = k+1:n
			//  if (A(i,k) != 0)
			//    A(i,k) = A(i,k) / A(k,k)
			float invDiagonal = 1.0f / A0.valeur(idx);
			Ai.valeur(idx) *= invDiagonal;
			Aj.valeur(idx) *= invDiagonal;
			Ak.valeur(idx) *= invDiagonal;

			// correct the right and bottom stencil in other entries
			// for j = k+1:n
			//   for i = j:n
			//      if (A(i,j) != 0)
			//        A(i,j) = A(i,j) - A(i,k) * A(j,k)
			A0.valeur(i+1,j,k) -= dls::math::carre(Ai.valeur(idx));
			A0.valeur(i,j+1,k) -= dls::math::carre(Aj.valeur(idx));
			A0.valeur(i,j,k+1) -= dls::math::carre(Ak.valeur(idx));
		}
	}

	// invert A0 for faster computation later
	InvertCheckFluid (flags, A0);
}

//! Preconditioning using modified IC ala Bridson (needs 1 add. grid)
void InitPreconditionModifiedIncompCholesky2(const Grille<int>& flags,
											 Grille<float>&Aprecond,
											 Grille<float>&A0, Grille<float>& Ai, Grille<float>& Aj, Grille<float>& Ak)
{
	// compute IC according to Golub and Van Loan
	efface(Aprecond);

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

		if (!est_fluide(flags, i,j,k)) continue;

		auto const tau = 0.97f;
		auto const sigma = 0.25f;

		// compute modified incomplete cholesky
		float e = 0.0f;
		e = A0.valeur(i,j,k)
				- dls::math::carre(Ai.valeur(i-1,j,k) * Aprecond.valeur(i-1,j,k))
				- dls::math::carre(Aj.valeur(i,j-1,k) * Aprecond.valeur(i,j-1,k))
				- dls::math::carre(Ak.valeur(i,j,k-1) * Aprecond.valeur(i,j,k-1));

		e -= tau * (
					Ai.valeur(i-1,j,k) * (Aj.valeur(i-1,j,k) + Ak.valeur(i-1,j,k))* dls::math::carre(Aprecond.valeur(i-1,j,k)) +
					Aj.valeur(i,j-1,k) * (Ai.valeur(i,j-1,k) + Ak.valeur(i,j-1,k))* dls::math::carre(Aprecond.valeur(i,j-1,k)) +
					Ak.valeur(i,j,k-1) * (Ai.valeur(i,j,k-1) + Aj.valeur(i,j,k-1))* dls::math::carre(Aprecond.valeur(i,j,k-1)) +
					0.0f);

		// stability cutoff
		if(e < sigma * A0.valeur(i,j,k)){
			e = A0.valeur(i,j,k);
		}

		if (e != 0.0f) {
			Aprecond.valeur(i,j,k) = 1.0f / std::sqrt(e);
		}
	}
}

//! Preconditioning using multigrid ala Dick et al.
void InitPreconditionMultigrid(GridMg* MG, Grille<float>&A0, Grille<float>& Ai, Grille<float>& Aj, Grille<float>& Ak, float mAccuracy)
{
	// build multigrid hierarchy if necessary
	//	if (!MG->isASet()) {
	//		MG->setA(&A0, &Ai, &Aj, &Ak);
	//	}

	//	MG->setCoarsestLevelAccuracy(mAccuracy * 1E-4);
	//	MG->setSmoothing(1,1);
}

//! Apply WT-style ICP
void ApplyPreconditionIncompCholesky(Grille<float>& dst, Grille<float>& Var1, const Grille<int>& flags,
									 Grille<float>& A0, Grille<float>& Ai, Grille<float>& Aj, Grille<float>& Ak,
									 Grille<float>& orgA0, Grille<float>& orgAi, Grille<float>& orgAj, Grille<float>& orgAk)
{

	// forward substitution
	auto res = dst.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(1);
	limites.max = res;

	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos_iter = iter.suivante();
		auto i = static_cast<size_t>(pos_iter.x);
		auto j = static_cast<size_t>(pos_iter.y);
		auto k = static_cast<size_t>(pos_iter.z);

		if (!est_fluide(flags, i,j,k)){
			continue;
		}

		dst.valeur(i,j,k) = A0.valeur(i,j,k) * (Var1.valeur(i,j,k)
								  - dst.valeur(i-1,j,k) * Ai.valeur(i-1,j,k)
								  - dst.valeur(i,j-1,k) * Aj.valeur(i,j-1,k)
								  - dst.valeur(i,j,k-1) * Ak.valeur(i,j,k-1));
	}

	// backward substitution
	limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res - dls::math::vec3i(1);

	auto iter_inv = IteratricePositionInv(limites);

	while (!iter_inv.fini()) {
		auto pos_iter = iter_inv.suivante();
		auto i = static_cast<size_t>(pos_iter.x);
		auto j = static_cast<size_t>(pos_iter.y);
		auto k = static_cast<size_t>(pos_iter.z);
		auto const idx = A0.calcul_index(i,j,k);

		if (!est_fluide(flags, idx)) {
			continue;
		}

		dst.valeur(idx) = A0.valeur(idx) * (dst.valeur(idx)
											- dst.valeur(i+1,j,k) * Ai.valeur(idx)
											- dst.valeur(i,j+1,k) * Aj.valeur(idx)
											- dst.valeur(i,j,k+1) * Ak.valeur(idx));
	}
}

//! Apply Bridson-style mICP
void ApplyPreconditionModifiedIncompCholesky2(Grille<float>& dst, Grille<float>& Var1, const Grille<int>& flags,
											  Grille<float>& Aprecond,
											  Grille<float>& A0, Grille<float>& Ai, Grille<float>& Aj, Grille<float>& Ak)
{
	// forward substitution
	auto res = dst.resolution();
	auto limites = limites3i{};
	limites.min = dls::math::vec3i(1);
	limites.max = res;

	auto iter = IteratricePosition(limites);

	while (!iter.fini()) {
		auto pos_iter = iter.suivante();
		auto i = static_cast<size_t>(pos_iter.x);
		auto j = static_cast<size_t>(pos_iter.y);
		auto k = static_cast<size_t>(pos_iter.z);

		if (!est_fluide(flags, i, j, k)) {
			continue;
		}

		const float p = Aprecond.valeur(i,j,k);

		dst.valeur(i,j,k) = p * (Var1.valeur(i,j,k)
						  - dst.valeur(i-1,j,k) * Ai.valeur(i-1,j,k) * Aprecond.valeur(i-1,j,k)
						  - dst.valeur(i,j-1,k) * Aj.valeur(i,j-1,k) * Aprecond.valeur(i,j-1,k)
						  - dst.valeur(i,j,k-1) * Ak.valeur(i,j,k-1) * Aprecond.valeur(i,j,k-1));
	}

	// backward substitution
	limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = res - dls::math::vec3i(1);

	auto iter_inv = IteratricePositionInv(limites);

	while (!iter_inv.fini()) {
		auto pos_iter = iter_inv.suivante();
		auto i = static_cast<size_t>(pos_iter.x);
		auto j = static_cast<size_t>(pos_iter.y);
		auto k = static_cast<size_t>(pos_iter.z);
		auto const idx = A0.calcul_index(i,j,k);

		if (!est_fluide(flags, idx)) {
			continue;
		}

		const float p = Aprecond.valeur(idx);

		dst.valeur(idx) = p * (dst.valeur(idx)
						- dst.valeur(i+1,j,k) * Ai.valeur(idx) * p
						- dst.valeur(i,j+1,k) * Aj.valeur(idx) * p
						- dst.valeur(i,j,k+1) * Ak.valeur(idx) * p);
	}
}

//! Perform one Multigrid VCycle
void ApplyPreconditionMultigrid(GridMg* pMG, Grille<float>& dst, Grille<float>& Var1)
{
	// one VCycle on "A*dst = Var1" with initial guess dst=0
	//	pMG->setRhs(Var1);
	//	pMG->doVCycle(dst);
}


//*****************************************************************************
// Kernels

//! Kernel: Compute the dot product between two float grids
/*! Uses double precision internally */
double produit_scalaire(Grille<float> const &a, Grille<float> const &b)
{
	auto result = 0.0;

	for (auto idx = 0; idx < a.nombre_voxels(); ++idx) {
		result += static_cast<double>(a.valeur(idx) * b.valeur(idx));
	}

	return result;
}

float max_abs(Grille<float> const &grille)
{
	auto min =  std::numeric_limits<float>::max();
	auto max = -std::numeric_limits<float>::max();

	for (auto idx = 0; idx < grille.nombre_voxels(); ++idx) {
		auto v = grille.valeur(idx);

		if (v < min) {
			min = v;
		}
		if (v > max) {
			max = v;
		}
	}

	return std::max(std::abs(min), max);
}

//! Kernel: compute residual (init) and add to sigma
double InitSigma (const Grille<int>& flags, Grille<float>& dst, Grille<float>& rhs, Grille<float>& temp)
{
	auto sigma = 0.0;

	for (auto idx = 0; idx < flags.nombre_voxels(); ++idx) {
		auto const res = rhs.valeur(idx) - temp.valeur(idx);
		dst.valeur(idx) = res;

		// only compute residual in fluid region
		if (est_fluide(flags, idx)) {
			sigma += static_cast<double>(res * res);
		}
	}

	return sigma;
}

//! Kernel: update search vector
void UpdateSearchVec (Grille<float>& dst, Grille<float>& src, float factor)
{
	for (auto idx = 0; idx < dst.nombre_voxels(); ++idx) {
		dst.valeur(idx) = src.valeur(idx) + factor * dst.valeur(idx);
	}
}

//*****************************************************************************
//  CG class

GridCgInterface::GridCgInterface()
	: mUseL2Norm(true)
{}

void GridCgInterface::setUseL2Norm(bool set)
{
	mUseL2Norm = set;
}

static constexpr auto VECTOR_EPSILON = 1e-6f;

GridCg::GridCg(
		Grille<float>& dst,
		Grille<float>& rhs,
		Grille<float>& residual,
		Grille<float>& search,
		const Grille<int>& flags,
		Grille<float>& tmp,
		Grille<float>* pA0,
		Grille<float>* pAi,
		Grille<float>* pAj,
		Grille<float>* pAk)
	: GridCgInterface()
	, mInited(false)
	, mIterations(0)
	, mDst(dst)
	, mRhs(rhs)
	, mResidual(residual)
	, mSearch(search)
	, mFlags(flags)
	, mTmp(tmp)
	, mpA0(pA0)
	, mpAi(pAi)
	, mpAj(pAj)
	, mpAk(pAk)
	, mpPCA0(nullptr)
	, mpPCAi(nullptr)
	, mpPCAj(nullptr)
	, mpPCAk(nullptr)
	, mMG(nullptr)
	, mPcMethod(PC_None)
	, mSigma(0.0)
	, mAccuracy(VECTOR_EPSILON)
	, mResNorm(1e20f)
{}

void GridCg::doInit()
{
	mInited = true;
	mIterations = 0;

	efface(mDst);
	mResidual.copie_donnees(mRhs); // p=0, residual = b

	if (mPcMethod == PC_ICP) {
		InitPreconditionIncompCholesky(mFlags, *mpPCA0, *mpPCAi, *mpPCAj, *mpPCAk, *mpA0, *mpAi, *mpAj, *mpAk);
		ApplyPreconditionIncompCholesky(mTmp, mResidual, mFlags, *mpPCA0, *mpPCAi, *mpPCAj, *mpPCAk, *mpA0, *mpAi, *mpAj, *mpAk);
	}
	else if (mPcMethod == PC_mICP) {
		InitPreconditionModifiedIncompCholesky2(mFlags, *mpPCA0, *mpA0, *mpAi, *mpAj, *mpAk);
		ApplyPreconditionModifiedIncompCholesky2(mTmp, mResidual, mFlags, *mpPCA0, *mpA0, *mpAi, *mpAj, *mpAk);
	}
	else if (mPcMethod == PC_MGP) {
		InitPreconditionMultigrid(mMG, *mpA0, *mpAi, *mpAj, *mpAk, mAccuracy);
		ApplyPreconditionMultigrid(mMG, mTmp, mResidual);
	}
	else {
		mTmp.copie_donnees(mResidual);
	}

	mSearch.copie_donnees(mTmp);

	mSigma = static_cast<float>(produit_scalaire(mTmp, mResidual));
}

bool GridCg::iterate()
{
	if (!mInited) {
		doInit();
	}

	mIterations++;

	// create matrix application operator passed as template argument,
	// this could reinterpret the mpA pointers (not so clean right now)
	// tmp = applyMat(search)

	ApplyMatrix(mFlags, mTmp, mSearch, *mpA0, *mpAi, *mpAj, *mpAk);

	// alpha = sigma/dot(tmp, search)
	auto dp = produit_scalaire(mTmp, mSearch);
	auto alpha = 0.0f;

	if (std::abs(dp) > 0.0){
		alpha = mSigma / static_cast<float>(dp);
	}

	gridScaledAdd(mDst, mSearch, alpha);    // dst += search * alpha
	gridScaledAdd(mResidual, mTmp, -alpha); // residual += tmp * -alpha

	if (mPcMethod == PC_ICP) {
		ApplyPreconditionIncompCholesky(mTmp, mResidual, mFlags, *mpPCA0, *mpPCAi, *mpPCAj, *mpPCAk, *mpA0, *mpAi, *mpAj, *mpAk);
	}
	else if (mPcMethod == PC_mICP) {
		ApplyPreconditionModifiedIncompCholesky2(mTmp, mResidual, mFlags, *mpPCA0, *mpA0, *mpAi, *mpAj, *mpAk);
	}
	else if (mPcMethod == PC_MGP) {
		ApplyPreconditionMultigrid(mMG, mTmp, mResidual);
	}
	else {
		mTmp.copie_donnees(mResidual);
	}

	// use the l2 norm of the residual for convergence check? (usually max norm is recommended instead)
	if (this->mUseL2Norm) {
		mResNorm = static_cast<float>(somme_carree(mResidual));
	}
	else {
		mResNorm = max_abs(mResidual);
	}

	// abort here to safe some work...
	if (mResNorm < mAccuracy) {
		mSigma = mResNorm; // this will be returned later on to the caller...
		std::cerr << "Arrêt au bout de " << mIterations
				  << " car mResNorm (" << mResNorm << ") < mAccuracy (" << mAccuracy << ")!\n";
		return false;
	}

	auto sigmaNew = static_cast<float>(produit_scalaire(mTmp, mResidual));
	auto beta = sigmaNew / mSigma;

	// search =  tmp + beta * search
	UpdateSearchVec (mSearch, mTmp, beta);

	std::cerr <<"GridCg::iterate i="<<mIterations<<" sigmaNew="<<sigmaNew<<" sigmaLast="<<mSigma<<" alpha="<<alpha<<" beta="<<beta<<"\n";
	mSigma = sigmaNew;

	if(!(static_cast<double>(mResNorm) < 1e35)) {
		if(mPcMethod == PC_MGP) {
			// diverging solves can be caused by the static multigrid mode, we cannot detect this here, though
			// only the pressure solve call "knows" whether the MG is static or dynamics...
			//debMsg("GridCg::iterate: Warning - this diverging solve can be caused by the 'static' mode of the MG preconditioner. If the static mode is active, try switching to dynamic.", 1);
		}
		std::cerr << "GridCg::iterate: The CG solver diverged, residual norm ("<<mResNorm<<") > 1e30, stopping.\n";
		return false;
	}

	//std::cerr <<"PB-CG-Norms::p"<<sqrt(GridOpNormNosqrt(mpDst, mpFlags).getValue()) <<" search"<<sqrt(GridOpNormNosqrt(mpSearch, mpFlags).getValue(), CG_DEBUGLEVEL) <<" res"<<sqrt(GridOpNormNosqrt(mpResidual, mpFlags).getValue()) <<" tmp"<<sqrt(GridOpNormNosqrt(mpTmp, mpFlags).getValue()); // debug
	return true;
}

void GridCg::solve(int maxIter) {
	for (int iter=0; iter<maxIter; iter++) {
		if (!iterate()) iter=maxIter;
	}
	return;
}

void GridCg::setICPreconditioner(PreconditionType method, Grille<float> *A0, Grille<float> *Ai, Grille<float> *Aj, Grille<float> *Ak)
{
	//assertMsg(method==PC_ICP || method==PC_mICP, "GridCg<APPLYMAT>::setICPreconditioner: Invalid method specified.");

	mPcMethod = method;
	mpPCA0 = A0;
	mpPCAi = Ai;
	mpPCAj = Aj;
	mpPCAk = Ak;
}

void GridCg::setMGPreconditioner(PreconditionType method, GridMg* MG)
{
	//assertMsg(method==PC_MGP, "GridCg<APPLYMAT>::setMGPreconditioner: Invalid method specified.");

	mPcMethod = method;
	mMG = MG;
}

// explicit instantiation

//*****************************************************************************
// diffusion for real and vec grids, e.g. for viscosity

#if 0
//! do a CG solve for diffusion; note: diffusion coefficient alpha given in grid space,
//  rescale in python file for discretization independence (or physical correspondence)
//  see lidDrivenCavity.py for an example
void cgSolveDiffusion(
		const Grille<int>& flags,
		GridBase& grid,
		float alpha = 0.25,
		float cgMaxIterFac = 1.0,
		float cgAccuracy   = 1e-4)
{
	// reserve temp grids
	FluidSolver* parent = flags.getParent();
	Grille<float> rhs(parent);
	Grille<float> residual(parent), search(parent), tmp(parent);
	Grille<float> A0(parent), Ai(parent), Aj(parent), Ak(parent);

	// setup matrix and boundaries
	Grille<int> flagsDummy(parent);
	flagsDummy.setConst(Grille<int>::TypeFluid);
	MakeLaplaceMatrix (flagsDummy, A0, Ai, Aj, Ak);

	FOR_IJK(flags) {
		if(flags.isObstacle(i,j,k)) {
			Ai(i,j,k)  = Aj(i,j,k)  = Ak(i,j,k)  = 0.0;
			A0(i,j,k)  = 1.0;
		} else {
			Ai(i,j,k) *= alpha;
			Aj(i,j,k) *= alpha;
			Ak(i,j,k) *= alpha;
			A0(i,j,k) *= alpha;
			A0(i,j,k) += 1.;
		}
	}

	GridCgInterface *gcg;
	// note , no preconditioning for now...
	const int maxIter = (int)(cgMaxIterFac * flags.getSize().max()) * (flags.is3D() ? 1 : 4);

	if (grid.getType() & GridBase::Typefloat) {
		auto &u = ((Grille<float>&) grid);
		rhs.copie_donnees(u);
		gcg = new GridCg<ApplyMatrix  >(u, rhs, residual, search, flags, tmp, &A0, &Ai, &Aj, &Ak);
		gcg->setAccuracy(cgAccuracy);
		gcg->solve(maxIter);

		debMsg("FluidSolver::solveDiffusion iterations:"<<gcg->getIterations()<<", res:"<<gcg->getSigma(), CG_DEBUGLEVEL);
	}
	else if((grid.getType() & GridBase::TypeVec3))
	{
		Grille<Vec3>& vec = ((Grille<Vec3>&) grid);
		Grille<float> u(parent);

		// core solve is same as for a regular real grid
		gcg = new GridCg<ApplyMatrix  >(u, rhs, residual, search, flags, tmp, &A0, &Ai, &Aj, &Ak);
		gcg->setAccuracy(cgAccuracy);

		// diffuse every component separately
		for(int component = 0; component< (grid.is3D() ? 3:2); ++component) {
			getComponent(vec, u, component);
			gcg->forceReinit();

			rhs.copie_donnees(u);
			gcg->solve(maxIter);
			debMsg("FluidSolver::solveDiffusion vec3, iterations:"<<gcg->getIterations()<<", res:"<<gcg->getSigma(), CG_DEBUGLEVEL);

			setComponent(u, vec, component);
		}
	} else {
		//errMsg("cgSolveDiffusion: Grid Type is not supported (only float, Vec3, MAC, or Levelset)");
	}

	delete gcg;
}
#endif

void ApplyMatrix(const Grille<int> &flags, Grille<float> &dst, const Grille<float> &src, Grille<float> &A0, Grille<float> &Ai, Grille<float> &Aj, Grille<float> &Ak)
{
	auto const X = 1;
	auto const Y = flags.resolution().x;
	auto const Z = flags.resolution().x * flags.resolution().y;

	for (auto idx = 0; idx < flags.nombre_voxels(); ++idx) {
		if (!est_fluide(flags, idx)) {
			dst.valeur(idx) = src.valeur(idx);
			return;
		}

		dst.valeur(idx) =  src.valeur(idx) * A0.valeur(idx)
				+ src.valeur(idx - X) * Ai.valeur(idx - X)
				+ src.valeur(idx + X) * Ai.valeur(idx)
				+ src.valeur(idx - Y) * Aj.valeur(idx - Y)
				+ src.valeur(idx + Y) * Aj.valeur(idx)
				+ src.valeur(idx - Z) * Ak.valeur(idx - Z)
				+ src.valeur(idx + Z) * Ak.valeur(idx);
	}
}
