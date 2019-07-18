// TAKUA Render: Physically Based Renderer
// Written by Yining Karl Li
//
// File: sphere.cpp
// Implements sphere.hpp

#include <tbb/tbb.h>
#include "spheregen.hpp"
#include "biblinternes/math/transformation.hh"
#include "biblinternes/outils/constantes.h"

namespace geomCore{

//Constructor with options for presets
SphereGen::SphereGen(const unsigned int& subdivCount)
	: m_subdivs(static_cast<int>(subdivCount))
{
}

void SphereGen::Tesselate(objCore::Obj* o, const dls::math::vec3f& center, const float& radius)
{
	auto scale = dls::math::vec3f(radius*2.0f);

	auto xform = math::construit_transformation(center, dls::math::vec3f(0.0f, 0.0f, 0.0f), scale);
	auto transform = math::matf_depuis_matd(xform.matrice());

	Tesselate(o);
	unsigned int numberOfPoints = o->m_numberOfVertices;

	tbb::parallel_for(tbb::blocked_range<unsigned int>(0,numberOfPoints),
					  [&](const tbb::blocked_range<unsigned int>& r)
	{
		for (unsigned int i=r.begin(); i!=r.end(); ++i) {
			o->m_vertices[i] = dls::math::vec3f(transform*dls::math::vec4f(o->m_vertices[i], 1.0f));
		}
	});
}

/*Returns sphere mesh with specified subdiv counts. 
Axis and height must have a minimum of 3 subdivs and 
tesselate() will default to 3 if subdiv count is below 3.*/
//Yes, this function is total spaghetti and hacked together in many places. Will fix later. Maybe.
void SphereGen::Tesselate(objCore::Obj* o)
{
	auto a = static_cast<size_t>(std::max(m_subdivs, 3));
	auto h = static_cast<size_t>(std::max(m_subdivs, 3));
	auto vertCount = (a * (h - 1)) + 2;
	auto faceCount = a * h;
	auto vertices = new dls::math::vec3f[vertCount];
	auto normals = new dls::math::vec3f[vertCount];
	auto uvs = new dls::math::vec2f[vertCount + 3ul * h];
	auto polyVertexIndices = new dls::math::vec4i[faceCount];
	auto polyNormalIndices = new dls::math::vec4i[faceCount];
	auto polyUVIndices = new dls::math::vec4i[faceCount];

	//generate vertices and write into vertex array
	for (auto x = 1ul; x < h; x++) {
		for (auto y = 0ul; y < a; y++) {
			auto angle1 = float(x) * 1.0f / float(a);
			auto angle2 = float(y) * 1.0f / float(h);
			auto i = ((x - 1) * a) + y;
			vertices[i] = GetPointOnSphereByAngles(angle1, angle2);
		}
	}

	vertices[vertCount-2] = dls::math::vec3f(0.0f, -0.5f, 0.0f);
	vertices[vertCount-1] = dls::math::vec3f(0.0f,  0.5f, 0.0f);
	//generate normals and uvs on sides of sphere
	auto uvoffset = 0.0f;
	for (auto i = 0ul; i < vertCount; i++) {
		normals[i] = normalise(vertices[i]);
		dls::math::vec2f uv;
		uv.x = 0.5f - (std::atan2(normals[i].z, normals[i].x) / constantes<float>::TAU) ;
		uv.y = 0.5f - (2.0f * (std::asin(normals[i].y) / constantes<float>::TAU));

		if (i == (a / 2)) {
			uvoffset = uv.x;
		}

		uvs[i] = uv;
	}

	//offset uvs
	for (auto i = 0ul; i < vertCount; i++) {
		uvs[i].x = uvs[i].x - uvoffset;
	}

	//generate wraparound uvs
	for (auto i=0ul; i < h; i++) {
		dls::math::vec2f uv;
		uv.x = 1;
		auto ih = i*h;
		if (ih>0) {
			ih = ih-1;
		}
		uv.y = uvs[ih].y;
		uvs[vertCount + i] = uv;
	}

	//generate faces for sphere sides
	for (auto x = 1ul; x < h - 1; x++) {
		for (auto y = 0ul; y < a; y++) {
			auto i1 = ((x-1)*a) + y;
			auto i2 = ((x-1)*a) + (y+1);
			auto i3 = ((x)*a) + (y+1);
			auto i4 = ((x)*a) + y;
			//attach vertices at wraparound point

			if (y==a-1) {
				i2 = ((x-1)*a) + (0);
				i3 = ((x)*a) + (0);
			}

			auto indices = dls::math::vec4i(
						static_cast<int>(i1 + 1),
						static_cast<int>(i2 + 1),
						static_cast<int>(i3 + 1),
						static_cast<int>(i4 + 1));

			polyVertexIndices[i1] = indices;
			polyNormalIndices[i1] = indices;

			//fix uvs at uv wraparound point
			if (y==a/2) {
				indices[0] = static_cast<int>(vertCount + x + 1);
				indices[3] = static_cast<int>(vertCount + x + 2);
			}

			polyUVIndices[i1] = indices;
		}
	}

	//generate faces and uvs for top pole
	for (auto x = 0ul; x < h; x++) {
		auto indices = polyVertexIndices[x];
		indices[3] = 0;
		indices[2] = indices[0];
		indices[0] = static_cast<int>(vertCount);

		polyVertexIndices[faceCount-(a*2)+x] = indices;
		polyNormalIndices[faceCount-(a*2)+x] = indices;

		indices = polyUVIndices[x];
		indices[3] = 0;
		indices[2] = indices[0];
		indices[0] = static_cast<int>(vertCount+h+x+1);
		polyUVIndices[faceCount-(a*2)+x] = indices;

		auto uvindex = vertCount+h+x;
		dls::math::vec2f uv;
		uv.y = 0.0f;
		uv.x = uvs[static_cast<size_t>(indices[2])].x + (0.5f / static_cast<float>(h));

		if (x == (h / 2)) {
			uv.x = uv.x - (1.0f / static_cast<float>(h));
		}

		uvs[uvindex] = uv;
	}

	//generate faces and uvs for bottom pole
	for (auto x = 0ul; x < h; x++) {
		auto index = (h-1)*(a-2)-2+x;
		auto indices = polyVertexIndices[index];
		indices[1] = indices[2];
		indices[0] = indices[3];
		indices[3] = 0;
		indices[2] = static_cast<int>(vertCount-1);

		polyVertexIndices[faceCount-a+x] = indices;
		polyNormalIndices[faceCount-a+x] = indices;
		indices = polyUVIndices[vertCount-(h*2)-2+x];

		dls::math::vec4i i2;
		i2[0] = indices[3];
		i2[1] = indices[2];
		i2[2] = static_cast<int>(vertCount+(h*2)+x+1);
		i2[3] = 0;
		polyUVIndices[faceCount-a+x] = i2;

		dls::math::vec2f uv;
		uv.y=1;
		uv.x=uvs[static_cast<size_t>(indices[0])].x + (0.5f / static_cast<float>(h));

		if (x == (h / 2)) {
			uv.x = uv.x - (1.0f / static_cast<float>(h));
		}

		uvs[vertCount+(2*h)+x] = uv;
	}

	//flip uvs
	for (auto i = 0ul; i < vertCount + (3 * h); i++) {
		uvs[i].x = 1.0f-uvs[i].x;
	}

	o->m_numberOfVertices = static_cast<unsigned int>(vertCount);
	o->m_vertices = vertices;
	o->m_numberOfNormals = static_cast<unsigned int>(vertCount);
	o->m_normals = normals;
	o->m_numberOfUVs = static_cast<unsigned int>(vertCount)+static_cast<unsigned int>(3*h);
	o->m_uvs = uvs;
	o->m_numberOfPolys = static_cast<unsigned int>(faceCount);
	o->m_polyVertexIndices = polyVertexIndices;
	o->m_polyNormalIndices = polyNormalIndices;
	o->m_polyUVIndices = polyUVIndices;
}

dls::math::vec3f SphereGen::GetPointOnSphereByAngles(const float& angle1, const float& angle2)
{
	auto x = std::sin(constantes<float>::PI*angle1) * std::cos(constantes<float>::TAU*angle2);
	auto y = std::sin(constantes<float>::PI*angle1) * std::sin(constantes<float>::TAU*angle2);
	auto z = std::cos(constantes<float>::PI*angle1);
	return normalise(dls::math::vec3f(x,z,y))/2.0f;
}

}
