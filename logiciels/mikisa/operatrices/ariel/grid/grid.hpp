// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: grid.hpp
// Templatized grid class

#ifndef __INCLUS_GRILLE_ARIEL__
#define __INCLUS_GRILLE_ARIEL__

#include <tbb/tbb.h>
#include "biblinternes/math/vecteur.hh"

namespace fluidCore {

enum geomtype {
	SOLID=2,
	FLUID=1,
	AIR=0
};

//====================================
// Class Declarations
//====================================

template <typename T>
class Grid {
public:
	//Initializers
	Grid(const dls::math::vec3i& dimensions, const T& background);
	~Grid();

	Grid(Grid const &) = default;
	Grid &operator=(Grid const &) = default;

	//Cell accessors and setters and whatever
	T GetCell(const dls::math::vec3f& index);
	T GetCell(const int& x, const int& y, const int& z);

	void SetCell(const dls::math::vec3f& index, const T& value);
	void SetCell(const int& x, const int& y, const int& z, const T& value);

	void Clear();

protected:
	T***        m_rawgrid{};
	T           m_background{};

	dls::math::vec3i   m_dimensions{};
};

template <class T>
T *** CreateGrid(int x, int y, int z)
{
	T *** field = new T **[static_cast<size_t>(x)];

	for (int i=0; i<x; i++) {
		field[i] = new T*[static_cast<size_t>(y)];

		for (int j=0; j<y; j++) {
			field[i][j] = new T[static_cast<size_t>(z)];
		}
	}

	return field;
}

template <class T>
void DeleteGrid(T ***ptr, int x, int y, int z)
{
	for (int i=0; i<x; i++) {
		for (int j=0; j<y; j++) {
			delete [] ptr[i][j];
		}

		delete [] ptr[i];
	}

	delete [] ptr;
}

template <typename T>
Grid<T>::Grid(const dls::math::vec3i& dimensions, const T& background)
	: m_background(background)
	, m_dimensions(dimensions)
{
	m_rawgrid = CreateGrid<T>(m_dimensions.x+1, m_dimensions.y+1, m_dimensions.z+1);

	tbb::parallel_for(tbb::blocked_range<int>(0, m_dimensions.x + 1),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (int i=r.begin(); i!=r.end(); ++i) {
			for (int j=0; j < m_dimensions.y+1; ++j) {
				for (int k=0; k< m_dimensions.z+1; ++k) {
					m_rawgrid[i][j][k] = m_background;
				}
			}
		}
	});
}

template <typename T>
Grid<T>::~Grid()
{
	DeleteGrid<T>(m_rawgrid, m_dimensions.x+1, m_dimensions.y+1, m_dimensions.z+1);
}

template <typename T>
T Grid<T>::GetCell(const dls::math::vec3f& index)
{
	return GetCell(static_cast<int>(index.x), static_cast<int>(index.y), static_cast<int>(index.z));
}

template <typename T>
T Grid<T>::GetCell(const int& x, const int& y, const int& z)
{
	T cell = m_rawgrid[x][y][z];
	return cell;
}

template <typename T>
void Grid<T>::SetCell(const dls::math::vec3f& index, const T& value)
{
	SetCell(static_cast<int>(index.x), static_cast<int>(index.y), static_cast<int>(index.z), value);
}

template <typename T> void Grid<T>::SetCell(const int& x, const int& y, const int& z,
											const T& value)
{
	m_rawgrid[x][y][z] = value;
}

template <typename T> void Grid<T>::Clear()
{
	tbb::parallel_for(tbb::blocked_range<int>(0, m_dimensions.x+1),
					  [&](const tbb::blocked_range<int>& r)
	{
		for (int i=r.begin(); i!=r.end(); ++i) {
			for (int j=0; j< m_dimensions.y+1; ++j) {
				for (int k=0; k< m_dimensions.z+1; ++k) {
					m_rawgrid[i][j][k] = m_background;
				}
			}
		}
	});
}

#define FOR_EACH_CELL(x, y, z) \
	for (int i = 0; i < x; i++) \
	for (int j = 0; j < y; j++) \
	for (int k = 0; k < z; k++)

#define FOR_EACH_FLOW_X(x, y, z) \
	for (int i = 0; i < x+1; i++)  \
	for (int j = 0; j < y; j++) \
	for (int k = 0; k < z; k++)

#define FOR_EACH_FLOW_Y(x, y, z) \
	for (int i = 0; i < x; i++)  \
	for (int j = 0; j < y+1; j++) \
	for (int k = 0; k < z; k++)

#define FOR_EACH_FLOW_Z(x, y, z) \
	for (int i = 0; i < x; i++)  \
	for (int j = 0; j < y; j++) \
	for (int k = 0; k < z+1; k++)


struct MacGrid {
	dls::math::vec3i       m_dimensions{};

	//face velocities
	Grid<float>*    m_u_x{};
	Grid<float>*    m_u_y{};
	Grid<float>*    m_u_z{};
	//technically this is the part that is an actual MAC grid, the rest is other useful stuff

	Grid<float>*    m_D{}; //divergence
	Grid<float>*    m_P{}; //pressure
	Grid<int>*      m_A{}; //cell type
	Grid<float>*    m_L{}; //internal lightweight SDF for project step
};

struct Particle {
	dls::math::vec3f       m_p{}; //position
	dls::math::vec3f       m_u{}; //velocity
	dls::math::vec3f       m_n{}; //normal
	float           m_density{};
	float           m_mass{};
	int             m_type{};
	dls::math::vec3f       m_t{};
	dls::math::vec3f       m_t2{};
	dls::math::vec3f       m_ut{}; //copy of previous velocity used for bound check correction
	dls::math::vec3f       m_pt{}; //copy of previous position used for bound checks
	bool            m_invalid{};
	bool            m_temp2{};
	bool            m_temp{};
};

//Forward declarations for externed inlineable methods

//====================================
// Function Implementations
//====================================

inline Particle CreateParticle(const dls::math::vec3f& position, const dls::math::vec3f& velocity,
							   const dls::math::vec3f& normal, const float& density) {
	Particle p;
	p.m_p = position;
	p.m_u = velocity;
	p.m_n = normal;
	p.m_density= density;
	return p;
}

inline MacGrid CreateMacgrid(dls::math::vec3i const &dimensions)
{
	auto const x = dimensions.x;
	auto const y = dimensions.y;
	auto const z = dimensions.z;

	MacGrid m;
	m.m_dimensions = dimensions;
	m.m_u_x = new Grid<float>(dls::math::vec3i(x+1,y,z), 0.0f);
	m.m_u_y = new Grid<float>(dls::math::vec3i(x,y+1,z), 0.0f);
	m.m_u_z = new Grid<float>(dls::math::vec3i(x,y,z+1), 0.0f);
	m.m_D = new Grid<float>(dls::math::vec3i(x,y,z), 0.0f);
	m.m_P = new Grid<float>(dls::math::vec3i(x,y,z), 0.0f);
	m.m_A = new Grid<int>(dls::math::vec3i(x,y,z), 0);
	m.m_L = new Grid<float>(dls::math::vec3i(x,y,z), 1.6f);

	return m;
}

inline void ClearMacgrid(MacGrid& m) {
	delete m.m_u_x;
	delete m.m_u_y;
	delete m.m_u_z;
	delete m.m_D;
	delete m.m_P;
}

}

#endif
