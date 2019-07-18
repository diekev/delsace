// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: aabb.cpp. Adapted from Takua Render.
// Implements aabb.hpp

#include "aabb.hpp"

namespace spaceCore {

static constexpr auto REALLY_BIG_NUMBER = 1000000000000000000.0f;

Aabb::Aabb()
{
	SetContents(dls::math::vec3f(REALLY_BIG_NUMBER), dls::math::vec3f(-REALLY_BIG_NUMBER), dls::math::vec3f(0.0f), -1);
}

Aabb::Aabb(const dls::math::vec3f& min, const dls::math::vec3f& max, const int& id)
{
	auto const center = (min + max)/2.0f;
	SetContents(min, max, center, id);
}

Aabb::Aabb(const dls::math::vec3f& min, const dls::math::vec3f& max, const dls::math::vec3f& centroid, const int& id)
{
	SetContents(min, max, centroid, id);
}

float Aabb::FastIntersectionTest(const rayCore::Ray& r)
{
	if (r.m_origin.x >= m_min.x && r.m_origin.y >= m_min.y && r.m_origin.z >= m_min.z &&
			r.m_origin.x <= m_max.x && r.m_origin.y <= m_max.y && r.m_origin.z <= m_max.z) {
		return 0;
	}

	auto const rdirection = r.m_direction;
	auto const dirfrac = dls::math::vec3f(1.0f / rdirection.x, 1.0f / rdirection.y, 1.0f / rdirection.z);

	auto const t1 = (m_min.x - r.m_origin.x)*dirfrac.x;
	auto const t2 = (m_max.x - r.m_origin.x)*dirfrac.x;
	auto const t3 = (m_min.y - r.m_origin.y)*dirfrac.y;
	auto const t4 = (m_max.y - r.m_origin.y)*dirfrac.y;
	auto const t5 = (m_min.z - r.m_origin.z)*dirfrac.z;
	auto const t6 = (m_max.z - r.m_origin.z)*dirfrac.z;
	auto const tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	auto const tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	if (tmax < 0) {
		return -1;
	}

	if (tmin > tmax) {
		return -1;
	}

	return tmin;
}

Aabb Aabb::Transform(const dls::math::mat4x4f& transform)
{
	//transform all corners of aabb
	auto const tmin = dls::math::vec3f(transform * dls::math::vec4f(m_min, 1.0f));
	auto const tmax = dls::math::vec3f(transform *  dls::math::vec4f(m_max, 1.0f));
	auto const tcentroid = dls::math::vec3f(transform * dls::math::vec4f(m_centroid, 1.0f));
	auto const m0 = dls::math::vec3f(transform * dls::math::vec4f(m_max.x, m_min.y, m_min.z, 1.0f));
	auto const m1 = dls::math::vec3f(transform * dls::math::vec4f(m_max.x, m_min.y, m_max.z, 1.0f));
	auto const m2 = dls::math::vec3f(transform * dls::math::vec4f(m_max.x, m_max.y, m_min.z, 1.0f));
	auto const m3 = dls::math::vec3f(transform * dls::math::vec4f(m_min.x, m_min.y, m_max.z, 1.0f));
	auto const m4 = dls::math::vec3f(transform * dls::math::vec4f(m_min.x, m_max.y, m_min.z, 1.0f));
	auto const m5 = dls::math::vec3f(transform * dls::math::vec4f(m_min.x, m_max.y, m_max.z, 1.0f));
	//build new aabb, return
	Aabb a;
	a.m_min = std::min(std::min(std::min(tmin, tmax), std::min(m0, m1)),
					   std::min(std::min(m2, m3), std::min(m4, m5)));
	a.m_max = std::max(std::max(std::max(tmin, tmax), std::max(m0, m1)),
					   std::max(std::max(m2, m3), std::max(m4, m5)));
	a.m_id = m_id;
	a.m_centroid = tcentroid;
	return a;
}

double Aabb::CalculateSurfaceArea()
{
	auto xlength = m_max.x-m_min.x;
	auto ylength = m_max.y-m_min.y;
	auto zlength = m_max.z-m_min.z;
	return static_cast<double>(2.0f*((xlength*ylength)+(ylength*zlength)+(zlength*xlength)));
}

void Aabb::SetContents(
		const dls::math::vec3f& min,
		const dls::math::vec3f& max,
		const dls::math::vec3f& centroid,
		const int& id)
{
	m_min = min;
	m_max = max;
	m_centroid = centroid;
	m_id = id;
}

void Aabb::ExpandAabb(const dls::math::vec3f& exMin, const dls::math::vec3f& exMax)
{
	m_min = std::min(m_min, exMin);
	m_max = std::max(m_max, exMax);
}

}
