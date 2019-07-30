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

#include "operatrices_snh.hh"

#include <memory>
#include <array>

#include "biblinternes/outils/definitions.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <iostream>
#include "biblexternes/snh/TetMesh.h"
#include "biblexternes/snh/StableNeoHookean.h"
#include "biblexternes/snh/TetNewtonSolver.h"
#pragma GCC diagnostic pop

#include "coeur/contexte_evaluation.hh"
#include "coeur/chef_execution.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

/* ************************************************************************** */

static std::vector<std::array<int,3>> g_surfaceTriangles;

static int g_nvx;
static int g_nvy;
static int g_nvz;

static CubeSim::TetMesh g_tetMesh;
static std::unique_ptr<CubeSim::Material> g_material;
static CubeSim::TetNewtonSolver<CubeSim::TetMesh> g_solver;

static void UpdateBoundaryConditions(const int stepNum, const CubeSim::Scalar& stepDelta)
{
	const double newNegativeBoundary = -1.0 - stepNum * stepDelta;
	for (int k = 0; k < g_nvz; k++)
	{
		constexpr int j = 0;
		for (int i = 0; i < g_nvx; i++)
		{
			const int vrtIdx = i + j * g_nvy + k * g_nvx * g_nvy;
			assert(g_tetMesh.IsVertexFixed(vrtIdx));
			CubeSim::Vector3 newPos = g_tetMesh.GetVertex(vrtIdx);
			newPos.y() = newNegativeBoundary;
			g_tetMesh.SetVertex(vrtIdx, newPos);
		}
	}

	const double newPositiveBoundary = 1.0 + stepNum * stepDelta;
	for (int k = 0; k < g_nvz; k++)
	{
		const int j = g_nvy - 1;
		for (int i = 0; i < g_nvx; i++)
		{
			const int vrtIdx =i + j * g_nvy + k * g_nvx * g_nvy;
			assert(g_tetMesh.IsVertexFixed(vrtIdx));
			CubeSim::Vector3 newPos = g_tetMesh.GetVertex(vrtIdx);
			newPos.y() = newPositiveBoundary;
			g_tetMesh.SetVertex(vrtIdx, newPos);
		}
	}

	g_tetMesh.UpdateCurrentCachedState();
}

static CubeSim::TetMesh GenerateTestMesh(const int resolution, const int nv, const int nc)
{
	const CubeSim::Scalar delta = 2.0 / CubeSim::Scalar(resolution);

	// Create the vertices
	const CubeSim::Vector3 origin(-1.0, -1.0, -1.0);
	std::vector<CubeSim::Vector3> verts(static_cast<unsigned long>(nv));
	for (int k = 0; k < g_nvz; k++)
	{
		for (int j = 0; j < g_nvy; j++)
		{
			for (int i = 0; i < g_nvx; i++)
			{
				const int vrtIdx = i + j * g_nvy + k * g_nvx * g_nvy;
				verts[static_cast<unsigned long>(vrtIdx)] = origin + delta * CubeSim::Vector3(i, j, k);
			}
		}
	}

	// Kinematically script two ends of the cube
	std::vector<bool> kinematic(static_cast<unsigned long>(nv), false);
	for (int k = 0; k < g_nvz; k++)
	{
		constexpr int j = 0;
		for (int i = 0; i < g_nvx; i++)
		{
			const int vrtIdx = i + j * g_nvy + k * g_nvx * g_nvy;
			kinematic[static_cast<unsigned long>(vrtIdx)] = true;
		}
	}
	for (int k = 0; k < g_nvz; k++)
	{
		const int j = g_nvy - 1;
		for (int i = 0; i < g_nvx; i++)
		{
			const int vrtIdx = i + j * g_nvy + k * g_nvx * g_nvy;
			kinematic[static_cast<unsigned long>(vrtIdx)] = true;
		}
	}

	// Create the tets
	constexpr std::array<std::array<int,4>,6> tetTemplates = {
		7, 4, 0, 5,
		7, 6, 0, 4,
		7, 5, 0, 1,
		7, 2, 0, 6,
		7, 1, 0, 3,
		7, 3, 0, 2
	};

	std::vector<CubeSim::Vector4i> tets(static_cast<unsigned long>(6 * nc));
	{
		int tetNum = 0;
		for (int k = 0; k < g_nvz - 1; k++)
		{
			for (int j = 0; j < g_nvy - 1; j++)
			{
				for (int i = 0; i < g_nvx - 1; i++)
				{
					const int c0 = i + j * g_nvy + k * g_nvx * g_nvy;
					const int c1 = c0 + 1;
					const int c2 = c0 + g_nvx;
					const int c3 = c1 + g_nvx;
					const std::array<int,8> cc = {
						c0,
						c1,
						c2,
						c3,
						c0 + g_nvx * g_nvy,
						c1 + g_nvx * g_nvy,
						c2 + g_nvx * g_nvy,
						c3 + g_nvx * g_nvy
					};

					for (int tetIdx = 0; tetIdx < 6; tetIdx++)
					{
						tets[static_cast<unsigned long>(tetNum)] << cc[static_cast<unsigned long>(tetTemplates[static_cast<unsigned long>(tetIdx)][0])], cc[static_cast<unsigned long>(tetTemplates[static_cast<unsigned long>(tetIdx)][1])], cc[static_cast<unsigned long>(tetTemplates[static_cast<unsigned long>(tetIdx)][2])], cc[static_cast<unsigned long>(tetTemplates[static_cast<unsigned long>(tetIdx)][3])];
						tetNum++;
					}
				}
			}
		}
		assert(tetNum == 6 * nc);
	}

	return CubeSim::TetMesh(verts, verts, tets, kinematic);
}

