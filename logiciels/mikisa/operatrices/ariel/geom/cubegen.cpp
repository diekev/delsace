// TAKUA Render: Physically Based Renderer
// Written by Yining Karl Li
//
// File: cube.cpp
// Implements cube.hpp

#include <tbb/tbb.h>
#include "cubegen.hpp"
#include "biblinternes/math/transformation.hh"

namespace geomCore{

//Literally just embed an obj because that's how we roll. Stupid but it works.
void CubeGen::Tesselate(objCore::Obj* o)
{
	dls::math::vec3f* vertices = new dls::math::vec3f[8];
	vertices[0] = dls::math::vec3f(-0.5f, -0.5f,  0.5f);
	vertices[1] = dls::math::vec3f( 0.5f, -0.5f,  0.5f);
	vertices[2] = dls::math::vec3f(-0.5f,  0.5f,  0.5f);
	vertices[3] = dls::math::vec3f( 0.5f,  0.5f,  0.5f);
	vertices[4] = dls::math::vec3f(-0.5f,  0.5f, -0.5f);
	vertices[5] = dls::math::vec3f( 0.5f,  0.5f, -0.5f);
	vertices[6] = dls::math::vec3f(-0.5f, -0.5f, -0.5f);
	vertices[7] = dls::math::vec3f( 0.5f, -0.5f, -0.5f);

	dls::math::vec3f* normals = new dls::math::vec3f[24];
	normals[ 0] = dls::math::vec3f( 0.000000f,  0.000000f,  1.000000f);
	normals[ 1] = dls::math::vec3f( 0.000000f,  0.000000f,  1.000000f);
	normals[ 2] = dls::math::vec3f( 0.000000f,  0.000000f,  1.000000f);
	normals[ 3] = dls::math::vec3f( 0.000000f,  0.000000f,  1.000000f);
	normals[ 4] = dls::math::vec3f( 0.000000f,  1.000000f,  0.000000f);
	normals[ 5] = dls::math::vec3f( 0.000000f,  1.000000f,  0.000000f);
	normals[ 6] = dls::math::vec3f( 0.000000f,  1.000000f,  0.000000f);
	normals[ 7] = dls::math::vec3f( 0.000000f,  1.000000f,  0.000000f);
	normals[ 8] = dls::math::vec3f( 0.000000f,  0.000000f, -1.000000f);
	normals[ 9] = dls::math::vec3f( 0.000000f,  0.000000f, -1.000000f);
	normals[10] = dls::math::vec3f( 0.000000f,  0.000000f, -1.000000f);
	normals[11] = dls::math::vec3f( 0.000000f,  0.000000f, -1.000000f);
	normals[12] = dls::math::vec3f( 0.000000f, -1.000000f,  0.000000f);
	normals[13] = dls::math::vec3f( 0.000000f, -1.000000f,  0.000000f);
	normals[14] = dls::math::vec3f( 0.000000f, -1.000000f,  0.000000f);
	normals[15] = dls::math::vec3f( 0.000000f, -1.000000f,  0.000000f);
	normals[16] = dls::math::vec3f( 1.000000f,  0.000000f,  0.000000f);
	normals[17] = dls::math::vec3f( 1.000000f,  0.000000f,  0.000000f);
	normals[18] = dls::math::vec3f( 1.000000f,  0.000000f,  0.000000f);
	normals[19] = dls::math::vec3f( 1.000000f,  0.000000f,  0.000000f);
	normals[20] = dls::math::vec3f(-1.000000f,  0.000000f,  0.000000f);
	normals[21] = dls::math::vec3f(-1.000000f,  0.000000f,  0.000000f);
	normals[22] = dls::math::vec3f(-1.000000f,  0.000000f,  0.000000f);
	normals[23] = dls::math::vec3f(-1.000000f,  0.000000f,  0.000000f);

	dls::math::vec2f* uvs = new dls::math::vec2f[4];
	uvs[0] = dls::math::vec2f( 0.0f, 0.0f);
	uvs[1] = dls::math::vec2f( 1.0f, 0.0f);
	uvs[2] = dls::math::vec2f( 1.0f, 1.0f);
	uvs[3] = dls::math::vec2f( 0.0f, 1.0f);

	dls::math::vec4i* polyVertexIndices = new dls::math::vec4i[6];
	polyVertexIndices[0] = dls::math::vec4i(1,2,4,3);
	polyVertexIndices[1] = dls::math::vec4i(3,4,6,5);
	polyVertexIndices[2] = dls::math::vec4i(5,6,8,7);
	polyVertexIndices[3] = dls::math::vec4i(7,8,2,1);
	polyVertexIndices[4] = dls::math::vec4i(2,8,6,4);
	polyVertexIndices[5] = dls::math::vec4i(7,1,3,5);

	dls::math::vec4i* polyNormalIndices = new dls::math::vec4i[6];
	polyNormalIndices[0] = dls::math::vec4i(1,2,3,4);
	polyNormalIndices[1] = dls::math::vec4i(5,6,7,8);
	polyNormalIndices[2] = dls::math::vec4i(9,10,11,12);
	polyNormalIndices[3] = dls::math::vec4i(13,14,15,16);
	polyNormalIndices[4] = dls::math::vec4i(17,18,19,20);
	polyNormalIndices[5] = dls::math::vec4i(21,22,23,24);

	dls::math::vec4i* polyUVIndices = new dls::math::vec4i[6];
	polyUVIndices[0] = dls::math::vec4i(1,2,3,4);
	polyUVIndices[1] = dls::math::vec4i(1,2,3,4);
	polyUVIndices[2] = dls::math::vec4i(3,4,1,2);
	polyUVIndices[3] = dls::math::vec4i(1,2,3,4);
	polyUVIndices[4] = dls::math::vec4i(1,2,3,4);
	polyUVIndices[5] = dls::math::vec4i(1,2,3,4);

	o->m_numberOfVertices = 8;
	o->m_vertices = vertices;
	o->m_numberOfNormals = 24;
	o->m_normals = normals;
	o->m_numberOfUVs = 4;
	o->m_uvs = uvs;
	o->m_numberOfPolys = 6;
	o->m_polyVertexIndices = polyVertexIndices;
	o->m_polyNormalIndices = polyNormalIndices;
	o->m_polyUVIndices = polyUVIndices;
}

void CubeGen::Tesselate(
		objCore::Obj* o,
		const dls::math::vec3f& lowerCorner,
		const dls::math::vec3f& upperCorner)
{
	auto scale = upperCorner-lowerCorner;
	auto center = (upperCorner+lowerCorner)/2.0f;
	auto xform = math::construit_transformation(center, dls::math::vec3f(0.0f, 0.0f, 0.0f), scale);
	auto transform = math::matf_depuis_matd(xform.matrice());

	Tesselate(o);
	unsigned int numberOfPoints = o->m_numberOfVertices;

	tbb::parallel_for(tbb::blocked_range<unsigned int>(0,numberOfPoints),
					  [&](const tbb::blocked_range<unsigned int>& r)
	{
		for (unsigned int i=r.begin(); i!=r.end(); ++i) {
			o->m_vertices[i] = dls::math::vec3f(transform * dls::math::vec4f(o->m_vertices[i], 1.0f));
		}
	});
}

}
