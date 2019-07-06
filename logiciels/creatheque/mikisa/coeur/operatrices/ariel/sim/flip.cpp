// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: flip.cpp
// Implements the FLIP sim

#include "flip.hpp"

namespace fluidCore{

/* math utils */

static auto Sqrlength(const dls::math::vec3f& p0, const dls::math::vec3f& p1)
{
	auto a = p0.x - p1.x;
	auto b = p0.y - p1.y;
	auto c = p0.z - p1.z;
	return a*a + b*b + c*c;
}

static auto Smooth(const float& r2, const float& h)
{
	return std::max(1.0f-r2/(h*h), 0.0f);
}

static auto Sharpen(const float& r2, const float& h)
{
	return std::max(h*h / std::max(r2, 1.0e-5f) - 1.0f, 0.0f);
}

/* pressure solve */

//====================================
// Function Implementations
//====================================

//Takes a grid, multiplies everything by -1
static void FlipGrid(Grid<float>* grid, dls::math::vec3i dimensions)
{
	auto x = dimensions.x;
	auto y = dimensions.y;
	auto z = dimensions.z;

	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j=0; j<y; ++j) {
				for (auto k=0; k<z; ++k) {
					auto flipped = -grid->GetCell(i,j,k);
					grid->SetCell(i,j,k,flipped);
				}
			}
		}
	});
}

//Helper for preconditioner builder
static auto ARef(Grid<int>* A, int i, int j, int k, int qi, int qj, int qk, dls::math::vec3i dimensions)
{
	auto x = dimensions.x;
	auto y = dimensions.y;
	auto z = dimensions.z;

	if ( i<0 || i>x-1 || j<0 || j>y-1 || k<0 || k>z-1 || A->GetCell(i,j,k)!=FLUID ) { //if not liquid
		return 0.0f;
	}
	//if not liquid
	if ( qi<0 || qi>x-1 || qj<0 || qj>y-1 || qk<0 || qk>z-1 || A->GetCell(qi,qj,qk)!=FLUID ) {
		return 0.0f;
	}
	return -1.0f;
}

//Helper for preconditioner builder
static auto PRef(Grid<float>* p, int i, int j, int k, dls::math::vec3i dimensions)
{
	auto x = dimensions.x;
	auto y = dimensions.y;
	auto z = dimensions.z;

	if ( i<0 || i>x-1 || j<0 || j>y-1 || k<0 || k>z-1 || static_cast<int>(p->GetCell(i,j,k))!=FLUID ) { //if not liquid
		return 0.0f;
	}

	return p->GetCell(i,j,k);
}

//Helper for preconditioner builder
static auto ADiag(Grid<int>* A, Grid<float>* L, int i, int j, int k, dls::math::vec3i dimensions, int subcell)
{
	auto x = dimensions.x;
	auto y = dimensions.y;
	auto z = dimensions.z;

	auto diag = 6.0f;
	if ( A->GetCell(i,j,k) != FLUID ) {
		return diag;
	}

	int q[][3] = { {i-1,j,k}, {i+1,j,k}, {i,j-1,k}, {i,j+1,k}, {i,j,k-1}, {i,j,k+1} };
	for ( int m=0; m<6; m++ ) {
		int qi = q[m][0];
		int qj = q[m][1];
		int qk = q[m][2];
		if ( qi<0 || qi>x-1 || qj<0 || qj>y-1 || qk<0 || qk>z-1 || A->GetCell(qi,qj,qk)==SOLID ) {
			diag -= 1.0f;
		}
		else if ( A->GetCell(qi,qj,qk)==AIR && subcell ) {
			diag -= L->GetCell(qi,qj,qk) / std::min(1.0e-6f, L->GetCell(i,j,k));
		}
	}

	return diag;
}

//Does what it says
static void BuildPreconditioner(Grid<float>* pc, MacGrid& mgrid, int subcell)
{
	auto x = mgrid.m_dimensions.x;
	auto y = mgrid.m_dimensions.y;
	auto z = mgrid.m_dimensions.z;

	auto a = 0.25f;
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j=0; j<y; ++j) {
				for (auto k=0; k<z; ++k) {
					if (mgrid.m_A->GetCell(i,j,k)==FLUID) {
						auto left = ARef(mgrid.m_A,i-1,j,k,i,j,k,mgrid.m_dimensions) *
								PRef(pc,i-1,j,k,mgrid.m_dimensions);
						auto bottom = ARef(mgrid.m_A,i,j-1,k,i,j,k,mgrid.m_dimensions) *
								PRef(pc,i,j-1,k,mgrid.m_dimensions);
						auto back = ARef(mgrid.m_A,i,j,k-1,i,j,k,mgrid.m_dimensions) *
								PRef(pc,i,j,k-1,mgrid.m_dimensions);
						auto diag = ADiag(mgrid.m_A, mgrid.m_L,i,j,k,mgrid.m_dimensions,
										  subcell);
						auto e = diag - (left*left) - (bottom*bottom) - (back*back);
						if (diag>0) {
							if ( e < a*diag ) {
								e = diag;
							}
							pc->SetCell(i,j,k, 1.0f / std::sqrt(e));
						}
					}
				}
			}
		}
	});
}

//Helper for PCG solver: read X with clamped bounds
static auto XRef(
		Grid<int>* A,
		Grid<float>* L,
		Grid<float>* X,
		dls::math::vec3f const &f,
		dls::math::vec3f const &p,
		dls::math::vec3i const &dimensions,
		int subcell)
{
	auto x = dimensions.x;
	auto y = dimensions.y;
	auto z = dimensions.z;

	auto i = std::min(std::max(0,static_cast<int>(p.x)),x-1);
	auto j = std::min(std::max(0,static_cast<int>(p.y)),y-1);
	auto k = std::min(std::max(0,static_cast<int>(p.z)),z-1);

	auto fi = static_cast<int>(f.x);
	auto fj = static_cast<int>(f.y);
	auto fk = static_cast<int>(f.z);

	if (A->GetCell(i,j,k) == FLUID) {
		return X->GetCell(i,j,k);
	}

	if (A->GetCell(i,j,k) == SOLID) {
		return X->GetCell(fi,fj,fk);
	}

	if (subcell) {
		return L->GetCell(i,j,k)/std::min(1.0e-6f,L->GetCell(fi,fj,fk))*X->GetCell(fi,fj,fk);
	}

	return 0.0f;
}

// target = X + alpha*Y
static void Op(
		Grid<int>* A,
		Grid<float>* X,
		Grid<float>* Y,
		Grid<float>* target,
		float alpha,
		dls::math::vec3i const &dimensions)
{
	auto x = dimensions.x;
	auto y = dimensions.y;
	auto z = dimensions.z;

	for (auto j=0; j<y; ++j) {
		for (auto k=0; k<z; ++k) {
			//this parallel loop has to be the inner loop or else MSVC will barf
			tbb::parallel_for(tbb::blocked_range<int>(0,x),
							  [&](const tbb::blocked_range<int>& r)
			{
				for (auto i=r.begin(); i!=r.end(); ++i) {
					if (A->GetCell(i,j,k)==FLUID) {
						auto targetval = X->GetCell(i,j,k)+alpha*Y->GetCell(i,j,k);
						target->SetCell(i,j,k,targetval);
					}else {
						target->SetCell(i,j,k,0.0f);
					}
				}
			});
		}
	}
}

