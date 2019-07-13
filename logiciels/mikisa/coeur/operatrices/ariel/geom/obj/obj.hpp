// ObjCore4.0: An (improved) obj mesh wrangling library. Part of TAKUA Render.
// Written by Yining Karl Li
//
// File: obj.hpp
// A simple obj class. This library superscedes ObjCore 1.x/2.x. 

#ifndef OBJ_HPP
#define OBJ_HPP

#ifdef __CUDACC__
#define HOST __host__
#define DEVICE __device__
#else
#define HOST
#define DEVICE
#endif

#include <sstream>
#include <fstream>
#include <iostream>
#include "../../utilities/utilities.h"
#include "../../ray/ray.hpp"
#include "../../spatial/aabb.hpp"

namespace objCore {

//====================================
// Struct Declarations
//====================================

struct Point {
	dls::math::vec3f m_position{};
	dls::math::vec3f m_normal{};
	dls::math::vec2f m_uv{};

	Point() = default;

	Point(const dls::math::vec3f& p, const dls::math::vec3f& n, const dls::math::vec2f& u)
		: m_position(p)
		, m_normal(n)
		, m_uv(u)
	{}
};

struct Poly {
	Point m_vertex0{};
	Point m_vertex1{};
	Point m_vertex2{};
	Point m_vertex3{};
	unsigned int m_id{};

	Poly() = default;

	//Builds a quad struct with the given data
	Poly(const Point& v0, const Point& v1, const Point& v2, const Point& v3)
		: m_vertex0(v0)
		, m_vertex1(v1)
		, m_vertex2(v2)
		, m_vertex3(v3)
	{}

	Poly(const Point& v0, const Point& v1, const Point& v2, const Point& v3,
		 const unsigned int& id)
		: m_vertex0(v0)
		, m_vertex1(v1)
		, m_vertex2(v2)
		, m_vertex3(v3)
		, m_id(id)
	{
	}

	//Builds a triangle struct with the given data
	Poly(const Point& v0, const Point& v1, const Point& v2)
		: m_vertex0(v0)
		, m_vertex1(v1)
		, m_vertex2(v2)
		, m_vertex3(v0)//in a triangle, the fourth vertex is the same as the first
	{}

	Poly(const Point& v0, const Point& v1, const Point& v2, const unsigned int& id)
		: m_vertex0(v0)
		, m_vertex1(v1)
		, m_vertex2(v2)
		, m_vertex3(v0)
		, m_id(id)
	{}
};

//====================================
// Class Declarations
//====================================

class Obj {
public:
	Obj();
	Obj(const dls::chaine& filename);
	~Obj();

	Obj(Obj const &) = default;
	Obj &operator=(Obj const &) = default;

	void BakeTransform(const dls::math::mat4x4f& transform);

	bool ReadObj(const dls::chaine& filename);
	bool WriteObj(const dls::chaine& filename);

	Poly GetPoly(const unsigned int& polyIndex);
	static Poly TransformPoly(const Poly& p, const dls::math::mat4x4f& m);
	static Point TransformPoint(const Point& p, const dls::math::mat4x4f& m);

	//Standard interface functions for accel. structures
	rayCore::Intersection IntersectElement(const unsigned int& primID,
										   const rayCore::Ray& r);
	spaceCore::Aabb GetElementAabb(const unsigned int& primID);
	unsigned int GetNumberOfElements();

	static inline rayCore::Intersection RayTriangleTest(const dls::math::vec3f& v0,
														const dls::math::vec3f& v1,
														const dls::math::vec3f& v2,
														const dls::math::vec3f& n0,
														const dls::math::vec3f& n1,
														const dls::math::vec3f& n2,
														const dls::math::vec2f& u0,
														const dls::math::vec2f& u1,
														const dls::math::vec2f& u2,
														const rayCore::Ray& r);

	unsigned int    m_numberOfVertices{};
	dls::math::vec3f*      m_vertices{};
	unsigned int    m_numberOfNormals{};
	dls::math::vec3f*      m_normals{};
	unsigned int    m_numberOfUVs{};
	dls::math::vec2f*      m_uvs{};
	unsigned int    m_numberOfPolys{};
	dls::math::vec4i*     m_polyVertexIndices{};
	dls::math::vec4i*     m_polyNormalIndices{};
	dls::math::vec4i*     m_polyUVIndices{};
	unsigned int    m_id{};
	bool            m_keep{};

private:
	void PrereadObj(const dls::chaine& filename);
	rayCore::Intersection TriangleTest(const unsigned int& polyIndex,
									   const rayCore::Ray& r,
									   const bool& checkQuad);
	rayCore::Intersection QuadTest(const unsigned int& polyIndex,
								   const rayCore::Ray& r);
};

class InterpolatedObj {
public:
	InterpolatedObj();
	InterpolatedObj(objCore::Obj* obj0, objCore::Obj* obj1);
	~InterpolatedObj() = default;

	InterpolatedObj(InterpolatedObj const &) = default;
	InterpolatedObj &operator=(InterpolatedObj const &) = default;

	Poly GetPoly(const unsigned int& polyIndex, const float& interpolation);

	//Standard interface functions for accel. structures
	rayCore::Intersection IntersectElement(const unsigned int& primID,
										   const rayCore::Ray& r);
	spaceCore::Aabb GetElementAabb(const unsigned int& primID);
	unsigned int GetNumberOfElements();

	objCore::Obj *m_obj0 = nullptr;
	objCore::Obj *m_obj1 = nullptr;

private:
	rayCore::Intersection TriangleTest(const unsigned int& polyIndex,
									   const rayCore::Ray& r,
									   const bool& checkQuad);
	rayCore::Intersection QuadTest(const unsigned int& polyIndex,
								   const rayCore::Ray& r);
};
}

#endif
