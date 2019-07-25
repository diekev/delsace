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

#include "corps/volume.hh"

#include "fluide.hh"

struct GridMg;

static const bool CG_DEBUG = false;

//! Basic CG interface 
class GridCgInterface {
public:
	enum PreconditionType {
		PC_None=0,
		PC_ICP,
		PC_mICP,
		PC_MGP
	};

	GridCgInterface();

	virtual ~GridCgInterface() = default;

	// solving functions
	virtual bool iterate() = 0;
	virtual void solve(int maxIter) = 0;

	// precond
	virtual void setICPreconditioner(PreconditionType method, Grille<float> *A0, Grille<float> *Ai, Grille<float> *Aj, Grille<float> *Ak) = 0;
	virtual void setMGPreconditioner(PreconditionType method, GridMg* MG) = 0;

	// access
	virtual float getSigma() const = 0;
	virtual int getIterations() const = 0;
	virtual float getResNorm() const = 0;
	virtual void setAccuracy(float set) = 0;
	virtual float getAccuracy() const = 0;

	//! force reinit upon next iterate() call, can be used for doing multiple solves
	virtual void forceReinit() = 0;

	void setUseL2Norm(bool set);

protected:

	// use l2 norm of residualfor threshold? (otherwise uses max norm)
	bool mUseL2Norm{};
	char pad[7] = {};
};


//! Run single iteration of the cg solver
/*! the template argument determines the type of matrix multiplication,
	typically a ApplyMatrix kernel, another one is needed e.g. for the
	mesh-based wave equation solver */
class GridCg : public GridCgInterface {
public:
	//! constructor
	GridCg(Grille<float>& dst, Grille<float>& rhs, Grille<float>& residual, Grille<float>& search, const Grille<int>& flags, Grille<float>& tmp,
		   Grille<float>* A0, Grille<float>* pAi, Grille<float>* pAj, Grille<float>* pAk);
	~GridCg() = default;

	GridCg(GridCg const &) = default;
	GridCg &operator=(GridCg const &) = default;

	void doInit();
	bool iterate();
	void solve(int maxIter);
	//! init pointers, and copy values from "normal" matrix
	void setICPreconditioner(PreconditionType method, Grille<float> *A0, Grille<float> *Ai, Grille<float> *Aj, Grille<float> *Ak);
	void setMGPreconditioner(PreconditionType method, GridMg* MG);
	void forceReinit() { mInited = false; }

	// Accessors
	float getSigma() const { return mSigma; }
	int getIterations() const { return mIterations; }

	float getResNorm() const { return mResNorm; }

	void setAccuracy(float set) { mAccuracy=set; }
	float getAccuracy() const { return mAccuracy; }

protected:
	bool mInited = false;
	char pad2[3] = {};
	int mIterations = 0;
	// grids
	Grille<float>& mDst;
	Grille<float>& mRhs;
	Grille<float>& mResidual;
	Grille<float>& mSearch;
	const Grille<int>& mFlags;
	Grille<float>& mTmp;

	Grille<float> *mpA0 = nullptr;
	Grille<float> *mpAi = nullptr;
	Grille<float> *mpAj = nullptr;
	Grille<float> *mpAk = nullptr;

	//! preconditioning grids
	Grille<float> *mpPCA0 = nullptr;
	Grille<float> *mpPCAi = nullptr;
	Grille<float> *mpPCAj = nullptr;
	Grille<float> *mpPCAk = nullptr;
	GridMg* mMG = nullptr;

	PreconditionType mPcMethod{};

	//! sigma / residual
	float mSigma = 0.0f;
	//! accuracy of solver (max. residuum)
	float mAccuracy = 0.0f;
	//! norm of the residual
	float mResNorm = 0.0f;
}; // GridCg


//! Kernel: Apply symmetric stored Matrix

void ApplyMatrix (const Grille<int>& flags, Grille<float>& dst, const Grille<float>& src,
				  Grille<float>& A0, Grille<float>& Ai, Grille<float>& Aj, Grille<float>& Ak);