// ans = x^T * x
static auto Product(Grid<int>* A, Grid<float>* X, Grid<float>* Y, dls::math::vec3i dimensions)
{
	auto x = dimensions.x;
	auto y = dimensions.y;
	auto z = dimensions.z;

	auto result = 0.0f;
	for (auto i=0; i<x; i++) {
		for (auto j=0; j<y; j++) {
			for (auto k=0; k<z; k++) {
				if (A->GetCell(i,j,k)==FLUID) {
					result += X->GetCell(i,j,k) * Y->GetCell(i,j,k);
				}
			}
		}
	}
	return result;
}

//Helper for PCG solver: target = AX
static void ComputeAx(Grid<int>* A, Grid<float>* L, Grid<float>* X, Grid<float>* target,
					  dls::math::vec3i dimensions, int subcell)
{
	auto x = dimensions.x;
	auto y = dimensions.y;
	auto z = dimensions.z;
	auto n = static_cast<float>(std::max(std::max(x, y), z));
	auto h = 1.0f/(n*n);

	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j=0; j<y; ++j) {
				for (auto k=0; k<z; ++k) {
					if (A->GetCell(i,j,k) == FLUID) {
						auto i_f = static_cast<float>(i);
						auto j_f = static_cast<float>(j);
						auto k_f = static_cast<float>(k);

						auto result = (6.0f*X->GetCell(i,j,k)
									   -XRef(A, L, X, dls::math::vec3f(i_f, j_f, k_f), dls::math::vec3f(i_f + 1.0f, j_f, k_f),
											 dimensions, subcell)
									   -XRef(A, L, X, dls::math::vec3f(i_f, j_f, k_f), dls::math::vec3f(i_f - 1.0f, j_f, k_f),
											 dimensions, subcell)
									   -XRef(A, L, X, dls::math::vec3f(i_f, j_f, k_f), dls::math::vec3f(i_f, j_f + 1.0f, k_f),
											 dimensions, subcell)
									   -XRef(A, L, X, dls::math::vec3f(i_f, j_f, k_f), dls::math::vec3f(i_f, j_f - 1.0f, k_f),
											 dimensions, subcell)
									   -XRef(A, L, X, dls::math::vec3f(i_f, j_f, k_f), dls::math::vec3f(i_f, j_f, k_f + 1.0f),
											 dimensions, subcell)
									   -XRef(A, L, X, dls::math::vec3f(i_f, j_f, k_f), dls::math::vec3f(i_f, j_f, k_f - 1.0f),
											 dimensions, subcell)
									   )/h;
						target->SetCell(i,j,k,result);
					}
					else {
						target->SetCell(i,j,k,0.0f);
					}
				}
			}
		}
	});
}

static void ApplyPreconditioner(
		Grid<float>* Z,
		Grid<float>* R,
		Grid<float>* P,
		Grid<float>* /*L*/,
		Grid<int>* A,
		dls::math::vec3i const &dimensions)
{
	auto x = dimensions.x;
	auto y = dimensions.y;
	auto z = dimensions.z;

	Grid<float>* Q = new Grid<float>(dimensions, 0.0f);

	// LQ = R
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j=0; j<y; ++j) {
				for (auto k=0; k<z; ++k) {
					if (A->GetCell(i,j,k) == FLUID) {
						auto left = ARef(A,i-1,j,k,i,j,k,dimensions)*
								PRef(P,i-1,j,k,dimensions)*PRef(Q,i-1,j,k,dimensions);
						auto bottom = ARef(A,i,j-1,k,i,j,k,dimensions)*
								PRef(P,i,j-1,k,dimensions)*PRef(Q,i,j-1,k,dimensions);
						auto back = ARef(A,i,j,k-1,i,j,k,dimensions)*
								PRef(P,i,j,k-1,dimensions)*PRef(Q,i,j,k-1,dimensions);

						auto t = R->GetCell(i,j,k) - left - bottom - back;
						auto qVal = t * P->GetCell(i,j,k);
						Q->SetCell(i,j,k,qVal);
					}
				}
			}
		}
	});

	// L^T Z = Q
	for (auto j=y-1; j>=0; j--) {
		for (auto k=z-1; k>=0; k--) {
			//this parallel loop has to be the inner loop or else MSVC will barf
			tbb::parallel_for(tbb::blocked_range<int>(-1,x-1),
							  [&](const tbb::blocked_range<int>& r)
			{
				for (int i=r.end(); i!=r.begin(); i--) {
					if (A->GetCell(i,j,k) == FLUID) {
						auto right = ARef(A,i,j,k,i+1,j,k,dimensions)*
								PRef(P,i,j,k,dimensions)*PRef(Z,i+1,j,k,dimensions);
						auto top = ARef(A,i,j,k,i,j+1,k,dimensions)*
								PRef(P,i,j,k,dimensions)*PRef(Z,i,j+1,k,dimensions);
						auto front = ARef(A,i,j,k,i,j,k+1,dimensions)*
								PRef(P,i,j,k,dimensions)*PRef(Z,i,j,k+1,dimensions);

						auto t = Q->GetCell(i,j,k) - right - top - front;
						auto zVal = t * P->GetCell(i,j,k);
						Z->SetCell(i,j,k,zVal);
					}
				}
			});
		}
	}

	delete Q;
}

//Does what it says
static void SolveConjugateGradient(MacGrid& mgrid, Grid<float>* PC, int subcell, const bool& verbose)
{
	auto x = mgrid.m_dimensions.x;
	auto y = mgrid.m_dimensions.y;
	auto z = mgrid.m_dimensions.z;

	auto R = new Grid<float>(mgrid.m_dimensions, 0.0f);
	auto Z = new Grid<float>(mgrid.m_dimensions, 0.0f);
	auto S = new Grid<float>(mgrid.m_dimensions, 0.0f);

	//note: we're calling pressure "mgrid.P" instead of x

	ComputeAx(mgrid.m_A, mgrid.m_L, mgrid.m_P, Z, mgrid.m_dimensions, subcell); // z = apply A(x)
	Op(mgrid.m_A, mgrid.m_D, Z, R, -1.0f, mgrid.m_dimensions);                // r = b-Ax
	auto error0 = Product(mgrid.m_A, R, R, mgrid.m_dimensions);            // error0 = product(r,r)

	// z = f(r), aka preconditioner step
	ApplyPreconditioner(Z, R, PC, mgrid.m_L, mgrid.m_A, mgrid.m_dimensions);

	//s = z. TODO: replace with VDB deep copy?

	for (auto j=0; j<y; ++j ) {
		for (auto k=0; k<z; ++k ) {
			//this parallel loop has to be the inner loop or else MSVC will barf
			tbb::parallel_for(tbb::blocked_range<int>(0,x),
							  [&](const tbb::blocked_range<int>& r)
			{
				for (auto i=r.begin(); i!=r.end(); ++i) {
					S->SetCell(i,j,k,Z->GetCell(i,j,k));
				}
			});
		}
	}

	auto eps = 1.0e-2f * static_cast<float>(x*y*z);
	auto a = Product(mgrid.m_A, Z, R, mgrid.m_dimensions);                 // a = product(z,r)

	for ( int k=0; k<x*y*z; k++) {
		//Solve current iteration
		ComputeAx(mgrid.m_A, mgrid.m_L, S, Z, mgrid.m_dimensions, subcell); // z = applyA(s)
		auto alpha = a/Product(mgrid.m_A, Z, S, mgrid.m_dimensions);       // alpha = a/(z . s)
		Op(mgrid.m_A, mgrid.m_P, S, mgrid.m_P, alpha, mgrid.m_dimensions);  // x = x + alpha*s
		Op(mgrid.m_A, R, Z, R, -alpha, mgrid.m_dimensions);                 // r = r - alpha*z;
		auto error1 = Product(mgrid.m_A, R, R, mgrid.m_dimensions);        // error1 = product(r,r)
		error0 = std::max(error0, error1);
		//Output progress
		auto rate = 1.0f - std::max(0.0f,std::min(1.0f,(error1-eps)/(error0-eps)));
		if (verbose) {
		std::cout << "PCG Iteration " << k+1 << ": " << 100.0f * std::pow(rate, 6.0f) << "% solved"
				  << std::endl;
		}

		if (error1<=eps || k == 100) {
			break;
		}

		//Prep next iteration
		// z = f(r)
		ApplyPreconditioner(Z, R, PC, mgrid.m_L, mgrid.m_A, mgrid.m_dimensions);
		auto a2 = Product(mgrid.m_A, Z, R, mgrid.m_dimensions);            // a2 = product(z,r)
		auto beta = a2/a;                                                  // beta = a2/a
		Op(mgrid.m_A, Z, S, S, beta, mgrid.m_dimensions);                   // s = z + beta*s
		a = a2;
	}

	delete R;
	delete Z;
	delete S;
}

