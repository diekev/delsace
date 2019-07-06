// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: levelset.cpp
// Implements levelset.hpp

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <openvdb/tools/ParticlesToLevelSet.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToSpheres.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <openvdb/util/NullInterrupter.h>
#pragma GCC diagnostic pop

#include "levelset.hpp"

namespace fluidCore{

LevelSet::LevelSet()
{
	openvdb::initialize();
	m_vdbgrid = openvdb::FloatGrid::create(0.0f);
}

LevelSet::~LevelSet()
{
	m_vdbgrid->clear();
	m_vdbgrid.reset();
}

LevelSet::LevelSet(
		objCore::Obj* mesh)
{
	LevelSetFromMesh(mesh, dls::math::mat4x4f());
}

LevelSet::LevelSet(
		objCore::Obj* mesh,
		const dls::math::mat4x4f& m)
{
	LevelSetFromMesh(mesh, m);
}

LevelSet::LevelSet(
		objCore::InterpolatedObj* animmesh,
		const float& interpolation,
		const dls::math::mat4x4f& m)
{
	LevelSetFromAnimMesh(animmesh, interpolation, m);
}

void LevelSet::LevelSetFromAnimMesh(objCore::InterpolatedObj* animmesh, const float& interpolation, 
									const dls::math::mat4x4f& m)
{
	auto transform = openvdb::math::Transform::createLinearTransform(0.25);

	//grab object pointers
	objCore::Obj* o0 = animmesh->m_obj0;
	objCore::Obj* o1 = animmesh->m_obj1;

	//copy vertices into vdb format
	std::vector<openvdb::Vec3s> vdbpoints;
	for (unsigned int i=0; i<o0->m_numberOfVertices; i++) {
		dls::math::vec3f vertex = o0->m_vertices[i] * (1.0f-interpolation) +
				o1->m_vertices[i] * interpolation;
		vertex = dls::math::vec3f(m * dls::math::vec4f(vertex, 1.0f));
		openvdb::Vec3s vdbvertex(vertex.x, vertex.y, vertex.z);
		vdbpoints.push_back(transform->worldToIndex(vdbvertex));
	}

	//copy faces into vdb format
	std::vector<openvdb::Vec4I> vdbpolys;
	for (unsigned int i=0; i<o0->m_numberOfPolys; i++) {
		dls::math::vec4i poly = o0->m_polyVertexIndices[i];
		openvdb::Vec4I vdbpoly(static_cast<unsigned int>(poly[0])-1, static_cast<unsigned int>(poly[1])-1, static_cast<unsigned int>(poly[2])-1, static_cast<unsigned int>(poly[3])-1);
		if (poly[0]==poly[3] || poly[3]<0) {
			vdbpoly[3] = openvdb::util::INVALID_IDX;
		}
		vdbpolys.push_back(vdbpoly);
	}

	//call vdb tools for creating level set
	m_vdbgrid = openvdb::tools::meshToLevelSet<openvdb::FloatGrid>(
				*transform, vdbpoints, vdbpolys);

	//cleanup
	vdbpoints.clear();
	vdbpolys.clear();
}

void LevelSet::LevelSetFromMesh(objCore::Obj* mesh, const dls::math::mat4x4f& m)
{
	auto transform = openvdb::math::Transform::createLinearTransform(0.25);

	//copy vertices into vdb format
	std::vector<openvdb::Vec3s> vdbpoints;
	for (unsigned int i=0; i<mesh->m_numberOfVertices; i++) {
		dls::math::vec3f vertex = mesh->m_vertices[i];
		vertex = dls::math::vec3f(m * dls::math::vec4f(vertex, 1.0f));
		openvdb::Vec3s vdbvertex(vertex.x, vertex.y, vertex.z);
		vdbpoints.push_back(transform->worldToIndex(vdbvertex));
	}

	//copy faces into vdb format
	std::vector<openvdb::Vec4I> vdbpolys;
	for (unsigned int i=0; i<mesh->m_numberOfPolys; i++) {
		dls::math::vec4i poly = mesh->m_polyVertexIndices[i];
		openvdb::Vec4I vdbpoly(static_cast<unsigned int>(poly[0])-1, static_cast<unsigned int>(poly[1])-1, static_cast<unsigned int>(poly[2])-1, static_cast<unsigned int>(poly[3])-1);
		if (poly[0]==poly[3] || poly[3]==0) {
			vdbpoly[3] = openvdb::util::INVALID_IDX;
		}
		vdbpolys.push_back(vdbpoly);
	}

	//call vdb tools for creating level set
	m_vdbgrid = openvdb::tools::meshToLevelSet<openvdb::FloatGrid>(
				*transform, vdbpoints, vdbpolys);

	//cleanup
	vdbpoints.clear();
	vdbpolys.clear();
}

LevelSet::LevelSet(std::vector<Particle*>& particles, float maxdimension)
{
	m_vdbgrid = openvdb::createLevelSet<openvdb::FloatGrid>();
	openvdb::tools::ParticlesToLevelSet<openvdb::FloatGrid> raster(*m_vdbgrid);
	raster.setGrainSize(1);
	raster.setRmin(0.01);

	ParticleList plist(particles, maxdimension);
	// raster.rasterizeSpheres(plist);
	raster.rasterizeTrails(plist);
	raster.finalize();
}

void LevelSet::WriteObjToFile(std::string filename)
{
	openvdb::tools::VolumeToMesh vdbmesher(0, 0.05);
	vdbmesher(*GetVDBGrid());

	//get all mesh points and dump to dls::math::vec3f std::vector
	auto vdbPointsCount = vdbmesher.pointListSize();
	openvdb::tools::PointList& vdbPoints = vdbmesher.pointList();

	std::vector<dls::math::vec3f> points;
	points.reserve(vdbPointsCount);
	for (auto i = 0l; i < static_cast<long>(vdbPointsCount); i++) {
		dls::math::vec3f v(vdbPoints[i][0], vdbPoints[i][1], vdbPoints[i][2]);
		points.push_back(v);
	}

	//get all mesh faces and dump to dls::math::vec4f std::vector
	auto vdbFacesCount = vdbmesher.polygonPoolListSize();
	auto &vdbFaces = vdbmesher.polygonPoolList();

	auto facesCount = 0ul;
	for (auto i = 0l; i < static_cast<long>(vdbFacesCount); i++) {
		facesCount = facesCount + vdbFaces[i].numQuads() + vdbFaces[i].numTriangles();
	}

	std::vector<dls::math::vec4i> faces;
	faces.reserve(facesCount);

	for (unsigned int i=0; i<vdbFacesCount; i++) {
		auto count = vdbFaces[i].numQuads();
		for (unsigned int j=0; j<count; j++) {
			openvdb::Vec4I vdbface = vdbFaces[i].quad(j);
			dls::math::vec4i f(
						static_cast<int>(vdbface.x()) + 1,
						static_cast<int>(vdbface.y()) + 1,
						static_cast<int>(vdbface.z()) + 1,
						static_cast<int>(vdbface.w()) + 1);
			faces.push_back(f);
		}
		count = vdbFaces[i].numTriangles();
		for (unsigned int j=0; j<count; j++) {
			openvdb::Vec3I vdbface = vdbFaces[i].triangle(j);
			dls::math::vec4i f(static_cast<int>(vdbface[0])+1, static_cast<int>(vdbface[1])+1, static_cast<int>(vdbface[2])+1, 0);
			faces.push_back(f);
		}
	}

	//pack points and faces into objcontainer and write
	objCore::Obj* mesh = new objCore::Obj();
	mesh->m_numberOfVertices = static_cast<unsigned int>(points.size());
	mesh->m_vertices = &points[0];
	mesh->m_numberOfNormals = 0;
	mesh->m_normals = nullptr;
	mesh->m_numberOfUVs = 0;
	mesh->m_uvs = nullptr;
	mesh->m_numberOfPolys = static_cast<unsigned int>(faces.size());
	mesh->m_polyVertexIndices = &faces[0];
	mesh->m_polyNormalIndices = nullptr;
	mesh->m_polyUVIndices = nullptr;

	mesh->m_keep = true;

	mesh->WriteObj(filename);

	delete mesh;
}

void LevelSet::ProjectPointsToSurface(std::vector<Particle*>& particles, const float& pscale)
{
	auto pointsCount = particles.size();
	std::vector<openvdb::Vec3R> vdbpoints(pointsCount);
	std::vector<float> distances;
	distances.reserve(pointsCount);

	for (unsigned int i=0; i<pointsCount; i++) {
		dls::math::vec3f p = particles[i]->m_p * pscale;
		openvdb::Vec3s vdbvertex(p.x, p.y, p.z);
		vdbpoints[i] = vdbvertex;
	}
	openvdb::util::NullInterrupter n;
	auto csp = openvdb::tools::ClosestSurfacePoint<openvdb::FloatGrid>::create(*m_vdbgrid, 0.0f, &n);

	//	openvdb::tools::ClosestSurfacePoint<openvdb::FloatGrid> csp;
	//    csp.initialize(*m_vdbgrid, 0.0f, &n);

	csp->searchAndReplace(vdbpoints, distances);

	for (unsigned int i=0; i<pointsCount; i++) {
		vdbpoints[i] = vdbpoints[i]/pscale;
		particles[i]->m_p = dls::math::vec3f(static_cast<float>(vdbpoints[i][0]), static_cast<float>(vdbpoints[i][1]), static_cast<float>(vdbpoints[i][2]));
	}
}

void LevelSet::WriteVDBGridToFile(std::string filename)
{
	openvdb::io::File file(filename);
	openvdb::GridPtrVec grids;
	grids.push_back(m_vdbgrid);
	file.write(grids);
	file.close();
}

float LevelSet::GetInterpolatedCell(const dls::math::vec3f& index)
{
	return GetInterpolatedCell(index.x, index.y, index.z);
}

float LevelSet::GetInterpolatedCell(const float& x, const float& y, const float& z)
{
	float value;
	m_getInterpolatedCellLock.lock();
	{
		openvdb::Vec3f p(x,y,z);
		openvdb::tools::GridSampler<openvdb::FloatTree, openvdb::tools::BoxSampler> interpolator(
					m_vdbgrid->constTree(),
					m_vdbgrid->transform());
		value = interpolator.wsSample(p);
	}
	m_getInterpolatedCellLock.unlock();
	return value;
}

float LevelSet::GetCell(const dls::math::vec3f& index)
{
	return GetCell(static_cast<int>(index.x), static_cast<int>(index.y), static_cast<int>(index.z));
}

float LevelSet::GetCell(const int& x, const int& y, const int& z)
{
	openvdb::Coord coord = openvdb::Coord(x,y,z);
	openvdb::FloatGrid::Accessor accessor = m_vdbgrid->getAccessor();
	float cell = accessor.getValue(coord);
	return cell;
}

void LevelSet::SetCell(const dls::math::vec3f& index, const float& value)
{
	SetCell(static_cast<int>(index.x), static_cast<int>(index.y), static_cast<int>(index.z), value);
}

void LevelSet::SetCell(const int& x, const int& y, const int& z, const float& value)
{
	m_setCellLock.lock();
	{
		openvdb::Coord coord = openvdb::Coord(x,y,z);
		openvdb::FloatGrid::Accessor accessor = m_vdbgrid->getAccessor();
		accessor.setValue(coord, value);
	}
	m_setCellLock.unlock();
}

openvdb::FloatGrid::Ptr& LevelSet::GetVDBGrid()
{
	return m_vdbgrid;
}

void LevelSet::Merge(LevelSet& ls)
{
	openvdb::FloatGrid::Ptr objectSDF = ls.GetVDBGrid()->deepCopy();
	openvdb::tools::csgUnion(*m_vdbgrid, *objectSDF);
	objectSDF->clear();
}

void LevelSet::Copy(LevelSet& ls)
{
	m_vdbgrid = ls.GetVDBGrid()->deepCopy();
}

}
