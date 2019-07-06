// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: spatial.cpp. Adapted from Takua Render.
// Implements spatial.hpp

#include "spatial.hpp"

namespace spaceCore {

TraverseAccumulator::TraverseAccumulator(const dls::math::vec3f& origin)
	: m_origin(origin)
{}

//The default traverse accumulator does not do anything even remotely interesting
void TraverseAccumulator::RecordIntersection(const rayCore::Intersection& intersect,
											 const unsigned int& nodeid) {
	if (m_intersection.m_hit==false && intersect.m_hit==true) {
        m_intersection = intersect;
        m_nodeid = nodeid;   
	}
	else if (intersect.m_hit==true) {
        float currentDistance = longueur(m_intersection.m_point - m_origin);
        float newDistance = longueur(intersect.m_point - m_origin);

		if (newDistance<currentDistance) {
            m_intersection = intersect;
            m_nodeid = nodeid;   
        }
    }
}

void TraverseAccumulator::Transform(const dls::math::mat4x4f& m)
{
    m_intersection = m_intersection.Transform(m);
    m_origin = dls::math::vec3f(utilityCore::multiply(m, dls::math::vec4f(m_origin, 1.0f)));
}

HitCountTraverseAccumulator::HitCountTraverseAccumulator(const dls::math::vec3f& origin)
{
    m_numberOfHits = 0;
    m_origin = origin;  
}

//Just like the default traverse accumulator, but also tracks the number of
//times a hit has been accumulated. Useful for checking if things have
//passed through a mesh or not
void HitCountTraverseAccumulator::RecordIntersection(const rayCore::Intersection& intersect,
													 const unsigned int& nodeid) {
	if (m_intersection.m_hit==false && intersect.m_hit==true) {
        m_intersection = intersect;
        m_nodeid = nodeid;   
        m_numberOfHits++;
	}
	else if (intersect.m_hit==true) {
        float currentDistance = longueur(m_intersection.m_point - m_origin);
        float newDistance = longueur(intersect.m_point - m_origin);

		if (newDistance<currentDistance) {
            m_intersection = intersect;
            m_nodeid = nodeid;  
        }

        m_numberOfHits++; 
    }
}

void HitCountTraverseAccumulator::Transform(const dls::math::mat4x4f& m)
{
    m_intersection = m_intersection.Transform(m);
    m_origin = dls::math::vec3f(utilityCore::multiply(m, dls::math::vec4f(m_origin, 1.0f)));
}

//Keeps a copy of every single hit that is recorded to this accumulator
void DebugTraverseAccumulator::RecordIntersection(
		const rayCore::Intersection& intersect,
		const unsigned int& nodeid)
{
    m_intersections.push_back(intersect);
    m_nodeids.push_back(nodeid);
}

void DebugTraverseAccumulator::Transform(const dls::math::mat4x4f& m)
{
	auto intersectionCount = m_intersections.size();

	for (unsigned int i=0; i<intersectionCount; i++) {
        m_intersections[i] = m_intersections[i].Transform(m);
    }
}

}