static void Solve(MacGrid& mgrid, const int& subcell, const bool& verbose)
{
	//if in VDB mode, force to single threaded to prevent VDB write issues.
	//this is a kludgey fix for now.
	// if (mgrid.type==VDB) {
	//  omp_set_num_threads(1);
	// }

	//flip divergence
	FlipGrid(mgrid.m_D, mgrid.m_dimensions);

	//build preconditioner
	Grid<float>* preconditioner = new Grid<float>(mgrid.m_dimensions, 0.0f);
	BuildPreconditioner(preconditioner, mgrid, subcell);

	//solve conjugate gradient
	SolveConjugateGradient(mgrid, preconditioner, subcell, verbose);

	delete preconditioner;

	// if (mgrid.type==VDB) {
	//  omp_set_num_threads(omp_get_num_procs());
	// }
}

/* particles resampling */

static dls::math::vec3f Resample(
		ParticleGrid* pgrid,
		const dls::math::vec3f& p,
		const dls::math::vec3f& u,
		float re,
		const dls::math::vec3i& dimensions)
{
	auto nx = dimensions.x;
	auto ny = dimensions.y;
	auto nz = dimensions.z;
	auto maxd = static_cast<float>(std::max(std::max(nx, ny), nz));

	auto wsum = 0.0f;
	auto ru = dls::math::vec3f(0.0f);

	auto x = std::max(0.0f, std::min(maxd - 1.0f, maxd * p.x));
	auto y = std::max(0.0f, std::min(maxd - 1.0f, maxd * p.y));
	auto z = std::max(0.0f, std::min(maxd - 1.0f, maxd * p.z));

	auto neighbors = pgrid->GetCellNeighbors(
				dls::math::vec3f(x, y, z),
				dls::math::vec3f(1.0f));

	for (auto np : neighbors) {
		if (np->m_type == FLUID) {
			auto dist2 = Sqrlength(p,np->m_p);
			auto w = np->m_mass * Sharpen(dist2,re);
			ru += w * np->m_u;
			wsum += w;
		}
	}

	if (wsum != 0.0f) {
		ru /= wsum;
	}
	else {
		ru = u;
	}

	return ru;
}

static void ResampleParticles(ParticleGrid* pgrid, std::vector<Particle*>& particles,
							  sceneCore::Scene* scene, const float& frame, const float& dt,
							  const float& re, const dls::math::vec3i& dimensions)
{
	auto nx = dimensions.x;
	auto ny = dimensions.y;
	auto nz = dimensions.z;
	auto maxd = static_cast<float>(std::max(std::max(nx, ny), nz));
	pgrid->Sort(particles);

	auto springforce = 50.0f;

	//use springs to temporarily displace particles
	auto particleCount = particles.size();
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particleCount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto n0=r.begin(); n0!=r.end(); ++n0) {
			if (particles[n0]->m_type==FLUID) {
				Particle* p = particles[n0];
				dls::math::vec3f spring(0.0f, 0.0f, 0.0f);
				auto x = std::max(0.0f,std::min(maxd,maxd*p->m_p.x));
				auto y = std::max(0.0f,std::min(maxd,maxd*p->m_p.y));
				auto z = std::max(0.0f,std::min(maxd,maxd*p->m_p.z));
				std::vector<Particle*> neighbors = pgrid->GetCellNeighbors(dls::math::vec3f(x,y,z),
																		   dls::math::vec3f(1.0f));
				auto neighborsCount = neighbors.size();
				for (auto n1=0ul; n1<neighborsCount; ++n1) {
					Particle* np = neighbors[n1];
					if (p!=np) {
						auto dist = longueur(p->m_p-np->m_p);
						auto w = springforce * np->m_mass * Smooth(dist*dist,re);
						if (dist > 0.1f*re) {
							spring.x += w * (p->m_p.x-np->m_p.x) / dist * re;
							spring.y += w * (p->m_p.y-np->m_p.y) / dist * re;
							spring.z += w * (p->m_p.z-np->m_p.z) / dist * re;
						}
						else {
							if (np->m_type == FLUID) {
								spring.x += 0.01f * re / dt * static_cast<float>(rand() % 101) / 100.0f;
								spring.y += 0.01f * re / dt * static_cast<float>(rand() % 101) / 100.0f;
								spring.z += 0.01f * re / dt * static_cast<float>(rand() % 101) / 100.0f;
							}
							else {
								spring.x += 0.05f*re/dt*np->m_n.x;
								spring.y += 0.05f*re/dt*np->m_n.y;
								spring.z += 0.05f*re/dt*np->m_n.z;
							}
						}
					}
				}
				p->m_t.x = p->m_p.x + dt*spring.x;
				p->m_t.y = p->m_p.y + dt*spring.y;
				p->m_t.z = p->m_p.z + dt*spring.z;
			}
		}
	});

	particleCount = particles.size();
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particleCount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto n=r.begin(); n!=r.end(); ++n) {
			if (particles[n]->m_type == FLUID) {
				Particle* p = particles[n];
				p->m_t2.x = p->m_u.x;
				p->m_t2.y = p->m_u.y;
				p->m_t2.z = p->m_u.z;
				p->m_t2 = Resample(pgrid, p->m_t, p->m_t2, re, dimensions);
			}
		}
	});

	particleCount = particles.size();
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particleCount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto n=r.begin(); n!=r.end(); ++n) {
			if (particles[n]->m_type == FLUID) {
				Particle *p = particles[n];
				unsigned int solidGeomID = 0;
				if (scene->CheckPointInsideSolidGeom(p->m_t * maxd, frame, solidGeomID)==false) {
					p->m_p = p->m_t;
					p->m_u = p->m_t2;
				}
			}
		}
	});
}

/* particle grid operations */

static auto CheckWall(Grid<int>* A, const int& x, const int& y, const int& z)
{
	return (A->GetCell(x, y, z) == SOLID) ? 1.0f : -1.0f;
}

