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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "imcompressibilite.h"

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

	void initialise(const dls::math::vec3i &res)
	{
		M.initialise(res.x, res.y, res.z);

		Adiag.initialise(res.x, res.y, res.z);

		Aplusi.initialise(res.x, res.y, res.z);
		Aplusj.initialise(res.x, res.y, res.z);
		Aplusk.initialise(res.x, res.y, res.z);

		p.initialise(res.x, res.y, res.z);
		r.initialise(res.x, res.y, res.z);
		z.initialise(res.x, res.y, res.z);
		s.initialise(res.x, res.y, res.z);
	}
};

float calcul_divergence(
		const Grille<float> &grille_x,
		const Grille<float> &grille_y,
		const Grille<float> &grille_z,
		Grille<float> &d)
{
	const auto res = d.resolution();
	float max_divergence = 0.0f;

	for (int z = 0; z < res.z; ++z) {
		for (int y = 0; y < res.y; ++y) {
			for (int x = 0; x < res.x; ++x) {
				const auto x0 = grille_x.valeur(x - 1, y, z);
				const auto x1 = grille_x.valeur(x + 1, y, z);
				const auto y0 = grille_y.valeur(x, y - 1, z);
				const auto y1 = grille_y.valeur(x, y + 1, z);
				const auto z0 = grille_z.valeur(x, y, z - 1);
				const auto z1 = grille_z.valeur(x, y, z + 1);

				const auto divergence = (x1 - x0) + (y1 - y0) + (z1 - z0);

				max_divergence = std::max(max_divergence, divergence);

				d.valeur(x, y, z, divergence);
			}
		}
	}

	return max_divergence;
}

float calcul_divergence(
		const Grille<dls::math::vec3f> &grille,
		Grille<float> &d)
{
	const auto res = d.resolution();
	float max_divergence = 0.0f;

	for (int z = 0; z < res.z; ++z) {
		for (int y = 0; y < res.y; ++y) {
			for (int x = 0; x < res.x; ++x) {
				const auto x0 = grille.valeur(x - 1, y, z);
				const auto x1 = grille.valeur(x + 1, y, z);
				const auto y0 = grille.valeur(x, y - 1, z);
				const auto y1 = grille.valeur(x, y + 1, z);
				const auto z0 = grille.valeur(x, y, z - 1);
				const auto z1 = grille.valeur(x, y, z + 1);

				const auto divergence = (x1.x - x0.x) + (y1.y - y0.y) + (z1.z - z0.z);

				max_divergence = std::max(max_divergence, divergence);

				d.valeur(x, y, z, divergence);
			}
		}
	}

	return max_divergence;
}

