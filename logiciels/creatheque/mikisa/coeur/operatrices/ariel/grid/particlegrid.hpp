// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: particlegrid.cpp
// Helper class for projecting particles back onto the grid and storing particles in cells

#ifndef FLUIDGRID_HPP
#define FLUIDGRID_HPP

#include <tbb/tbb.h>
#include "../utilities/utilities.h"
#include "grid.hpp"

namespace fluidCore {
//====================================
// Class Declarations
//====================================

class ParticleGrid {
public:
	//Initializers
	ParticleGrid(const dls::math::vec3i& dimensions);
	ParticleGrid(const int& x, const int& y, const int& z);
	~ParticleGrid();

	ParticleGrid(ParticleGrid const &) = default;
	ParticleGrid &operator=(ParticleGrid const &) = default;

	//Sorting tools
	void Sort(std::vector<Particle*>& particles);
	std::vector<Particle*> GetCellNeighbors(const dls::math::vec3f& index,
											const dls::math::vec3f& numberOfNeighbors);
	std::vector<Particle*> GetWallNeighbors(const dls::math::vec3f& index,
											const dls::math::vec3f& numberOfNeighbors);

	void MarkCellTypes(std::vector<Particle*>& particles, Grid<int>* A,
					   const float& density);
	float CellSDF(const int& i, const int& j, const int& k, const float& density,
				  const geomtype& type);

	void BuildSDF(MacGrid& mgrid, const float& density);

private:
	void Init(const int& x, const int& y, const int& z);

	dls::math::vec3i                            m_dimensions{};
	Grid<int>*                                  m_grid{};
	std::vector< std::vector<Particle*> >       m_cells{};

};
}

#endif