static void EnforceBoundaryVelocity(MacGrid* mgrid)
{
	auto x = mgrid->m_dimensions.x;
	auto y = mgrid->m_dimensions.y;
	auto z = mgrid->m_dimensions.z;

	//for every x face
	tbb::parallel_for(tbb::blocked_range<int>(0,x+1),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z; ++k) {
					if (i==0 || i==x) {
						mgrid->m_u_x->SetCell(i,j,k, 0.0f);
					}
					if ( i<x && i>0 && CheckWall(mgrid->m_A, i, j, k)*
						 CheckWall(mgrid->m_A, i-1, j, k) < 0 ) {
						mgrid->m_u_x->SetCell(i,j,k, 0.0f);
					}
				}
			}
		}
	});

	//for every y face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r) {
		for (auto i=r.begin(); i!=r.end(); ++i)
		{
			for (auto j = 0; j < y+1; ++j) {
				for (auto k = 0; k < z; ++k) {
					if (j==0 || j==y) {
						mgrid->m_u_y->SetCell(i,j,k, 0.0f);
					}
					if ( j<y && j>0 && CheckWall(mgrid->m_A, i, j, k)*
						 CheckWall(mgrid->m_A, i, j-1, k) < 0 ) {
						mgrid->m_u_y->SetCell(i,j,k, 0.0f);
					}
				}
			}
		}
	});

	//for every z face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z+1; ++k) {
					if (k==0 || k==z) {
						mgrid->m_u_z->SetCell(i,j,k, 0.0f);
					}
					if ( k<z && k>0 && CheckWall(mgrid->m_A, i, j, k)*
						 CheckWall(mgrid->m_A, i, j, k-1) < 0 ) {
						mgrid->m_u_z->SetCell(i,j,k, 0.0f);
					}
				}
			}
		}
	});
}

static auto Interpolate(Grid<float>* q, dls::math::vec3f p, dls::math::vec3f n)
{
	auto x = std::max(0.0f,std::min(n.x,p.x));
	auto y = std::max(0.0f,std::min(n.y,p.y));
	auto z = std::max(0.0f,std::min(n.z,p.z));
	auto i = static_cast<int>(std::min(x,n.x-2));
	auto j = static_cast<int>(std::min(y,n.y-2));
	auto k = static_cast<int>(std::min(z,n.z-2));
	auto i1 = static_cast<float>(i + 1);
	auto j1 = static_cast<float>(j + 1);
	auto k1 = static_cast<float>(k + 1);

	auto term1 = ((i1 - x) * q->GetCell(i,j,k) + (x-static_cast<float>(i))*q->GetCell(i+1,j,k))*(j1 - y);
	auto term2 = ((i1 - x) * q->GetCell(i,j+1,k) + (x-static_cast<float>(i))*q->GetCell(i+1,j+1,k))*(y - static_cast<float>(j));
	auto term3 = ((i1 - x) * q->GetCell(i,j,k+1) + (x-static_cast<float>(i))*q->GetCell(i+1,j,k+1))*(j1 - y);
	auto term4 = ((i1 - x) * q->GetCell(i,j+1,k+1) + (x-static_cast<float>(i))*q->GetCell(i+1,j+1,k+1))*(y - static_cast<float>(j));
	return (k1 - z)*(term1 + term2) + (z - static_cast<float>(k))*(term3 + term4);
}

static dls::math::vec3f InterpolateVelocity(dls::math::vec3f p, MacGrid* mgrid)
{
	auto x = static_cast<float>(mgrid->m_dimensions.x);
	auto y = static_cast<float>(mgrid->m_dimensions.y);
	auto z = static_cast<float>(mgrid->m_dimensions.z);
	auto maxd = std::max(std::max(x,y),z);
	x = maxd;
	y = maxd;
	z = maxd;

	dls::math::vec3f u;
	u.x = Interpolate(mgrid->m_u_x, dls::math::vec3f(x*p.x, y*p.y-0.5f, z*p.z-0.5f),
					  dls::math::vec3f(x+1, y, z));
	u.y = Interpolate(mgrid->m_u_y, dls::math::vec3f(x*p.x-0.5f, y*p.y, z*p.z-0.5f),
					  dls::math::vec3f(x, y+1, z));
	u.z = Interpolate(mgrid->m_u_z, dls::math::vec3f(x*p.x-0.5f, y*p.y-0.5f, z*p.z),
					  dls::math::vec3f(x, y, z+1));
	return u;
}

static void SplatMACGridToParticles(std::vector<Particle*>& particles, MacGrid* mgrid)
{
	auto particleCount = particles.size();
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particleCount),
					  [&](const tbb::blocked_range<size_t>& r) {
		for (auto i=r.begin(); i!=r.end(); ++i) {
			particles[i]->m_u = InterpolateVelocity(particles[i]->m_p, mgrid);
		}
	});
}

static void SplatParticlesToMACGrid(
		ParticleGrid* sgrid,
		std::vector<Particle*>& /*particles*/,
		MacGrid* mgrid)
{
	auto const RE = 1.4f; //sharpen kernel weight
	auto const x = mgrid->m_dimensions.x;
	auto const y = mgrid->m_dimensions.y;
	auto const z = mgrid->m_dimensions.z;
	auto const maxd = static_cast<float>(std::max(std::max(x,y),z));

	tbb::parallel_for(tbb::blocked_range<int>(0,x+1),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y+1; ++j) {
				for (auto k = 0; k < z+1; ++k) {
					std::vector<Particle*> neighbors;
					//Splat X direction
					if (j<y && k<z) {
						auto px = dls::math::vec3f(
									static_cast<float>(i),
									static_cast<float>(j) + 0.5f,
									static_cast<float>(k) + 0.5f);

						auto sumw = 0.0f;
						auto sumx = 0.0f;

						neighbors = sgrid->GetWallNeighbors(
									dls::math::vec3f(static_cast<float>(i), static_cast<float>(j), static_cast<float>(k)),
									dls::math::vec3f(1.0f, 2.0f, 2.0f));

						for (unsigned int n=0; n<neighbors.size(); n++) {
							Particle* p = neighbors[n];
							if (p->m_type == FLUID) {
								dls::math::vec3f pos;
								pos.x = std::max(0.0f,std::min(maxd,maxd*p->m_p.x));
								pos.y = std::max(0.0f,std::min(maxd,maxd*p->m_p.y));
								pos.z = std::max(0.0f,std::min(maxd,maxd*p->m_p.z));
								auto w = p->m_mass * Sharpen(
											Sqrlength(pos,px),RE);
								sumx += w*p->m_u.x;
								sumw += w;
							}
						}
						auto uxsum = 0.0f;
						if (sumw>0) {
							uxsum = sumx/sumw;
						}
						mgrid->m_u_x->SetCell(i,j,k,uxsum);
					}
					neighbors.clear();

					//Splat Y direction
					if (i<x && k<z) {
						auto py = dls::math::vec3f(static_cast<float>(i)+0.5f, static_cast<float>(j), static_cast<float>(k)+0.5f);
						auto sumw = 0.0f;
						auto sumy = 0.0f;
						neighbors = sgrid->GetWallNeighbors(dls::math::vec3f(static_cast<float>(i), static_cast<float>(j), static_cast<float>(k)),
															dls::math::vec3f(2.0f, 1.0f, 2.0f));
						for (unsigned int n=0; n<neighbors.size(); n++) {
							Particle* p = neighbors[n];
							if (p->m_type == FLUID) {
								dls::math::vec3f pos;
								pos.x = std::max(0.0f,std::min(maxd,maxd*p->m_p.x));
								pos.y = std::max(0.0f,std::min(maxd,maxd*p->m_p.y));
								pos.z = std::max(0.0f,std::min(maxd,maxd*p->m_p.z));
								auto w = p->m_mass * Sharpen(
											Sqrlength(pos,py),RE);
								sumy += w*p->m_u.y;
								sumw += w;
							}
						}
						auto uysum = 0.0f;
						if (sumw>0.0f) {
							uysum = sumy/sumw;
						}
						mgrid->m_u_y->SetCell(i,j,k,uysum);
					}
					neighbors.clear();

					//Splat Z direction
					if (i<x && j<y) {
						dls::math::vec3f pz = dls::math::vec3f(static_cast<float>(i)+0.5f, static_cast<float>(j)+0.5f, static_cast<float>(k));
						auto sumw = 0.0f;
						auto sumz = 0.0f;
						neighbors = sgrid->GetWallNeighbors(dls::math::vec3f(static_cast<float>(i),static_cast<float>(j),static_cast<float>(k)),
															dls::math::vec3f(2.0f,2.0f,1.0f));
						for (unsigned int n=0; n<neighbors.size(); n++) {
							Particle* p = neighbors[n];
							if (p->m_type == FLUID) {
								dls::math::vec3f pos;
								pos.x = std::max(0.0f, std::min(maxd, maxd*p->m_p.x));
								pos.y = std::max(0.0f, std::min(maxd, maxd*p->m_p.y));
								pos.z = std::max(0.0f, std::min(maxd, maxd*p->m_p.z));
								auto w = p->m_mass * Sharpen(
											Sqrlength(pos,pz),RE);
								sumz += w*p->m_u.z;
								sumw += w;
							}
						}
						auto uzsum = 0.0f;
						if (sumw>0) {
							uzsum = sumz/sumw;
						}
						mgrid->m_u_z->SetCell(i,j,k,uzsum);
					}
					neighbors.clear();
				}
			}
		}
	});
}

