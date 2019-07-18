// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: aabb.hpp. Adapted from Takua Render.
// Axis aligned bounding box stuff

#ifndef AABB_HPP
#define AABB_HPP

#ifdef __CUDACC__
#define HOST __host__
#define DEVICE __device__
#define SHARED __shared__
#else
#define HOST
#define DEVICE
#define SHARED
#endif

#include "../ray/ray.hpp"
#include "biblinternes/math/matrice.hh"

namespace spaceCore {

//====================================
// Class Declarations
//====================================

class Aabb {
public:
	Aabb();
	Aabb(const dls::math::vec3f& min, const dls::math::vec3f& max, const int& id);
	Aabb(const dls::math::vec3f& min, const dls::math::vec3f& max, const dls::math::vec3f& centroid, const int& id);
	~Aabb() = default;

	void SetContents(const dls::math::vec3f& min, const dls::math::vec3f& max, const dls::math::vec3f& centroid,
					 const int& id);
	void ExpandAabb(const dls::math::vec3f& exMin, const dls::math::vec3f& exMax);
	double CalculateSurfaceArea();
	//"fast" as in only returns distance and not a full intersection
	float FastIntersectionTest(const rayCore::Ray& r);
	Aabb Transform(const dls::math::mat4x4f& transform);

	dls::math::vec3f   m_min{};
	dls::math::vec3f   m_max{};
	dls::math::vec3f   m_centroid{};
	int         m_id{}; //what this id actually means is entirely dependent on the context
};
}

#endif