static std::vector<std::array<int,3>> ExtractSurfaceTriangles(const int nvx, const int nvy, const int nvz)
{
	std::vector<std::array<int,3>> surfaceTriangles;

	// -y side
	for (int k = 0; k < nvz - 1; k++)
	{
		constexpr int j = 0;
		for (int i = 0; i < nvx - 1; i++)
		{
			const int c0 = i + j * nvy + k * nvx * nvy;
			const int c1 = c0 + 1;
			const int c4 = c0 + nvx * nvy;
			const int c5 = c1 + nvx * nvy;
			surfaceTriangles.emplace_back(std::array<int,3>{c0, c1, c4});
			surfaceTriangles.emplace_back(std::array<int,3>{c4, c1, c5});
		}
	}
	// +y side
	for (int k = 0; k < nvz - 1; k++)
	{
		const int j = nvy - 2;
		for (int i = 0; i < nvx - 1; i++)
		{
			const int c0 = i + j * nvy + k * nvx * nvy;
			const int c1 = c0 + 1;
			const int c2 = c0 + nvx;
			const int c3 = c1 + nvx;
			const int c6 = c2 + nvx * nvy;
			const int c7 = c3 + nvx * nvy;
			surfaceTriangles.emplace_back(std::array<int,3>{c2, c6, c3});
			surfaceTriangles.emplace_back(std::array<int,3>{c3, c6, c7});
		}
	}
	// -z side
	{
		constexpr int k = 0;
		for (int j = 0; j < nvy - 1; j++)
		{
			for (int i = 0; i < nvx - 1; i++)
			{
				const int c0 = i + j * nvy + k * nvx * nvy;
				const int c1 = c0 + 1;
				const int c2 = c0 + nvx;
				const int c3 = c1 + nvx;
				surfaceTriangles.emplace_back(std::array<int,3>{c0, c2, c1});
				surfaceTriangles.emplace_back(std::array<int,3>{c1, c2, c3});
			}
		}
	}
	// +z side
	{
		const int k = nvz - 2;
		for (int j = 0; j < nvy - 1; j++)
		{
			for (int i = 0; i < nvx - 1; i++)
			{
				const int c0 = i + j * nvy + k * nvx * nvy;
				const int c1 = c0 + 1;
				const int c2 = c0 + nvx;
				const int c3 = c1 + nvx;
				const int c4 = c0 + nvx * nvy;
				const int c5 = c1 + nvx * nvy;
				const int c6 = c2 + nvx * nvy;
				const int c7 = c3 + nvx * nvy;
				surfaceTriangles.emplace_back(std::array<int,3>{c4, c5, c6});
				surfaceTriangles.emplace_back(std::array<int,3>{c5, c7, c6});
			}
		}
	}
	// -x side
	for (int k = 0; k < nvz - 1; k++)
	{
		for (int j = 0; j < nvy - 1; j++)
		{
			constexpr int i = 0;
			const int c0 = i + j * nvy + k * nvx * nvy;
			// const int c1 = c0 + 1;
			const int c2 = c0 + nvx;
			// const int c3 = c1 + nvx;
			const int c4 = c0 + nvx * nvy;
			// const int c5 = c1 + nvx * nvy;
			const int c6 = c2 + nvx * nvy;
			// const int c7 = c3 + nvx * nvy;
			surfaceTriangles.emplace_back(std::array<int,3>{c0, c6, c2});
			surfaceTriangles.emplace_back(std::array<int,3>{c0, c4, c6});
		}
	}
	// +x side
	for (int k = 0; k < nvz - 1; k++)
	{
		for (int j = 0; j < nvy - 1; j++)
		{
			const int i = nvx - 2;
			{
				const int c0 = i + j * nvy + k * nvx * nvy;
				const int c1 = c0 + 1;
				const int c3 = c1 + nvx;
				const int c5 = c1 + nvx * nvy;
				const int c7 = c3 + nvx * nvy;
				surfaceTriangles.emplace_back(std::array<int,3>{c1, c3, c7});
				surfaceTriangles.emplace_back(std::array<int,3>{c1, c7, c5});
			}
		}
	}

	return surfaceTriangles;
}