/* simulation */

FlipSim::FlipSim(
		const dls::math::vec3i &maxres,
		const float& density,
		const float& stepsize,
		sceneCore::Scene* s,
		const bool& verbose)
{
	m_dimensions = maxres;
	m_pgrid = new ParticleGrid(maxres);
	m_mgrid = CreateMacgrid(maxres);
	m_mgrid_previous = CreateMacgrid(maxres);
	m_max_density = 0.0f;
	m_density = density;
	m_scene = s;
	m_frame = 0;
	m_stepsize = stepsize;
	m_subcell = 1;
	m_picflipratio = .95f;
	m_densitythreshold = 0.04f;
	m_verbose = verbose;
}

FlipSim::~FlipSim()
{
	delete m_pgrid;

	for (auto particule : m_particles) {
		delete particule;
	}

	m_particles.clear();
	ClearMacgrid(m_mgrid);
}

void FlipSim::Init()
{
	m_scene->BuildPermaSolidGeomLevelSet();
	//We need to figure out maximum particle pressure,
	//so we generate a bunch of temporary particles
	//inside of a known area, sort them back onto the underlying grid, and calculate the density
	auto maxd = std::max(std::max(m_dimensions.x, m_dimensions.z), m_dimensions.y);
	auto h = m_density / static_cast<float>(maxd);
	//generate temp particles
	for (unsigned int i = 0; i < 10; i++) {               //FOR_EACH_CELL
		for (unsigned int j = 0; j < 10; j++) {
			for (unsigned int k = 0; k < 10; k++) {
				Particle* p = new Particle;
				p->m_p = (dls::math::vec3f(static_cast<float>(i), static_cast<float>(j), static_cast<float>(k)) + dls::math::vec3f(0.5f))*h;
				p->m_type = FLUID;
				p->m_mass = 1.0f;
				m_particles.push_back(p);
			}
		}
	}
	m_pgrid->Sort(m_particles);
	m_max_density = 1.0f;
	ComputeDensity();
	m_max_density = 0.0f;
	//sum densities across particles
	for (unsigned int n=0; n<m_particles.size(); n++) {
		Particle *p = m_particles[n];
		m_max_density = std::max(m_max_density,p->m_density);
		delete p;
	}
	m_particles.clear();

	//Generate particles and sort
	m_scene->GenerateParticles(m_particles, m_dimensions, m_density, m_pgrid, 0);
	m_pgrid->Sort(m_particles);
	m_pgrid->MarkCellTypes(m_particles, m_mgrid.m_A, m_density);
}

void FlipSim::StoreTempParticleVelocities()
{
	auto particlecount = m_particles.size();

	tbb::parallel_for(tbb::blocked_range<size_t>(0,particlecount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto p=r.begin(); p!=r.end(); ++p) {
			m_particles[p]->m_pt = m_particles[p]->m_p;
			m_particles[p]->m_ut = m_particles[p]->m_u;
		}
	});
}

void FlipSim::Step(bool saveVDB, bool saveOBJ, bool savePARTIO)
{
	m_frame++;
	std::cout << "Simulating Step: " << m_frame << "..." << std::endl;

	auto maxd = static_cast<float>(std::max(std::max(m_dimensions.x, m_dimensions.z), m_dimensions.y));

	m_scene->GenerateParticles(m_particles, m_dimensions, m_density, m_pgrid, m_frame);
	m_scene->BuildSolidGeomLevelSet(m_frame);

	AdjustParticlesStuckInSolids();

	StoreTempParticleVelocities();
	m_pgrid->Sort(m_particles);
	ComputeDensity();
	ApplyExternalForces();
	SplatParticlesToMACGrid(m_pgrid, m_particles, &m_mgrid);
	m_pgrid->MarkCellTypes(m_particles, m_mgrid.m_A, m_density);
	StorePreviousGrid();
	EnforceBoundaryVelocity(&m_mgrid);
	Project();
	EnforceBoundaryVelocity(&m_mgrid);
	ExtrapolateVelocity();
	SubtractPreviousGrid();
	SolvePicFlip();
	AdvectParticles();

	CheckParticleSolidConstraints();
	StoreTempParticleVelocities();
	auto h = m_density/maxd;
	ResampleParticles(m_pgrid, m_particles, m_scene, static_cast<float>(m_frame), m_stepsize, h, m_dimensions);

	CheckParticleSolidConstraints();

	if (saveVDB || saveOBJ || savePARTIO) {
		m_scene->ExportParticles(m_particles, maxd, m_frame, saveVDB, saveOBJ, savePARTIO);
	}
}

void FlipSim::AdjustParticlesStuckInSolids()
{
	auto maxd = static_cast<float>(std::max(std::max(m_dimensions.x, m_dimensions.z), m_dimensions.y));
	auto particleCount = m_particles.size();
	//pushi_back to vectors doesn't play nice with lambdas for some reason, so we have to
	//do something a little bit convoluted here...
	bool* particleInSolidChecks = new bool[particleCount];
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particleCount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto p=r.begin(); p!=r.end(); ++p) {
			particleInSolidChecks[p] = false;
			if (m_particles[p]->m_type==FLUID) {
				m_particles[p]->m_temp = false;
				m_particles[p]->m_temp2 = false;
				dls::math::vec3f point = m_particles[p]->m_p * maxd;
				unsigned int id;
				if (m_scene->CheckPointInsideSolidGeom(point, static_cast<float>(m_frame), id)==true) {
					particleInSolidChecks[p] = true;
				}
			}
		}
	});

	//build vector of particles we need to adjust
	std::vector<Particle*> stuckParticles;
	stuckParticles.reserve(particleCount);
	for (unsigned int p=0; p<particleCount; p++) {
		if (particleInSolidChecks[p]==true) {
			stuckParticles.push_back(m_particles[p]);
			m_particles[p]->m_pt = m_particles[p]->m_p;
		}
	}
	delete [] particleInSolidChecks;
	//figure out direction to nearest surface from levelset, then raycast for a precise result
	m_scene->GetSolidLevelSet()->ProjectPointsToSurface(stuckParticles, maxd);
	auto stuckCount = static_cast<unsigned int>(stuckParticles.size());
	for (auto p=0u; p<stuckCount; p++) {
		rayCore::Ray r;
		r.m_origin = stuckParticles[p]->m_pt * maxd;
		r.m_frame = static_cast<float>(m_frame);
		r.m_direction = normalise(stuckParticles[p]->m_p -
								  stuckParticles[p]->m_pt);
		auto d = longueur(stuckParticles[p]->m_p - stuckParticles[p]->m_pt);
		auto raynulltest = longueur(r.m_direction);
		if (raynulltest==raynulltest) {
			rayCore::Intersection hit = m_scene->IntersectSolidGeoms(r);
			auto nearestDistance = longueur(r.m_origin - hit.m_point);
			stuckParticles[p]->m_p = (r.m_origin + r.m_direction * 1.05f * nearestDistance)/maxd;
			stuckParticles[p]->m_u = normalise(r.m_direction) * d;
		}
	}
	stuckParticles.clear();
}

