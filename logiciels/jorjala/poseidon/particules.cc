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

#include "particules.hh"

#include "wolika/iteration.hh"

#include "monde.hh"

namespace psn {

#if 0
dls::tableau<Particle*> ParticleGrid::GetWallNeighbors(
		const dls::math::vec3f& index,
		const dls::math::vec3f& numberOfNeighbors)
{
	dls::tableau<Particle*> neighbors;

	auto ix = index.x;
	auto iy = index.y;
	auto iz = index.z;

	auto nvx = numberOfNeighbors.x;
	auto nvy = numberOfNeighbors.y;
	auto nvz = numberOfNeighbors.z;

	for (auto sx = ix - nvx; sx <= ix + nvx - 1.0f; sx += 1.0f) {
		for (auto sy= iy - nvy; sy <= iy + nvy - 1.0f; sy += 1.0f) {
			for (auto sz= iz - nvz; sz <= iz + nvz - 1.0f; sz += 1.0f) {
				if (sx < 0.0f || sx > static_cast<float>(m_dimensions.x)-1.0f
						|| sy < 0.0f || sy > static_cast<float>(m_dimensions.y)-1.0f
						|| sz < 0.0f || sz > static_cast<float>(m_dimensions.z)-1.0f )
				{
					continue;
				}

				auto cellindex = m_grid->GetCell(dls::math::vec3f(sx, sy, sz));

				if (cellindex != -1l) {
					auto cellparticlecount = m_cells[cellindex].taille();
					for (auto a = 0l; a<cellparticlecount; a++) {
						neighbors.ajoute(m_cells[cellindex][a]);
					}
				}
			}
		}
	}

	return neighbors;
}

float ParticleGrid::CellSDF(
		const int& i,
		const int& j,
		const int& k,
		const float& density,
		const geomtype& type)
{
	auto accm = 0.0f;
	auto cellindex = m_grid->GetCell(i,j,k);

	if (cellindex>=0) {
		for (auto a = 0; a < m_cells[cellindex].taille(); a++ ) {
			if (m_cells[cellindex][a]->m_type == type) {
				accm += m_cells[cellindex][a]->m_density;
			}
			else {
				return 1.0f;
			}
		}
	}

	auto n0 = 1.0f / (density * density * density);
	return 0.2f*n0-accm;
}

void ParticleGrid::BuildSDF(
		MacGrid& mgrid,
		const float& density)
{
	int x = m_dimensions.x; int y = m_dimensions.y; int z = m_dimensions.z;
	mgrid.m_L->Clear();

	tbb::parallel_for(tbb::blocked_range<int>(0, x),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (int i=r.begin(); i!=r.end(); ++i) {
			for (int j = 0; j < y; ++j) {
				for (int k = 0; k < z; ++k) {
					mgrid.m_L->SetCell(i, j, k, CellSDF(i, j, k, density, FLUID));
				}
			}
		}
	});
}

void ParticleGrid::MarkCellTypes(
		dls::tableau<Particle*>& particles,
		Grid<int>* A,
		const float& density)
{
	auto x = m_dimensions.x;
	auto y = m_dimensions.y;
	auto z = m_dimensions.z;

	tbb::parallel_for(tbb::blocked_range<int>(0, x),
					  [&](const tbb::blocked_range< int>& r)
	{
		for (int i=r.begin(); i!=r.end(); ++i) {
			for (int j = 0; j < y; ++j) {
				for (int k = 0; k < z; ++k) {
					A->SetCell(i,j,k, AIR);
					auto cellindex = m_grid->GetCell(i,j,k);

					if (cellindex != -1l && cellindex<m_cells.taille()) {
						for (auto a = 0; a<m_cells[cellindex].taille(); a++) {
							if (m_cells[cellindex][a]->m_type == SOLID) {
								A->SetCell(i,j,k, SOLID);
							}
						}
					}

					if ( A->GetCell(i,j,k) != SOLID ) {
						bool isfluid = CellSDF(i, j, k, density, FLUID) < 0.0f;
						if (isfluid) {
							A->SetCell(i,j,k, FLUID);
						}
						else {
							A->SetCell(i,j,k, AIR);
						}
					}
				}
			}
		}
	});
}
#endif

/* Low order B-Spline. */
struct KernelBSP2 {
	static const int rayon = 1;

	static inline float poids(dls::math::vec3f const &v, float dx_inv)
	{
		auto r = longueur(v) * dx_inv;

		if (r > 1.0f) {
			return 0.0f;
		}

		return (1.0f - r);
	}
};

/* Fonction M'4 */
struct KernelMP4 {
	static const int rayon = 2;

	static inline float poids(dls::math::vec3f const &v, float dx_inv)
	{
		auto r = longueur(v) * dx_inv;

		if (r > 2.0f) {
			return 0.0f;
		}

		if (r >= 1.0f) {
			return 0.5f * (2.0f * r * r) * (1.0f - r);
		}

		return 1.0f - (2.5f * r * r) + (1.5f * r * r * r);
	}
};

void transfere_particules_grille(Poseidon &poseidon)
{
	auto densite = poseidon.densite;
	auto &grille_particules = poseidon.grille_particule;

	for (auto i = 0; i < densite->nombre_elements(); ++i) {
		densite->valeur(i) = 0.0f;
	}

	auto dx_inv = static_cast<float>(1.0 / densite->desc().taille_voxel);
	auto res = densite->desc().resolution;

	using type_kernel = KernelBSP2;

	auto dens_parts = poseidon.parts.champs_scalaire("densité");
	auto pos_parts  = poseidon.parts.champs_vectoriel("position");

	boucle_parallele(tbb::blocked_range<int>(0, res.z - 1),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto lims = limites3i{};
		lims.min = dls::math::vec3i(0, 0, plage.begin());
		lims.max = dls::math::vec3i(res.x - 1, res.y - 1, plage.end());

		auto iter = wlk::IteratricePosition(lims);

		while (!iter.fini()) {
			auto pos_index = iter.suivante();

			auto pos_monde = densite->index_vers_monde(pos_index);

			auto voisines = grille_particules.voisines_cellules(pos_index, dls::math::vec3i(type_kernel::rayon));

			/* utilise le filtre BSP2 */
			auto valeur = 0.0f;
			auto poids = 0.0f;

			for (auto pv : voisines) {
				auto r = type_kernel::poids(pos_monde - pos_parts[pv], dx_inv);
				valeur += r * dens_parts[pv];
				poids += r;
			}

			if (poids != 0.0f) {
				valeur /= poids;
			}

			densite->valeur(pos_index) = valeur;
		}
	});
}

}  /* namespace psn */
