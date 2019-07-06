// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: spatial.hpp. Adapted from Takua Render.
// Spatial acceleration stuff

#ifndef SPATIAL_HPP
#define SPATIAL_HPP

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
#include "../utilities/utilities.h"

namespace spaceCore {

//====================================
// Class Declarations
//===================================

class TraverseAccumulator {
public:
	TraverseAccumulator() = default;
	TraverseAccumulator(const dls::math::vec3f& origin);
	virtual ~TraverseAccumulator() = default;

	virtual void RecordIntersection(const rayCore::Intersection& intersect,
									const unsigned int& nodeid);
	virtual void Transform(const dls::math::mat4x4f& m);

	rayCore::Intersection   m_intersection{};
	unsigned int            m_nodeid{};
	dls::math::vec3f               m_origin{};
};

class HitCountTraverseAccumulator: public TraverseAccumulator {
public:
	HitCountTraverseAccumulator() = default;
	HitCountTraverseAccumulator(const dls::math::vec3f& origin);
	virtual ~HitCountTraverseAccumulator() = default;

	void RecordIntersection(const rayCore::Intersection& intersect,
							const unsigned int& nodeid);
	void Transform(const dls::math::mat4x4f& m);

	rayCore::Intersection   m_intersection{};
	unsigned int            m_nodeid{};
	unsigned int            m_numberOfHits{};
	dls::math::vec3f               m_origin{};
};

class DebugTraverseAccumulator: public TraverseAccumulator {
public:
	DebugTraverseAccumulator() = default;
	virtual ~DebugTraverseAccumulator() = default;

	void RecordIntersection(const rayCore::Intersection& intersect,
							const unsigned int& nodeid);
	void Transform(const dls::math::mat4x4f& m);

	dls::tableau<rayCore::Intersection> m_intersections{};
	dls::tableau<unsigned int> m_nodeids{};
};
}

#endif