void FlipSim::CheckParticleSolidConstraints()
{
	auto maxd = static_cast<float>(std::max(std::max(m_dimensions.x, m_dimensions.z), m_dimensions.y));
	auto particlecount = m_particles.size();
	// for (unsigned int p=0; p<particlecount; p++) {
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particlecount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto p = r.begin(); p!=r.end(); ++p) {
			if (m_particles[p]->m_type==FLUID) {
				rayCore::Ray rayon;
				rayon.m_origin = m_particles[p]->m_pt * maxd;
				rayon.m_frame = static_cast<float>(m_frame);
				rayon.m_direction = normalise(m_particles[p]->m_p -
										  m_particles[p]->m_pt);
			//	auto d = longueur(m_particles[p]->m_p - m_particles[p]->m_pt);
				auto raynulltest = longueur(rayon.m_direction);

				if (raynulltest==raynulltest) {
					rayCore::Intersection hit = m_scene->IntersectSolidGeoms(rayon);
					auto u_dir = longueur(m_particles[p]->m_ut);
					if (hit.m_hit==true) {
						auto solidDistance = longueur(rayon.m_origin - hit.m_point);
						auto velocityDistance = longueur(m_particles[p]->m_p - m_particles[p]->m_pt) * maxd;

						if (solidDistance<velocityDistance) {
							m_particles[p]->m_p = (rayon.m_origin +
												   rayon.m_direction * .90f * solidDistance)/maxd;
							m_particles[p]->m_u = 2.0f*produit_scalaire(rayon.m_direction, hit.m_normal)*
									hit.m_normal-normalise(rayon.m_direction);
							m_particles[p]->m_u = normalise(m_particles[p]->m_u) * u_dir;
						}
					}

					rayon.m_origin = m_particles[p]->m_p * maxd;
					unsigned int id;

					if (m_scene->CheckPointInsideSolidGeom(rayon.m_origin, static_cast<float>(m_frame), id)==true) {
						m_particles[p]->m_u = -normalise(rayon.m_direction) * u_dir;
						m_particles[p]->m_p = m_particles[p]->m_pt +
								m_particles[p]->m_u * m_stepsize;
					}
				}
			}
		}
	});
}

void FlipSim::AdvectParticles()
{
	auto x = m_dimensions.x;
	auto y = m_dimensions.y;
	auto z = m_dimensions.z;
	auto maxd = static_cast<float>(std::max(std::max(x, z), y));
	auto particleCount = m_particles.size();

	//update positions
	tbb::parallel_for(tbb::blocked_range<size_t>(0, particleCount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			if (m_particles[i]->m_type == FLUID) {
				dls::math::vec3f velocity = InterpolateVelocity(m_particles[i]->m_p, &m_mgrid);
				m_particles[i]->m_p += m_stepsize*velocity;
				//m_particles[i]->m_u = velocity;
			}
		}
	});

	m_pgrid->Sort(m_particles); //sort

	//apply constraints for outer walls of sim
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particleCount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto p0=r.begin(); p0!=r.end(); ++p0) {
			auto rayon = 1.0f/maxd;
			if ( m_particles[p0]->m_type == FLUID ) {
				m_particles[p0]->m_p = std::max(
							dls::math::vec3f(rayon),
							std::min(dls::math::vec3f(1.0f - rayon),
									 m_particles[p0]->m_p));
			}

			Particle* p = m_particles[p0];
			if (p->m_type == FLUID) {
				auto i = std::min(static_cast<float>(x)-1.0f,p->m_p.x * maxd);
				auto j = std::min(static_cast<float>(y)-1.0f,p->m_p.y * maxd);
				auto k = std::min(static_cast<float>(z)-1.0f,p->m_p.z * maxd);
				auto neighbors = m_pgrid->GetCellNeighbors(
							dls::math::vec3f(i,j,k),
							dls::math::vec3f(1.0f));

				for (auto p1 = 0ul; p1 < neighbors.size(); p1++) {
					Particle* np = neighbors[p1];
					auto re = 1.5f*m_density/maxd;
					if (np->m_type == SOLID) {
						auto dist = longueur(p->m_p-np->m_p); //check this later
						if (dist<re) {
							dls::math::vec3f normal = np->m_n;
							if (longueur(normal)<0.0000001f && dist != 0.0f) {
								normal = normalise(p->m_p - np->m_p);
							}
							p->m_p += (re-dist)*normal;
							p->m_u -= produit_scalaire(p->m_u, normal) * normal;
						}
					}
				}
			}
		}
	});
}

void FlipSim::SolvePicFlip()
{
	auto particleCount = m_particles.size();

	//store copy of current velocities for later
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particleCount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			m_particles[i]->m_t = m_particles[i]->m_u;
		}
	});

	SplatMACGridToParticles(m_particles, &m_mgrid_previous);

	//set FLIP velocity
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particleCount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			m_particles[i]->m_t = m_particles[i]->m_u + m_particles[i]->m_t;
		}
	});

	//set PIC velocity
	SplatMACGridToParticles(m_particles, &m_mgrid);

	//combine PIC and FLIP
	tbb::parallel_for(tbb::blocked_range<size_t>(0,particleCount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			m_particles[i]->m_u = (1.0f-m_picflipratio)*m_particles[i]->m_u +
					m_picflipratio*m_particles[i]->m_t;
		}
	});
}

void FlipSim::StorePreviousGrid()
{
	auto x = m_dimensions.x;
	auto y = m_dimensions.y;
	auto z = m_dimensions.z;

	//for every x face
	tbb::parallel_for(tbb::blocked_range<int>(0,x+1),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z; ++k) {
					m_mgrid_previous.m_u_x->SetCell(i,j,k,m_mgrid.m_u_x->GetCell(i,j,k));
				}
			}
		}
	});

	//for every y face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y+1; ++j) {
				for (auto k = 0; k < z; ++k) {
					m_mgrid_previous.m_u_y->SetCell(i,j,k,m_mgrid.m_u_y->GetCell(i,j,k));
				}
			}
		}
	});

	//for every z face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z+1; ++k) {
					m_mgrid_previous.m_u_z->SetCell(i,j,k,m_mgrid.m_u_z->GetCell(i,j,k));
				}
			}
		}
	});
}

