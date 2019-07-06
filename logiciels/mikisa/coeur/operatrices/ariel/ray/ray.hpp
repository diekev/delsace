// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: ray.hpp. Adapted from Takua Render.
// Classes for ray stuff

#ifndef RAY_HPP
#define RAY_HPP

#ifdef __CUDACC__
#define HOST __host__
#define DEVICE __device__
#define SHARED __shared__
#else
#define HOST
#define DEVICE
#define SHARED
#endif

#include "../utilities/utilities.h"

namespace rayCore {

//====================================
// Class Declarations
//====================================
class Ray {
public:
	Ray();
	Ray(const dls::math::vec3f& origin, const dls::math::vec3f& direction, const float& frame,
		const unsigned int& trackingID);
	Ray(const dls::math::vec3f& origin, const dls::math::vec3f& direction, const float& frame);
	~Ray() = default;

	void SetContents(const dls::math::vec3f& origin, const dls::math::vec3f& direction,
					 const float& frame, const unsigned int& trackingID);

	dls::math::vec3f GetPointAlongRay(const float& distance) const;
	Ray Transform(const dls::math::mat4x4f& m) const;

	dls::math::vec3f       m_origin{};
	dls::math::vec3f       m_direction{};
	unsigned int    m_trackingID{};
	float           m_frame{};
};

class Intersection {
public:
	Intersection();
	Intersection(const bool& hit, const dls::math::vec3f& point, const dls::math::vec3f& normal,
				 const dls::math::vec2f& uv, const unsigned int& objectID,
				 const unsigned int& primID);
	~Intersection();

	void SetContents(const bool& hit, const dls::math::vec3f& point,
					 const dls::math::vec3f& normal, const dls::math::vec2f& uv,
					 const unsigned int& objectID, const unsigned int& primID);

	Intersection CompareClosestAgainst(const Intersection& hit,
									   const dls::math::vec3f& point);
	Intersection Transform(const dls::math::mat4x4f& m);

	dls::math::vec3f       m_point{};
	dls::math::vec3f       m_normal{};
	dls::math::vec2f       m_uv{};
	unsigned int    m_objectID{};
	unsigned int    m_primID{};
	bool            m_hit{};
};
}

#endif
