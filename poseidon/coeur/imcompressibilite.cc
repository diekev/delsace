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
	Grille<float> M{};
	Grille<float> Adiag{};
	Grille<float> Aplusi{};
	Grille<float> Aplusj{};
	Grille<float> Aplusk{};

	/* Pression */
	Grille<float> p{};

	/* Résidus, initiallement la divergence du champs de vélocité. */
	Grille<float> r{};

	/* Vecteur auxiliaire */
	Grille<float> z{};

	/* Vecteur de recherche */
	Grille<float> s{};

	void initialise(dls::math::vec3<size_t> const &res)
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
		Grille<float> const &grille_x,
		Grille<float> const &grille_y,
		Grille<float> const &grille_z,
		Grille<float> &d)
{
	auto const res = d.resolution();
	float max_divergence = 0.0f;

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
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

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
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

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
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
	auto const res = drapeaux.resolution();

	Grille<float> q;
	q.initialise(res.x, res.y, res.z);

	/* Résoud Lq = r */
	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
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
	for (size_t z = res.z - 1; z < -1ul; --z) {
		for (size_t y = res.y - 1; y < -1ul; --y) {
			for (size_t x = res.x - 1; x < -1ul; --x) {
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

	for (size_t z = 0; z < res_z; ++z) {
		for (size_t y = 0; y < res_y; ++y) {
			for (size_t x = 0; x < res_x; ++x) {
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

	for (size_t x = 0; x < res_x; ++x) {
		for (size_t y = 0; y < res_y; ++y) {
			for (size_t z = 0; z < res_z; ++z) {
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

	for (size_t x = 0; x < res_x; ++x) {
		for (size_t y = 0; y < res_y; ++y) {
			for (size_t z = 0; z < res_z; ++z) {
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

	for (size_t x = 0; x < res_x; ++x) {
		for (size_t y = 0; y < res_y; ++y) {
			for (size_t z = 0; z < res_z; ++z) {
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

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
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

	pcg_solver.s.copie(pcg_solver.z);

	auto sigma = produit_scalaire(pcg_solver.z, pcg_solver.r);

	auto const tol = 1e-6f;
	auto const rho = 1.0f;
	auto const max_iter = 100;

	size_t i = 0;
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

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
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

void soustrait_gradient_pression(Grille<float> &grille, PCGSolver const &pcg_solver)
{
	auto const &res = grille.resolution();

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
				auto const p_i0jk = pcg_solver.p.valeur(x - 1, y, z);
				auto const p_i1jk = pcg_solver.p.valeur(x + 1, y, z);
				auto const p_ij0k = pcg_solver.p.valeur(x, y - 1, z);
				auto const p_ij1k = pcg_solver.p.valeur(x, y + 1, z);
				auto const p_ijk0 = pcg_solver.p.valeur(x, y, z - 1);
				auto const p_ijk1 = pcg_solver.p.valeur(x, y, z + 1);

				auto const px = p_i1jk - p_i0jk;
				auto const py = p_ij1k - p_ij0k;
				auto const pz = p_ijk1 - p_ijk0;

				auto const vx = grille.valeur(x + 1, y, z);
				auto const vy = grille.valeur(x, y + 1, z);
				auto const vz = grille.valeur(x, y, z + 1);

				grille.valeur(x + 1, y, z, vx - px);
				grille.valeur(x, y + 1, z, vy - py);
				grille.valeur(x, y, z + 1, vz - pz);
			}
		}
	}
}

void rend_imcompressible(Grille<dls::math::vec3f> &grille, Grille<char> const &drapeaux)
{
	auto const &res = grille.resolution();

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

void rend_imcompressible(Grille<float> &grille_x, Grille<float> &grille_y, Grille<float> &grille_z, Grille<char> const &drapeaux)
{
	auto const &res = grille_x.resolution();

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