void FlipSim::SubtractPreviousGrid()
{
	auto x = m_dimensions.x;
	auto y = m_dimensions.y;
	auto z = m_dimensions.z;

	//for every x face
	tbb::parallel_for(tbb::blocked_range<int>(0,x+1),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z; ++k) {
					auto subx = m_mgrid.m_u_x->GetCell(i,j,k) -
							m_mgrid_previous.m_u_x->GetCell(i,j,k);
					m_mgrid_previous.m_u_x->SetCell(i,j,k,subx);
				}
			}
		}
	});

	//for every y face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y+1; ++j) {
				for (auto k = 0; k < z; ++k) {
					auto suby = m_mgrid.m_u_y->GetCell(i,j,k) -
							m_mgrid_previous.m_u_y->GetCell(i,j,k);
					m_mgrid_previous.m_u_y->SetCell(i,j,k,suby);
				}
			}
		}
	});

	//for every z face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z+1; ++k) {
					auto subz = m_mgrid.m_u_z->GetCell(i,j,k) -
							m_mgrid_previous.m_u_z->GetCell(i,j,k);
					m_mgrid_previous.m_u_z->SetCell(i,j,k,subz);
				}
			}
		}
	});
}

void FlipSim::Project()
{
	auto x = m_dimensions.x;
	auto y = m_dimensions.y;
	auto z = m_dimensions.z;

	auto maxd = std::max(std::max(m_dimensions.x, m_dimensions.z), m_dimensions.y);
	auto h = 1.0f/static_cast<float>(maxd); //cell width

	//compute divergence per cell
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z; ++k) {
					auto divergence = (m_mgrid.m_u_x->GetCell(i+1, j, k) -
									   m_mgrid.m_u_x->GetCell(i, j, k) +
									   m_mgrid.m_u_y->GetCell(i, j+1, k) -
									   m_mgrid.m_u_y->GetCell(i, j, k) +
									   m_mgrid.m_u_z->GetCell(i, j, k+1) -
									   m_mgrid.m_u_z->GetCell(i, j, k)
									   ) / h;
					m_mgrid.m_D->SetCell(i,j,k,divergence);
				}
			}
		}
	});

	//compute internal level set for liquid surface
	m_pgrid->BuildSDF(m_mgrid, m_density);

	Solve(m_mgrid, m_subcell, m_verbose);

	if (m_verbose) {
		std::cout << " " << std::endl;//TODO: no more stupid formatting hacks like this to std::out
	}

	//subtract pressure gradient
	SubtractPressureGradient();
}

void FlipSim::ExtrapolateVelocity()
{
	auto x = m_dimensions.x;
	auto y = m_dimensions.y;
	auto z = m_dimensions.z;

	Grid<int>** mark = new Grid<int>*[3];
	Grid<int>** wallmark = new Grid<int>*[3];
	for (unsigned int i=0; i<3; i++) {
		mark[i] = new Grid<int>(m_dimensions, 0);
		wallmark[i] = new Grid<int>(m_dimensions, 0);
	}

	//initalize temp grids with values
	//for every x face
	tbb::parallel_for(tbb::blocked_range<int>(0,x+1),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z; ++k) {
					mark[0]->SetCell(i,j,k, (i>0 && m_mgrid.m_A->GetCell(i-1,j,k)==FLUID) ||
									 (i<x && m_mgrid.m_A->GetCell(i,j,k)==FLUID));
					wallmark[0]->SetCell(i,j,k,(i<=0 || m_mgrid.m_A->GetCell(i-1,j,k)==SOLID) &&
										 (i>=x || m_mgrid.m_A->GetCell(i,j,k)==SOLID));
				}
			}
		}
	});

	//for every y face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y+1; ++j) {
				for (auto k = 0; k < z; ++k) {
					mark[1]->SetCell(i,j,k, (j>0 && m_mgrid.m_A->GetCell(i,j-1,k)==FLUID) ||
									 (j<y && m_mgrid.m_A->GetCell(i,j,k)==FLUID));
					wallmark[1]->SetCell(i,j,k,(j<=0 || m_mgrid.m_A->GetCell(i,j-1,k)==SOLID) &&
										 (j>=y || m_mgrid.m_A->GetCell(i,j,k)==SOLID));
				}
			}
		}
	});

	//for every z face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z+1; ++k) {
					mark[2]->SetCell(i,j,k, (k>0 && m_mgrid.m_A->GetCell(i,j,k-1)==FLUID) ||
									 (k<z && m_mgrid.m_A->GetCell(i,j,k)==FLUID));
					wallmark[2]->SetCell(i,j,k,(k<=0 || m_mgrid.m_A->GetCell(i,j,k-1)==SOLID) &&
										 (k>=z || m_mgrid.m_A->GetCell(i,j,k)==SOLID));
				}
			}
		}
	});

	//extrapolate
	tbb::parallel_for(tbb::blocked_range<int>(0,x+1),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y+1; ++j) {
				for (auto k = 0; k < z+1; ++k) {
					for (auto n=0; n<3; ++n) {
						if (n!=0 && i>x-1) {
							continue;
						};

						if (n!=1 && j>y-1) {
							continue;
						};

						if (n!=2 && k>z-1) {
							continue;
						};

						if (!mark[n]->GetCell(i,j,k) && wallmark[n]->GetCell(i,j,k)) {
							unsigned int wsum = 0;
							auto sum = 0.0f;
							dls::math::vec3f q[6] = {
								dls::math::vec3f(static_cast<float>(i - 1), static_cast<float>(j), static_cast<float>(k)),
								dls::math::vec3f(static_cast<float>(i + 1), static_cast<float>(j), static_cast<float>(k)),
								dls::math::vec3f(static_cast<float>(i), static_cast<float>(j - 1), static_cast<float>(k)),
								dls::math::vec3f(static_cast<float>(i), static_cast<float>(j + 1), static_cast<float>(k)),
								dls::math::vec3f(static_cast<float>(i), static_cast<float>(j), static_cast<float>(k - 1)),
								dls::math::vec3f(static_cast<float>(i), static_cast<float>(j), static_cast<float>(k + 1))
							};

							for (unsigned int qk=0; qk<6; ++qk) {
								auto ok = q[qk][0] >= 0.0f;
								ok &= q[qk][0] < static_cast<float>(x + (n == 0));
								ok &= q[qk][1] >= 0.0f;
								ok &= q[qk][1] < static_cast<float>(y + (n == 1));
								ok &= q[qk][2] >= 0.0f;
								ok &= q[qk][2] < static_cast<float>(z + (n == 2));

								if (ok) {
									if (mark[n]->GetCell(q[qk])) {
										wsum ++;
										if (n==0) {
											sum += m_mgrid.m_u_x->GetCell(q[qk]);
										}
										else if (n==1) {
											sum += m_mgrid.m_u_y->GetCell(q[qk]);
										}
										else if (n==2) {
											sum += m_mgrid.m_u_z->GetCell(q[qk]);
										}
									}
								}
							}

							if (wsum) {
								if (n==0) {
									m_mgrid.m_u_x->SetCell(i,j,k, sum / static_cast<float>(wsum));
								}
								else if (n==1) {
									m_mgrid.m_u_y->SetCell(i,j,k, sum / static_cast<float>(wsum));
								}
								else if (n==2) {
									m_mgrid.m_u_z->SetCell(i,j,k, sum / static_cast<float>(wsum));
								}
							}
						}
					}
				}
			}
		}
	});

	for (unsigned int i=0; i<3; i++) {
		delete mark[i];
		delete wallmark[i];
	}

	delete [] mark;
	delete [] wallmark;
}

