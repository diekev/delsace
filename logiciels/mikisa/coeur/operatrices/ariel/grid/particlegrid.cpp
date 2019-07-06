// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: particlegrid.cpp
// Implements particlegrid.hpp

#include "particlegrid.hpp"

namespace fluidCore{

ParticleGrid::ParticleGrid(const dls::math::vec3i& dim)
{
	Init(dim.x, dim.y, dim.z);
}

ParticleGrid::ParticleGrid(const int& x, const int& y, const int& z)
{
	Init(x, y, z);
}

ParticleGrid::~ParticleGrid()
{
	delete m_grid;
}

void ParticleGrid::Init(const int& x, const int& y, const int& z)
{
	m_dimensions = dls::math::vec3i(x,y,z);
	m_grid = new Grid<int>(dls::math::vec3i(x,y,z), -1);
}

dls::tableau<Particle*> ParticleGrid::GetCellNeighbors(
		const dls::math::vec3f& index,
		const dls::math::vec3f& numberOfNeighbors)
{
	//loop through neighbors, for each neighbor, check if cell has particles and push back contents
	dls::tableau<Particle*> neighbors;

	auto ix = index.x;
	auto iy = index.y;
	auto iz = index.z;

	auto nvx = numberOfNeighbors.x;
	auto nvy = numberOfNeighbors.y;
	auto nvz = numberOfNeighbors.z;

	for (auto sx = ix - numberOfNeighbors.x; sx <= ix + nvx; sx += 1.0f) {
		for (auto sy = iy - numberOfNeighbors.y; sy <= iy + nvy; sy += 1.0f) {
			for (auto sz = iz - numberOfNeighbors.z; sz <= iz + nvz; sz += 1.0f) {
				if ( sx < 0.0f || sx > static_cast<float>(m_dimensions.x)-1.0f
					 || sy < 0.0f || sy > static_cast<float>(m_dimensions.y)-1.0f
					 || sz < 0.0f || sz > static_cast<float>(m_dimensions.z)-1.0f ) {
					continue;
				}

				auto cellindex = m_grid->GetCell(dls::math::vec3f(sx, sy, sz));

				if (cellindex != -1l) {
					auto cellparticlecount = m_cells[cellindex].taille();

					for (auto a = 0l; a < cellparticlecount; a++) {
						neighbors.pousse(m_cells[cellindex][a]);
					}
				}
			}
		}
	}

	return neighbors;
}

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

void ParticleGrid::Sort(dls::tableau<Particle*>& particles)
{
	// clear existing cells
	for (auto &cellule : m_cells) {
		cellule.clear();
	}

	auto maxd = std::max(std::max(m_dimensions.x, m_dimensions.y), m_dimensions.z);

	auto particlecount = particles.taille();
	auto cellscount = m_cells.taille();

	// cout << particlecount << endl;
	for (auto i = 0l; i < particlecount; i++) {
		auto p = particles[i];

		auto pos = p->m_p;
		pos.x = std::max(0.0f, std::min(static_cast<float>(maxd) - 1.0f, static_cast<float>(maxd) * pos.x));
		pos.y = std::max(0.0f, std::min(static_cast<float>(maxd) - 1.0f, static_cast<float>(maxd) * pos.y));
		pos.z = std::max(0.0f, std::min(static_cast<float>(maxd) - 1.0f, static_cast<float>(maxd) * pos.z));

		auto cellindex = m_grid->GetCell(pos);

		if (cellindex>=0) { //if grid has value here, a cell already exists for it
			m_cells[cellindex].pousse(p);
		}
		else { //if grid has no value, create new cell and push index to grid
			dls::tableau<Particle*> cell;
			cell.pousse(p);
			m_cells.pousse(cell);
			m_grid->SetCell(pos, static_cast<int>(cellscount));
			cellscount++;
		}
	}
}

}