/* ************************************************************************** */

class OperatriceSNH : public OperatriceCorps {
public:
	static constexpr auto NOM = "Stable Neo Hookean";
	static constexpr auto AIDE = "";

	OperatriceSNH(Graphe &graphe_parent, Noeud *noeud);

	const char *chemin_entreface() const override;

	const char *nom_classe() const override;

	const char *texte_aide() const override;

	void converti_corps();

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;
};

OperatriceSNH::OperatriceSNH(Graphe &graphe_parent, Noeud *noeud)
	: OperatriceCorps(graphe_parent, noeud)
{
	entrees(1);
	sorties(1);
}

const char *OperatriceSNH::chemin_entreface() const
{
	return "";
}

const char *OperatriceSNH::nom_classe() const
{
	return NOM;
}

const char *OperatriceSNH::texte_aide() const
{
	return AIDE;
}

void OperatriceSNH::converti_corps()
{
	m_corps.reinitialise();
	auto points = m_corps.points();
	points->reserve(static_cast<long>(g_tetMesh.GetVertices().size()));

	for (const CubeSim::Vector3& p : g_tetMesh.GetVertices()) {
		m_corps.ajoute_point(static_cast<float>(p.x()), static_cast<float>(p.y()), static_cast<float>(p.z()));
	}

	for (const std::array<int,3>& f : g_surfaceTriangles) {
		auto poly = Polygone::construit(&m_corps, type_polygone::FERME, 3);
		poly->ajoute_sommet(f[0]);
		poly->ajoute_sommet(f[1]);
		poly->ajoute_sommet(f[2]);
	}
}

int OperatriceSNH::execute(const ContexteEvaluation &contexte, DonneesAval *donnees_aval)
{
	INUTILISE(donnees_aval);
	m_corps.reinitialise();

	auto chef = contexte.chef;
	chef->demarre_evaluation("stable_neo_hookean");

	if (contexte.temps_courant == 1) {
		// reinitialise
		auto resolution = 10;
		auto mu = 1.0; // >= 0.0
		auto lambda = 10.0; // >= 0.0

		g_nvx = 1 + resolution;
		g_nvy = 1 + resolution;
		g_nvz = 1 + resolution;
		const int nv = g_nvx * g_nvy * g_nvz;
		const int nc = (g_nvx - 1) * (g_nvy - 1) * (g_nvz - 1);

		g_material.reset(new CubeSim::StableNeoHookean(mu, lambda));
		g_tetMesh = GenerateTestMesh(resolution, nv, nc);

		chef->indique_progression(25.0f);

		g_solver.Resize(3 * g_tetMesh.GetNumSimulatedVertices());
		g_tetMesh.BuildHessianWithPlaceholderValues(g_solver.GetH());

		chef->indique_progression(50.0f);

		constexpr CubeSim::Scalar cgTol = 1.0e-6;
		const int maxCGIters = 2 * g_tetMesh.GetNumSimulatedVertices();
		g_solver.InitLinearSolver(maxCGIters, cgTol);

		chef->indique_progression(75.0f);

		g_surfaceTriangles = ExtractSurfaceTriangles(g_nvx, g_nvy, g_nvz);

		converti_corps();

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}

	auto stepNum = contexte.temps_courant;
	auto stepSize = 0.1;

	// lance simulation
	UpdateBoundaryConditions(stepNum, stepSize);

	chef->indique_progression(25.0f);

	{
		constexpr bool printStatus = false;
		constexpr CubeSim::Scalar forceTol = 1.0e-6;
		constexpr int minIters = 0;
		constexpr int maxIters = 50;

		auto const result = g_solver.Solve(printStatus, *g_material, forceTol, minIters, maxIters, g_tetMesh);

		if (!result.succeeded) {
			this->ajoute_avertissement("Warning, quasistatic solve failed.");
		}
	}

	chef->indique_progression(75.0f);

	converti_corps();

	chef->indique_progression(100.0f);

	return EXECUTION_REUSSIE;
}

/* ************************************************************************** */

void enregistre_operatrices_snh(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceSNH>());
}