void FlipSim::SubtractPressureGradient()
{
	auto x = m_dimensions.x;
	auto y = m_dimensions.y;
	auto z = m_dimensions.z;

	auto maxd = std::max(std::max(m_dimensions.x, m_dimensions.z), m_dimensions.y);
	auto h = 1.0f/static_cast<float>(maxd); //cell width

	//for every x face
	tbb::parallel_for(tbb::blocked_range<int>(0,x+1),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z; ++k) {
					if (i>0 && i<x) {
						auto pf = m_mgrid.m_P->GetCell(i,j,k);
						auto pb = m_mgrid.m_P->GetCell(i-1,j,k);
						if (m_subcell && m_mgrid.m_L->GetCell(i,j,k) *
								m_mgrid.m_L->GetCell(i-1,j,k) < 0.0f) {
							if (m_mgrid.m_L->GetCell(i,j,k)<0.0f) {
								pf = m_mgrid.m_P->GetCell(i,j,k);
							}else {
								pf = m_mgrid.m_L->GetCell(i,j,k)/
										std::min(1.0e-3f,m_mgrid.m_L->GetCell(i-1,j,k))*
										m_mgrid.m_P->GetCell(i-1,j,k);
							}
							if (m_mgrid.m_L->GetCell(i-1,j,k)<0.0f) {
								pb = m_mgrid.m_P->GetCell(i-1,j,k);
							}else {
								pb = m_mgrid.m_L->GetCell(i-1,j,k)/
										std::min(1.0e-6f,m_mgrid.m_L->GetCell(i,j,k))*
										m_mgrid.m_P->GetCell(i,j,k);
							}
						}
						auto xval = m_mgrid.m_u_x->GetCell(i,j,k);
						xval -= (pf-pb)/h;
						m_mgrid.m_u_x->SetCell(i,j,k,xval);
					}
				}
			}
		}
	});

	//for every y face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y+1; ++j) {
				for (auto k = 0; k < z; ++k) {
					if (j>0 && j<y) {
						auto pf = m_mgrid.m_P->GetCell(i,j,k);
						auto pb = m_mgrid.m_P->GetCell(i,j-1,k);
						if (m_subcell && m_mgrid.m_L->GetCell(i,j,k) *
								m_mgrid.m_L->GetCell(i,j-1,k) < 0.0f) {
							if (m_mgrid.m_L->GetCell(i,j,k)<0.0f) {
								pf = m_mgrid.m_P->GetCell(i,j,k);
							}else {
								pf = m_mgrid.m_L->GetCell(i,j,k)/
										std::min(1.0e-3f,m_mgrid.m_L->GetCell(i,j-1,k))*
										m_mgrid.m_P->GetCell(i,j-1,k);
							}
							if (m_mgrid.m_L->GetCell(i,j-1,k)<0.0f) {
								pb = m_mgrid.m_P->GetCell(i,j-1,k);
							}else {
								pb = m_mgrid.m_L->GetCell(i,j-1,k)/
										std::min(1.0e-6f,m_mgrid.m_L->GetCell(i,j,k))*
										m_mgrid.m_P->GetCell(i,j,k);
							}
						}
						auto yval = m_mgrid.m_u_y->GetCell(i,j,k);
						yval -= (pf-pb)/h;
						m_mgrid.m_u_y->SetCell(i,j,k,yval);
					}
				}
			}
		}
	});

	//for every z face
	tbb::parallel_for(tbb::blocked_range<int>(0,x),
					  [&](const tbb::blocked_range<int>& r) {
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j = 0; j < y; ++j) {
				for (auto k = 0; k < z+1; ++k) {
					if (k>0 && k<z) {
						auto pf = m_mgrid.m_P->GetCell(i,j,k);
						auto pb = m_mgrid.m_P->GetCell(i,j,k-1);
						if (m_subcell && m_mgrid.m_L->GetCell(i,j,k) *
								m_mgrid.m_L->GetCell(i,j,k-1) < 0.0f) {
							if (m_mgrid.m_L->GetCell(i,j,k)<0.0f) {
								pf = m_mgrid.m_P->GetCell(i,j,k);
							}else {
								pf = m_mgrid.m_L->GetCell(i,j,k)/
										std::min(1.0e-3f,m_mgrid.m_L->GetCell(i,j,k-1))*
										m_mgrid.m_P->GetCell(i,j,k-1);
							}
							if (m_mgrid.m_L->GetCell(i,j,k-1)<0.0f) {
								pb = m_mgrid.m_P->GetCell(i,j,k-1);
							}else {
								pb = m_mgrid.m_L->GetCell(i,j,k-1)/
										std::min(1.0e-6f,m_mgrid.m_L->GetCell(i,j,k))*
										m_mgrid.m_P->GetCell(i,j,k);
							}
						}
						auto zval = m_mgrid.m_u_z->GetCell(i,j,k);
						zval -= (pf-pb)/h;
						m_mgrid.m_u_z->SetCell(i,j,k,zval);
					}
				}
			}
		}
	});
}

void FlipSim::ApplyExternalForces()
{
	auto externalForces = m_scene->GetExternalForces();
	auto numberOfExternalForces = externalForces.size();
	auto particlecount = m_particles.size();

	tbb::parallel_for(tbb::blocked_range<size_t>(0,particlecount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			for (auto j=0ul; j<numberOfExternalForces; j++) {
				m_particles[i]->m_u += externalForces[j]*m_stepsize;
			}
		}
	});
}

void FlipSim::ComputeDensity()
{
	auto maxd = static_cast<float>(std::max(std::max(m_dimensions.x, m_dimensions.z), m_dimensions.y));

	auto particlecount = m_particles.size();

	tbb::parallel_for(tbb::blocked_range<size_t>(0,particlecount),
					  [&](const tbb::blocked_range<size_t>& r)
	{
		for (auto i=r.begin(); i!=r.end(); ++i) {
			//Find neighbours
			if (m_particles[i]->m_type==SOLID) {
				m_particles[i]->m_density = 1.0f;
				continue;
			}

			dls::math::vec3f position = m_particles[i]->m_p;
			position.x = std::max(0.0f,std::min(maxd-1.0f, maxd*position.x));
			position.y = std::max(0.0f,std::min(maxd-1.0f, maxd*position.y));
			position.z = std::max(0.0f,std::min(maxd-1.0f, maxd*position.z));

			std::vector<Particle *> neighbors;
			neighbors = m_pgrid->GetCellNeighbors(position, dls::math::vec3f(1));

			auto weightsum = 0.0f;
			for (auto &voisin : neighbors) {
				// if (voisin->m_type!=SOLID) {
				auto sqd = Sqrlength(voisin->m_p, m_particles[i]->m_p);
				//TODO: figure out a better density smooth approx than density/maxd
				auto weight = voisin->m_mass * Smooth(sqd, 4.0f * m_density / maxd);
				weightsum = weightsum + weight;
				// }
			}

			m_particles[i]->m_density = weightsum / m_max_density;
		}
	});
}

bool FlipSim::IsCellFluid(const int& x, const int& y, const int& z)
{
	return (m_scene->GetLiquidLevelSet()->GetCell(x,y,z)<0.0f);
}

std::vector<Particle*>* FlipSim::GetParticles()
{
	return &m_particles;
}

dls::math::vec3i FlipSim::GetDimensions()
{
	return m_dimensions;
}

sceneCore::Scene* FlipSim::GetScene()
{
	return m_scene;
}

FlipTask::FlipTask(FlipSim* sim, bool dumpVDB, bool dumpOBJ, bool dumpPARTIO)
{
	m_sim = sim;
	m_dumpPARTIO = dumpPARTIO;
	m_dumpOBJ = dumpOBJ;
	m_dumpVDB = dumpVDB;
}

tbb::task* FlipTask::execute()
{
	m_sim->Step(m_dumpVDB, m_dumpOBJ, m_dumpPARTIO);
	return nullptr;
}

}
