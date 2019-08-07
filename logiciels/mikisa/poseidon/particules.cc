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
						neighbors.pousse(m_cells[cellindex][a]);
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

}  /* namespace psn */
