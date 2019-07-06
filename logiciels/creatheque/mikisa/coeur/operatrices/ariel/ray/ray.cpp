// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: ray.cpp. Adapted from Takua Render.
// Implements ray.hpp

#include "ray.hpp"

namespace rayCore {

//====================================
// Ray Class
//====================================

Ray::Ray() {
	SetContents(dls::math::vec3f(0.0f), dls::math::vec3f(0.0f, 1.0f, 0.0f), 0.0f, 0);
}

Ray::Ray(const dls::math::vec3f& origin, const dls::math::vec3f& direction, const float& frame,
					 const unsigned int& trackingID) {
    SetContents(origin, direction, frame, trackingID);
}

Ray::Ray(const dls::math::vec3f& origin, const dls::math::vec3f& direction, const float& frame) {
    SetContents(origin, direction, frame, 0);
}

void Ray::SetContents(const dls::math::vec3f& origin, const dls::math::vec3f& direction, 
								  const float& frame, const unsigned int& trackingID) {
    m_origin = origin;
    m_direction = direction/longueur(direction);
    m_trackingID = trackingID;
    m_frame = frame;
}

dls::math::vec3f Ray::GetPointAlongRay(const float& distance) const{
    return m_origin + distance * m_direction;
}

Ray Ray::Transform(const dls::math::mat4x4f& m) const{
    dls::math::vec3f transformedOrigin = dls::math::vec3f(utilityCore::multiply(m, dls::math::vec4f(m_origin, 1.0f)));
    dls::math::vec3f transformedPoint = dls::math::vec3f(utilityCore::multiply(m, dls::math::vec4f(m_origin+m_direction,
                                                                              1.0f)));
    dls::math::vec3f transformedDirection = transformedPoint - transformedOrigin;
    return Ray(transformedOrigin, transformedDirection, m_frame, m_trackingID);
}

//====================================
// Intersection Class
//====================================

Intersection::Intersection() {
    SetContents(false, dls::math::vec3f(0.0f), dls::math::vec3f(1.0f,0.0f,0.0f), dls::math::vec2f(0.0f), 0, 0);
}

Intersection::Intersection(const bool& hit, const dls::math::vec3f& point, 
                                       const dls::math::vec3f& normal, const dls::math::vec2f& uv, 
									   const unsigned int& objectID, const unsigned int& primID) {
    SetContents(hit, point, normal, uv, objectID, primID);
}

Intersection::~Intersection() {

}

void Intersection::SetContents(const bool& hit, const dls::math::vec3f& point, 
                                           const dls::math::vec3f& normal, const dls::math::vec2f& uv, 
                                           const unsigned int& objectID, 
										   const unsigned int& primID) {
    m_point = point;
    m_hit = hit;
    m_normal = normal;
    m_uv = uv;
    m_objectID = objectID;
    m_primID = primID;
}


Intersection Intersection::CompareClosestAgainst(const Intersection& b, 
															 const dls::math::vec3f& point) {
	if (m_hit && !b.m_hit) {
        return *this;
	}else if (!m_hit && b.m_hit) {
        return b;
	}else {
        float distanceA = longueur(m_point - point);
        float distanceB = longueur(b.m_point - point);
		if (distanceA<=distanceB) {
            return *this;
		}else {
            return b;
        }
    }
}

Intersection Intersection::Transform(const dls::math::mat4x4f& m) {
    dls::math::vec3f transformedPoint = dls::math::vec3f(m*dls::math::vec4f(m_point, 1.0f));
    dls::math::vec3f transformedNormal = dls::math::vec3f(m*dls::math::vec4f(m_normal, 0.0f));
    return Intersection(m_hit, transformedPoint, transformedNormal, m_uv, m_objectID, m_primID); 
}
}