void construit_preconditionneur(
		PCGSolver &pcg_solver,
		const Grille<char> &drapeaux)
{
	const auto res = drapeaux.resolution();

	constexpr auto T = 0.97f;

	for (int z = 0; z < res.z; ++z) {
		for (int y = 0; y < res.y; ++y) {
			for (int x = 0; x < res.x; ++x) {
				if (drapeaux.valeur(x, y, z) == 0) {
					pcg_solver.M.valeur(x, y, z, 0.0f);
					continue;
				}

				const auto pi = pcg_solver.M.valeur(x - 1, y, z);
				const auto pj = pcg_solver.M.valeur(x, y - 1, z);
				const auto pk = pcg_solver.M.valeur(x, y, z - 1);

				const auto Ad = pcg_solver.Adiag.valeur(x, y, z);

				const auto Aii = pcg_solver.Aplusi.valeur(x - 1, y, z);
				const auto Aij = pcg_solver.Aplusi.valeur(x, y - 1, z);
				const auto Aik = pcg_solver.Aplusi.valeur(x, y, z - 1);

				const auto Aji = pcg_solver.Aplusj.valeur(x - 1, y, z);
				const auto Ajj = pcg_solver.Aplusj.valeur(x, y - 1, z);
				const auto Ajk = pcg_solver.Aplusj.valeur(x, y, z - 1);

				const auto Aki = pcg_solver.Aplusk.valeur(x - 1, y, z);
				const auto Akj = pcg_solver.Aplusk.valeur(x, y - 1, z);
				const auto Akk = pcg_solver.Aplusk.valeur(x, y, z - 1);

				const auto e = Ad
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
		const Grille<char> &drapeaux)
{
	const auto res = drapeaux.resolution();

	Grille<float> q;
	q.initialise(res.x, res.y, res.z);

	/* Résoud Lq = r */
	for (int z = 0; z < res.z; ++z) {
		for (int y = 0; y < res.y; ++y) {
			for (int x = 0; x < res.x; ++x) {
				if (drapeaux.valeur(x, y, z) == 0) {
					q.valeur(x, y, z, 0.0f);
					continue;
				}

				const auto r_ijk = pcg_solver.r.valeur(x, y, z);
				const auto M_ijk = pcg_solver.M.valeur(x, y, z);

				const auto A_i0jk = pcg_solver.Aplusi.valeur(x - 1, y, z);
				const auto A_ij0k = pcg_solver.Aplusj.valeur(x, y - 1, z);
				const auto A_ijk0 = pcg_solver.Aplusk.valeur(x, y, z - 1);

				const auto M_i0jk = pcg_solver.M.valeur(x - 1, y, z);
				const auto M_ij0k = pcg_solver.M.valeur(x, y - 1, z);
				const auto M_ijk0 = pcg_solver.M.valeur(x, y, z - 1);

				const auto q_i0jk = q.valeur(x - 1, y, z);
				const auto q_ij0k = q.valeur(x, y - 1, z);
				const auto q_ijk0 = q.valeur(x, y, z - 1);

				const auto t = r_ijk
							   - A_i0jk*M_i0jk*q_i0jk
							   - A_ij0k*M_ij0k*q_ij0k
							   - A_ijk0*M_ijk0*q_ijk0;


				q.valeur(x, y, z, t * M_ijk);
			}
		}
	}

	/* Résoud L^Tz = q */
	for (int z = res.z - 1; z >= 0; --z) {
		for (int y = res.y - 1; y >= 0; --y) {
			for (int x = res.x - 1; x >= 0; --x) {
				if (drapeaux.valeur(x, y, z) == 0) {
					q.valeur(x, y, z, 0.0f);
					continue;
				}

				const auto q_ijk = q.valeur(x, y, z);
				const auto M_ijk = pcg_solver.M.valeur(x, y, z);

				const auto Ai_ijk = pcg_solver.Aplusi.valeur(x, y, z);
				const auto Aj_ijk = pcg_solver.Aplusj.valeur(x, y, z);
				const auto Ak_ijk = pcg_solver.Aplusk.valeur(x, y, z);

				const auto z_i1jk = pcg_solver.z.valeur(x + 1, y, z);
				const auto z_ij1k = pcg_solver.z.valeur(x, y + 1, z);
				const auto z_ijk1 = pcg_solver.z.valeur(x, y, z + 1);

				const auto t = q_ijk
							   - Ai_ijk * M_ijk * z_i1jk
							   - Aj_ijk * M_ijk * z_ij1k
							   - Ak_ijk * M_ijk * z_ijk1;


				q.valeur(x, y, z, t * M_ijk);
			}
		}
	}
}

float produit_scalaire(const Grille<float> &a, const Grille<float> &b)
{
	const auto res_x = a.resolution().x;
	const auto res_y = a.resolution().y;
	const auto res_z = a.resolution().z;

	auto valeur = 0.0f;

	for (int z = 0; z < res_z; ++z) {
		for (int y = 0; y < res_y; ++y) {
			for (int x = 0; x < res_x; ++x) {
				valeur += a.valeur(x, y, z) * b.valeur(x, y, z);
			}
		}
	}

	return valeur;
}

float maximum(const Grille<float> &a)
{
	const auto res_x = a.resolution().x;
	const auto res_y = a.resolution().y;
	const auto res_z = a.resolution().z;

	auto max = std::numeric_limits<float>::min();

	for (int x = 0; x < res_x; ++x) {
		for (int y = 0; y < res_y; ++y) {
			for (int z = 0; z < res_z; ++z) {
				const auto v = std::abs(a.valeur(x, y, z));
				if (v > max) {
					max = v;
				}
			}
		}
	}

	return max;
}

void ajourne_pression_residus(const float alpha, Grille<float> &p, Grille<float> &r, const Grille<float> &a, const Grille<float> &b)
{
	const auto res_x = a.resolution().x;
	const auto res_y = a.resolution().y;
	const auto res_z = a.resolution().z;

	for (int x = 0; x < res_x; ++x) {
		for (int y = 0; y < res_y; ++y) {
			for (int z = 0; z < res_z; ++z) {
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

void ajourne_vecteur_recherche(Grille<float> &s, const Grille<float> &a, const float beta)
{
	const auto res_x = s.resolution().x;
	const auto res_y = s.resolution().y;
	const auto res_z = s.resolution().z;

	for (int x = 0; x < res_x; ++x) {
		for (int y = 0; y < res_y; ++y) {
			for (int z = 0; z < res_z; ++z) {
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
	const auto res = pcg_solver.M.resolution();

	for (int z = 0; z < res.z; ++z) {
		for (int y = 0; y < res.y; ++y) {
			for (int x = 0; x < res.x; ++x) {
				const auto coef = pcg_solver.Adiag.valeur(x, y, z);

				const auto s_i0jk = pcg_solver.s.valeur(x - 1, y, z);
				const auto s_i1jk = pcg_solver.s.valeur(x + 1, y, z);
				const auto s_ij0k = pcg_solver.s.valeur(x, y - 1, z);
				const auto s_ij1k = pcg_solver.s.valeur(x, y + 1, z);
				const auto s_ijk0 = pcg_solver.s.valeur(x, y, z - 1);
				const auto s_ijk1 = pcg_solver.s.valeur(x, y, z + 1);

				const auto A_i0jk = pcg_solver.Aplusi.valeur(x - 1, y, z);
				const auto A_i1jk = pcg_solver.Aplusi.valeur(x, y, z);
				const auto A_ij0k = pcg_solver.Aplusj.valeur(x, y - 1, z);
				const auto A_ij1k = pcg_solver.Aplusj.valeur(x, y, z);
				const auto A_ijk0 = pcg_solver.Aplusk.valeur(x, y, z - 1);
				const auto A_ijk1 = pcg_solver.Aplusk.valeur(x, y, z);

				const auto v = coef * (s_i0jk*A_i0jk+s_i1jk*A_i1jk+s_ij0k*A_ij0k+s_ij1k*A_ij1k+s_ijk0*A_ijk0+s_ijk1*A_ijk1);

				pcg_solver.z.valeur(x, y, z, v);
			}
		}
	}
}

// https://www.cs.ubc.ca/~rbridson/fluidsimulation/fluids_notes.pdf page 34
void solve_pressure(PCGSolver &pcg_solver, const Grille<char> &drapeaux)
{
	construit_preconditionneur(pcg_solver, drapeaux);
	applique_preconditionneur(pcg_solver, drapeaux);

	pcg_solver.s.copie(pcg_solver.z);

	auto sigma = produit_scalaire(pcg_solver.z, pcg_solver.r);

	const auto tol = 1e-6f;
	const auto rho = 1.0f;
	const auto max_iter = 100;

	int i = 0;
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

void construit_A(PCGSolver &pcg_solver, const Grille<char> &drapeaux)
{
	const auto &res = drapeaux.resolution();

	for (int z = 0; z < res.z; ++z) {
		for (int y = 0; y < res.y; ++y) {
			for (int x = 0; x < res.x; ++x) {
				//const auto p_i0jk = drapeaux.valeur(x - 1, y, z);
				const auto p_i1jk = drapeaux.valeur(x + 1, y, z);
				//const auto p_ij0k = drapeaux.valeur(x, y - 1, z);
				const auto p_ij1k = drapeaux.valeur(x, y + 1, z);
				//const auto p_ijk0 = drapeaux.valeur(x, y, z - 1);
				const auto p_ijk1 = drapeaux.valeur(x, y, z + 1);

				/* À FAIRE : compte le nombre de cellule qui ne sont pas solide */
				const auto coeff = 6.0f;

				pcg_solver.Adiag.valeur(x, y, z, coeff);

				pcg_solver.Aplusi.valeur(x, y, z, (p_i1jk == 0) ? -1.0f : 0.0f);
				pcg_solver.Aplusj.valeur(x, y, z, (p_ij1k == 0) ? -1.0f : 0.0f);
				pcg_solver.Aplusk.valeur(x, y, z, (p_ijk1 == 0) ? -1.0f : 0.0f);
			}
		}
	}
}

void soustrait_gradient_pression(Grille<float> &grille, const PCGSolver &pcg_solver)
{
	const auto &res = grille.resolution();

	for (int z = 0; z < res.z; ++z) {
		for (int y = 0; y < res.y; ++y) {
			for (int x = 0; x < res.x; ++x) {
				const auto p_i0jk = pcg_solver.p.valeur(x - 1, y, z);
				const auto p_i1jk = pcg_solver.p.valeur(x + 1, y, z);
				const auto p_ij0k = pcg_solver.p.valeur(x, y - 1, z);
				const auto p_ij1k = pcg_solver.p.valeur(x, y + 1, z);
				const auto p_ijk0 = pcg_solver.p.valeur(x, y, z - 1);
				const auto p_ijk1 = pcg_solver.p.valeur(x, y, z + 1);

				const auto px = p_i1jk - p_i0jk;
				const auto py = p_ij1k - p_ij0k;
				const auto pz = p_ijk1 - p_ijk0;

				const auto vx = grille.valeur(x + 1, y, z);
				const auto vy = grille.valeur(x, y + 1, z);
				const auto vz = grille.valeur(x, y, z + 1);

				grille.valeur(x + 1, y, z, vx - px);
				grille.valeur(x, y + 1, z, vy - py);
				grille.valeur(x, y, z + 1, vz - pz);
			}
		}
	}
}

void rend_imcompressible(Grille<dls::math::vec3f> &grille, const Grille<char> &drapeaux)
{
	const auto &res = grille.resolution();

	PCGSolver pcg_solver;
	pcg_solver.initialise(res);

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

	//soustrait_gradient_pression(grille, pcg_solver);
}

void rend_imcompressible(Grille<float> &grille_x, Grille<float> &grille_y, Grille<float> &grille_z, const Grille<char> &drapeaux)
{
	const auto &res = grille_x.resolution();

	PCGSolver pcg_solver;
	pcg_solver.initialise(res);

	//auto max_divergence = calcul_divergence(grille, grille, grille, pcg_solver.r);
	auto max_divergence = calcul_divergence(grille_x, grille_y, grille_z, pcg_solver.r);

	std::cerr << "---------------------------------------------------\n";
	std::cerr << "max_divergence avant PCG : " << max_divergence << '\n';

	/* Vérifie si le champs de vélocité est déjà (presque) non-divergent. */
	if (max_divergence <= 1e-6f) {
		std::cerr << "La vélocité n'est pas divergeante !\n";
		return;
	}

	construit_A(pcg_solver, drapeaux);

	solve_pressure(pcg_solver, drapeaux);

	//soustrait_gradient_pression(grille, pcg_solver);
}
