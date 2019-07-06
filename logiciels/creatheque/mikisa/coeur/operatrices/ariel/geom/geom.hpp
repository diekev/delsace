// TAKUA Render: Physically Based Renderer
// Written by Yining Karl Li
//
// File: geom.hpp
// Classes for geom frame operations and stuff

#ifndef GEOM_HPP
#define GEOM_HPP

#ifdef __CUDACC__
#define HOST __host__
#define DEVICE __device__
#define SHARED __shared__
#else
#define HOST
#define DEVICE
#define SHARED
#endif

#include "../spatial/bvh.hpp"
#include "../ray/ray.hpp"

enum GeomType{MESH, VOLUME, CURVE, ANIMMESH, NONE};

namespace geomCore {

//====================================
// Class Declarations
//====================================

class Geom;
class GeomTransform;
class GeomInterface;

class Geom {
public:
	Geom();
	Geom(GeomInterface* geom);
	~Geom() = default;

	Geom(Geom const &) = default;
	Geom &operator=(Geom const &) = default;

	void SetContents(GeomInterface* geom);
	void Intersect(const rayCore::Ray& r,
				   spaceCore::TraverseAccumulator& result);
	GeomType GetType();

	GeomInterface*          m_geom{};
	unsigned int            m_id{};
};

class GeomInterface {
public:
	GeomInterface() = default;
	virtual ~GeomInterface() = default;

	virtual void Intersect(const rayCore::Ray& r,
						   spaceCore::TraverseAccumulator& result) = 0;
	virtual GeomType GetType() = 0;
	virtual unsigned int GetID() = 0;

	virtual bool GetTransforms(const float& frame, dls::math::mat4x4f& transform,
							   dls::math::mat4x4f& inversetransform) = 0;
	virtual bool IsDynamic() = 0;
	virtual bool IsInFrame(const float& frame) = 0;

	virtual spaceCore::Aabb GetAabb(const float& frame) = 0;
};

class GeomTransform {
public:
	GeomTransform();
	GeomTransform(const dls::math::vec3f& t, const dls::math::vec3f& r, const dls::math::vec3f& s);
	~GeomTransform() = default;

	void SetContents(const dls::math::vec3f& t, const dls::math::vec3f& r, const dls::math::vec3f& s);

	dls::math::vec3f                   m_translation{};
	dls::math::vec3f                   m_rotation{};
	dls::math::vec3f                   m_scale{};
	unsigned int                m_id{};
};

}

#endif
