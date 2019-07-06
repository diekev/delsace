// ObjCore4.0: An (improved) obj mesh wrangling library. Part of TAKUA Render.
// Written by Yining Karl Li
//
// File: obj.cpp
// Implements obj.hpp

#include <tbb/tbb.h>
#include <vector>
#include "obj.hpp"

namespace objCore {

//====================================
// Obj Class
//====================================

Obj::Obj() {
	m_numberOfVertices = 0;
	m_numberOfNormals = 0;
	m_numberOfUVs = 0;
	m_numberOfPolys = 0;
	m_vertices = nullptr;
	m_normals = nullptr;
	m_uvs = nullptr;
	m_polyVertexIndices = nullptr;
	m_polyNormalIndices = nullptr;
	m_polyUVIndices = nullptr;
	m_keep = false;
}

Obj::Obj(const std::string& filename) {
	ReadObj(filename);
	m_keep = false;
}

Obj::~Obj() {
	if (!m_keep) { //if keep flag is set true, contents won't be destroyed. Used for memory transfer.
		delete [] m_vertices;
		delete [] m_polyVertexIndices;

		if (m_numberOfNormals>0) {
			delete [] m_normals;
			delete [] m_polyNormalIndices;
		}

		if (m_numberOfUVs) {
			delete [] m_uvs;
			delete [] m_polyUVIndices;
		}
	}
}

void Obj::BakeTransform(const dls::math::mat4x4f& transform)
{
	tbb::parallel_for(tbb::blocked_range<unsigned int>(0,m_numberOfVertices),
					  [&](const tbb::blocked_range<unsigned int>& r)
	{
		for (unsigned int i=r.begin(); i!=r.end(); ++i) {
			m_vertices[i] = dls::math::vec3f(transform * dls::math::vec4f(m_vertices[i], 1.0f));
		}
	});

	tbb::parallel_for(tbb::blocked_range<unsigned int>(0,m_numberOfNormals),
					  [&](const tbb::blocked_range<unsigned int>& r)
	{
		for (unsigned int i=r.begin(); i!=r.end(); ++i) {
			m_normals[i] = normalise(dls::math::vec3f(transform * dls::math::vec4f(m_normals[i], 0.0f)));
		}
	});
}

bool Obj::ReadObj(const std::string& filename)
{
	PrereadObj(filename);

	m_vertices = new dls::math::vec3f[m_numberOfVertices];
	m_normals = new dls::math::vec3f[m_numberOfNormals];
	m_uvs = new dls::math::vec2f[m_numberOfUVs];
	m_polyVertexIndices = new dls::math::vec4i[m_numberOfPolys];
	m_polyNormalIndices = new dls::math::vec4i[m_numberOfPolys];
	m_polyUVIndices = new dls::math::vec4i[m_numberOfPolys];

	unsigned int currentVertex = 0;
	unsigned int currentNormal = 0;
	unsigned int currentUV = 0;
	unsigned int currentFace = 0;

	//Load loop
	std::ifstream fp_in;
	auto fname = filename.c_str();
	fp_in.open(fname);

	if (fp_in.is_open()) {
		while (fp_in.good()) {
			std::string line;
			getline(fp_in, line);

			if (!line.empty()) {
				std::vector<std::string> tokens = utilityCore::tokenizeString(line, " ");

				if (tokens.size()>1) {
					if (tokens[0].compare("v")==0) {
						auto x = static_cast<float>(atof(tokens[1].c_str()));
						auto y = static_cast<float>(atof(tokens[2].c_str()));
						auto z = static_cast<float>(atof(tokens[3].c_str()));

						m_vertices[currentVertex] = dls::math::vec3f(x, y, z);
						currentVertex++;
					}
					else if (tokens[0].compare("vn")==0) {
						auto x = static_cast<float>(atof(tokens[1].c_str()));
						auto y = static_cast<float>(atof(tokens[2].c_str()));
						auto z = static_cast<float>(atof(tokens[3].c_str()));

						m_normals[currentNormal] = dls::math::vec3f(x, y, z);
						currentNormal++;
					}
					else if (tokens[0].compare("vt")==0) {
						auto u = static_cast<float>(atof(tokens[1].c_str()));
						auto v = static_cast<float>(atof(tokens[2].c_str()));
						m_uvs[currentUV] = dls::math::vec2f(u, v);
						currentUV++;
					}
					else if (tokens[0].compare("f")==0) {
						dls::math::vec4i vertices(0);
						dls::math::vec4i normals(0);
						dls::math::vec4i uvs(0);
						unsigned int loops = std::min(4u, static_cast<unsigned int>(tokens.size())-1)+1;
						for (unsigned int i=1; i<loops; i++) {
							std::vector<std::string> faceindices =
									utilityCore::tokenizeString(tokens[i], "/");
							vertices[i-1] = atoi(faceindices[0].c_str());
							if (faceindices.size()==2 && m_numberOfNormals>1) {
								normals[i-1] = atoi(faceindices[1].c_str());
							}
							else if (faceindices.size()==2 && m_numberOfUVs>1) {
								uvs[i-1] = atoi(faceindices[1].c_str());
							}
							else if (faceindices.size()==3) {
								uvs[i-1] = atoi(faceindices[1].c_str());
								normals[i-1] = atoi(faceindices[2].c_str());
							}
						}
						m_polyVertexIndices[currentFace] = vertices;
						m_polyNormalIndices[currentFace] = normals;
						m_polyUVIndices[currentFace] = uvs;
						currentFace++;
					}
				}
			}
		}
	}
	fp_in.close();

	if (m_numberOfUVs==0) {
		std::cout << "No UVs found, creating default UVs..." << std::endl;
		delete [] m_uvs;
		m_uvs = new dls::math::vec2f[4];
		m_uvs[0] = dls::math::vec2f(0.0f, 0.0f);
		m_uvs[1] = dls::math::vec2f(0.0f, 1.0f);
		m_uvs[2] = dls::math::vec2f(1.0f, 1.0f);
		m_uvs[3] = dls::math::vec2f(1.0f, 0.0f);
		m_numberOfUVs = 4;
		for (unsigned int i=0; i<m_numberOfPolys; i++) {
			m_polyUVIndices[i] = dls::math::vec4i(1,2,3,4);
		}
	}

	if (m_numberOfNormals==0) {
		std::cout << "No normals found, creating default normals..." << std::endl;
		delete [] m_normals;
		m_numberOfNormals = m_numberOfPolys;
		m_normals = new dls::math::vec3f[m_numberOfPolys];
		for (unsigned int i=0; i<m_numberOfPolys; i++) {
			auto const vertexIndices = m_polyVertexIndices[i];
			auto const v0 = m_vertices[vertexIndices[0]-1];
			auto const v1 = m_vertices[vertexIndices[1]-1];
			auto const v2 = m_vertices[vertexIndices[2]-1];
			auto const n0 = v0-v1;
			auto const n1 = v2-v1;
			m_normals[i] = normalise(produit_croix(n1,n0));
			m_polyNormalIndices[i] = dls::math::vec4i(
						static_cast<int>(i) + 1,
						static_cast<int>(i) + 1,
						static_cast<int>(i) + 1,
						static_cast<int>(i) + 1);
		}
	}

	std::cout << "Read obj from " << filename << std::endl;
	// std::cout << m_numberOfVertices << " vertices" << std::endl;
	// std::cout << m_numberOfNormals << " normals" << std::endl;
	// std::cout << m_numberOfUVs << " uvs" << std::endl;
	// std::cout << m_numberOfPolys << " polys" << std::endl;

	return true;
}

bool Obj::WriteObj(const std::string& filename) {
	std::ofstream outputFile(filename.c_str());
	if (outputFile.is_open()) {
		//write out vertices
		for (unsigned int i=0; i<m_numberOfVertices; i++) {
			dls::math::vec3f v = m_vertices[i];
			outputFile << "v " << v.x << " " << v.y << " " << v.z << "\n";
		}
		//write out uvs
		for (unsigned int i=0; i<m_numberOfUVs; i++) {
			dls::math::vec2f uv = m_uvs[i];
			outputFile << "vt " << uv.x << " " << uv.y << "\n";
		}
		//write out normals
		for (unsigned int i=0; i<m_numberOfNormals; i++) {
			dls::math::vec3f n = m_normals[i];
			outputFile << "vn " << n.x << " " << n.y << " " << n.z << "\n";
		}
		//Write out faces
		if (m_numberOfUVs==0 && m_numberOfNormals==0) {
			for (unsigned int i=0; i<m_numberOfPolys; i++) {
				dls::math::vec4i fv = m_polyVertexIndices[i];
				outputFile << "f ";
				outputFile << fv.x << " " << fv.y << " " << fv.z << " ";
				if (fv.w!=0) {
					outputFile << fv.w;
				}
				outputFile << "\n";
			}
		}else {
			for (unsigned int i=0; i<m_numberOfPolys; i++) {
				dls::math::vec4i fv = m_polyVertexIndices[i];
				dls::math::vec4i fn = m_polyNormalIndices[i];
				dls::math::vec4i fuv = m_polyUVIndices[i];
				outputFile << "f ";
				outputFile << fv.x << "/" << fuv.x << "/" << fn.x << " ";
				outputFile << fv.y << "/" << fuv.y << "/" << fn.y << " ";
				outputFile << fv.z << "/" << fuv.z << "/" << fn.z << " ";
				if (fv.w!=0) {
					outputFile << fv.w << "/" << fuv.w << "/" << fn.w << " ";
				}
				outputFile << "\n";
			}
		}
		outputFile.close();
		std::cout << "Wrote obj file to " << filename << std::endl;
		return true;
	}else {
		std::cout << "Error: Unable to write to " << filename << std::endl;
		return false;
	}
}

void Obj::PrereadObj(const std::string& filename)
{
	std::ifstream fp_in;
	unsigned int vertexCount = 0;
	unsigned int normalCount = 0;
	unsigned int uvCount = 0;
	unsigned int faceCount = 0;
	auto fname = filename.c_str();
	fp_in.open(fname);

	if (fp_in.is_open()) {
		while (fp_in.good()) {
			std::string line;
			getline(fp_in, line);
			if (!line.empty()) {
				std::string header = utilityCore::getFirstNCharactersOfString(line, 2);
				if (header.compare("v ")==0) {
					vertexCount++;
				}else if (header.compare("vt")==0) {
					uvCount++;
				}else if (header.compare("vn")==0) {
					normalCount++;
				}else if (header.compare("f ")==0) {
					faceCount++;
				}
			}
		}
	}

	fp_in.close();

	m_numberOfVertices = vertexCount;
	m_numberOfNormals = normalCount;
	m_numberOfUVs = uvCount;
	m_numberOfPolys = faceCount;
}

/*Return the requested face from the mesh, unless the index is out of range, 
in which case return a face of area zero*/
Poly Obj::GetPoly(const unsigned int& polyIndex)
{
	Point pNull(dls::math::vec3f(0.0f, 0.0f, 0.0f),
				dls::math::vec3f(0.0f, 1.0f, 0.0f),
				dls::math::vec2f(0.0f, 0.0f));

	if (polyIndex>=m_numberOfPolys) {
		return Poly(pNull, pNull, pNull, polyIndex);
	}

	auto vertexIndices = m_polyVertexIndices[polyIndex];
	auto normalIndices = m_polyNormalIndices[polyIndex];
	auto uvIndices = m_polyUVIndices[polyIndex];
	Point p1(m_vertices[vertexIndices.x-1], m_normals[normalIndices.x-1], m_uvs[uvIndices.x-1]);
	Point p2(m_vertices[vertexIndices.y-1], m_normals[normalIndices.y-1], m_uvs[uvIndices.y-1]);
	Point p3(m_vertices[vertexIndices.z-1], m_normals[normalIndices.z-1], m_uvs[uvIndices.z-1]);
	if (vertexIndices.w==0) {
		return Poly(p1, p2, p3, p1, polyIndex);
	}
	Point p4(m_vertices[vertexIndices.w-1], m_normals[normalIndices.w-1],
			m_uvs[uvIndices.w-1]);
	return Poly(p1, p2, p3, p4, polyIndex);

}

//Applies given transforglm::mation glm::matrix to the given point
Point Obj::TransformPoint(const Point& p, const dls::math::mat4x4f& m)
{
	Point r = p;
	r.m_position = dls::math::vec3f( utilityCore::multiply(m,dls::math::vec4f(p.m_position,1.0f)) );
	r.m_normal = normalise(dls::math::vec3f( utilityCore::multiply(m,dls::math::vec4f(p.m_normal,0.0f)) ));
	return r;
}

//Applies given transforglm::mation glm::matrix to the given poly
Poly Obj::TransformPoly(const Poly& p, const dls::math::mat4x4f& m)
{
	Poly r;
	r.m_vertex0 = TransformPoint(p.m_vertex0, m);
	r.m_vertex1 = TransformPoint(p.m_vertex1, m);
	r.m_vertex2 = TransformPoint(p.m_vertex2, m);
	r.m_vertex3 = TransformPoint(p.m_vertex3, m);
	r.m_id = p.m_id;
	return r;
}

unsigned int Obj::GetNumberOfElements()
{
	return m_numberOfPolys;
}

spaceCore::Aabb Obj::GetElementAabb(const unsigned int& primID)
{
	auto vertexIndices = m_polyVertexIndices[primID];
	auto v0 = m_vertices[vertexIndices.x-1];
	auto v1 = m_vertices[vertexIndices.y-1];
	auto v2 = m_vertices[vertexIndices.z-1];
	auto v3 = v0;

	if (vertexIndices.w > 0) {
		v3 = m_vertices[vertexIndices.w-1];
	}

	auto min = std::min(std::min(std::min(v0, v1), v2), v3);
	auto max = std::max(std::max(std::max(v0, v1), v2), v3);
	//if v0 and v3 are the same, it's a triangle! else, it's a quad. TODO: find better way to
	//handle this check
	auto centroid = dls::math::vec3f(0.0f);

	if (vertexIndices.w>0) {
		centroid = (v0 + v1 + v2) / 3.0f;
	}
	else {
		centroid = (v0 + v1 + v2 + v3)/4.0f;
	}

	return spaceCore::Aabb(min, max, centroid, static_cast<int>(primID));
}

rayCore::Intersection Obj::IntersectElement(
		const unsigned int& primID,
		const rayCore::Ray& r)
{
	//check if quad by comparing x and w components
	dls::math::vec4i vi = m_polyVertexIndices[primID];
	//triangle case
	if (vi.w==0) {
		return TriangleTest(primID, r, false);
	}
	else { //quad case
		return QuadTest(primID, r);
	}
}

rayCore::Intersection Obj::QuadTest(
		const unsigned int& polyIndex,
		const rayCore::Ray& r)
{
	rayCore::Intersection intersect = TriangleTest(polyIndex, r, false);
	if (intersect.m_hit) {
		return intersect;
	}
	else {
		return TriangleTest(polyIndex, r, true);
	}
}

inline rayCore::Intersection Obj::RayTriangleTest(const dls::math::vec3f& v0,
												  const dls::math::vec3f& v1,
												  const dls::math::vec3f& v2,
												  const dls::math::vec3f& n0,
												  const dls::math::vec3f& n1,
												  const dls::math::vec3f& n2,
												  const dls::math::vec2f& u0,
												  const dls::math::vec2f& u1,
												  const dls::math::vec2f& u2,
												  const rayCore::Ray& r)
{
	//grab points and edges from poly
	dls::math::vec3f edge1 = v1-v0;
	dls::math::vec3f edge2 = v2-v0;
	//calculate determinant
	dls::math::vec3f rdirection = r.m_direction;
	dls::math::vec3f pvec = produit_croix(rdirection, edge2);
	float det = produit_scalaire(edge1, pvec);

	if (det == 0.0f) {
		return rayCore::Intersection();
	}

	float inv_det = 1.0f/det;
	dls::math::vec3f tvec = r.m_origin - v0;
	//calculate barycentric coord
	dls::math::vec3f bary;
	bary.x = produit_scalaire(tvec, pvec) * inv_det;
	dls::math::vec3f qvec = produit_croix(tvec, edge1);
	bary.y = produit_scalaire(rdirection, qvec) * inv_det;
	//calculate distance from ray origin to intersection
	float t = produit_scalaire(edge2, qvec) * inv_det;
	bool hit = (bary.x >= 0.0f && bary.y >= 0.0f && (bary.x + bary.y) <= 1.0f);

	if (hit) {
		bary.z = 1.0f - bary.x - bary.y;
		// dls::math::vec3f hitPoint = r.m_origin + t*r.m_direction;
		dls::math::vec3f hitPoint = r.GetPointAlongRay(t);
		dls::math::vec3f hitNormal = (n0 * bary.z)+
				(n1 * bary.x)+
				(n2 * bary.y);
		hitNormal = hitNormal/longueur(hitNormal);
		dls::math::vec2f hitUV = (u0 * bary.z)+
				(u1 * bary.x)+
				(u2 * bary.y);
		return rayCore::Intersection(true, hitPoint, hitNormal, hitUV, 0, 0);
	}

	return rayCore::Intersection();
}

rayCore::Intersection Obj::TriangleTest(const unsigned int& polyIndex,
										const rayCore::Ray& r,
										const bool& checkQuad)
{
	//grab indices
	dls::math::vec4i vi = m_polyVertexIndices[polyIndex];
	dls::math::vec4i ni = m_polyNormalIndices[polyIndex];
	dls::math::vec4i ui = m_polyUVIndices[polyIndex];
	//grab points, edges, uvs from poly
	dls::math::vec3f v0 = m_vertices[vi.x-1];
	dls::math::vec3f v1 = m_vertices[vi.y-1];
	dls::math::vec3f v2 = m_vertices[vi.z-1];
	dls::math::vec3f n0 = m_normals[ni.x-1];
	dls::math::vec3f n1 = m_normals[ni.y-1];
	dls::math::vec3f n2 = m_normals[ni.z-1];
	dls::math::vec2f u0 = m_uvs[ui.x-1];
	dls::math::vec2f u1 = m_uvs[ui.y-1];
	dls::math::vec2f u2 = m_uvs[ui.z-1];

	if (checkQuad==true) {
		v1 = m_vertices[vi.w-1];
		n1 = m_normals[ni.w-1];
		u1 = m_uvs[ui.w-1];
	}

	rayCore::Intersection result = Obj::RayTriangleTest(v0, v1, v2, n0, n1, n2, u0, u1, u2, r);
	result.m_objectID = m_id;
	result.m_primID = polyIndex;
	return result;
}

//====================================
// InterpolatedObj Class
//====================================

InterpolatedObj::InterpolatedObj()
{
	m_obj0 = nullptr;
	m_obj1 = nullptr;
}

/*Right now only prints a warning if objs have mismatched topology, must make this do something
better in the future since mismatched topology leads to Very Bad Things*/
InterpolatedObj::InterpolatedObj(objCore::Obj* obj0, objCore::Obj* obj1) {
	m_obj0 = obj0;
	m_obj1 = obj1;
	if (obj0->m_numberOfPolys!=obj1->m_numberOfPolys) {
		std::cout << "Warning: Attempted to create InterpolatedObj with mismatched topology!"
				  << std::endl;
	}
}

InterpolatedObj::~InterpolatedObj() {
}

rayCore::Intersection InterpolatedObj::IntersectElement(const unsigned int& primID,
														const rayCore::Ray& r) {
	//check if quad by comparing x and w components
	dls::math::vec4i vi = m_obj0->m_polyVertexIndices[primID];
	//triangle case
	if (vi.w==0) {
		return TriangleTest(primID, r, false);
	}else { //quad case
		return QuadTest(primID, r);
	}
}

rayCore::Intersection InterpolatedObj::TriangleTest(const unsigned int& polyIndex,
													const rayCore::Ray& r,
													const bool& checkQuad) {
	//make sure interp value is between 0 and 1
	float clampedInterp = r.m_frame - std::floor(r.m_frame);
	//grab indices
	dls::math::vec4i vi0 = m_obj0->m_polyVertexIndices[polyIndex];
	dls::math::vec4i ni0 = m_obj0->m_polyNormalIndices[polyIndex];
	dls::math::vec4i ui0 = m_obj0->m_polyUVIndices[polyIndex];
	dls::math::vec4i vi1 = m_obj1->m_polyVertexIndices[polyIndex];
	dls::math::vec4i ni1 = m_obj1->m_polyNormalIndices[polyIndex];
	dls::math::vec4i ui1 = m_obj1->m_polyUVIndices[polyIndex];
	//grab points, edges, uvs from poly
	dls::math::vec3f v0 = m_obj0->m_vertices[vi0.x-1] * (1.0f-clampedInterp) +
			m_obj1->m_vertices[vi1.x-1] * clampedInterp;
	dls::math::vec3f v1 = m_obj0->m_vertices[vi0.y-1] * (1.0f-clampedInterp) +
			m_obj1->m_vertices[vi1.y-1] * clampedInterp;
	dls::math::vec3f v2 = m_obj0->m_vertices[vi0.z-1] * (1.0f-clampedInterp) +
			m_obj1->m_vertices[vi1.z-1] * clampedInterp;
	dls::math::vec3f n0 = m_obj0->m_normals[ni0.x-1] * (1.0f-clampedInterp) +
			m_obj1->m_normals[ni1.x-1] * clampedInterp;
	dls::math::vec3f n1 = m_obj0->m_normals[ni0.y-1] * (1.0f-clampedInterp) +
			m_obj1->m_normals[ni1.y-1] * clampedInterp;
	dls::math::vec3f n2 = m_obj0->m_normals[ni0.z-1] * (1.0f-clampedInterp) +
			m_obj1->m_normals[ni1.z-1] * clampedInterp;
	dls::math::vec2f u0 = m_obj0->m_uvs[ui0.x-1] * (1.0f-clampedInterp) +
			m_obj1->m_uvs[ui1.x-1] * clampedInterp;
	dls::math::vec2f u1 = m_obj0->m_uvs[ui0.y-1] * (1.0f-clampedInterp) +
			m_obj1->m_uvs[ui1.y-1] * clampedInterp;
	dls::math::vec2f u2 = m_obj0->m_uvs[ui0.z-1] * (1.0f-clampedInterp) +
			m_obj1->m_uvs[ui1.z-1] * clampedInterp;
	if (checkQuad==true) {
		v1 = m_obj0->m_vertices[vi0.w-1] * (1.0f-clampedInterp) +
				m_obj1->m_vertices[vi1.w-1] * clampedInterp;
		n1 = m_obj0->m_normals[ni0.w-1] * (1.0f-clampedInterp) +
				m_obj1->m_normals[ni1.w-1] * clampedInterp;
		u1 = m_obj0->m_uvs[ui0.w-1] * (1.0f-clampedInterp) +
				m_obj1->m_uvs[ui1.w-1] * clampedInterp;
	}
	rayCore::Intersection result = Obj::RayTriangleTest(v0, v1, v2, n0/longueur(n0),
														n1/longueur(n1), n2/longueur(n2),
														u0, u1, u2, r);
	result.m_objectID = m_obj0->m_id;
	result.m_primID = polyIndex;
	return result;
}

rayCore::Intersection InterpolatedObj::QuadTest(const unsigned int& polyIndex,
												const rayCore::Ray& r) {
	rayCore::Intersection intersect = TriangleTest(polyIndex, r, false);
	if (intersect.m_hit) {
		return intersect;
	}else {
		return TriangleTest(polyIndex, r, true);
	}
}

/*Calls GetPoly for both referenced objs and returns a single interpolated poly. */
Poly InterpolatedObj::GetPoly(const unsigned int& polyIndex,
							  const float& interpolation) {
	Poly p0 = m_obj0->GetPoly(polyIndex);
	Poly p1 = m_obj1->GetPoly(polyIndex);
	//make sure interp value is between 0 and 1
	float clampedInterp = interpolation - std::floor(interpolation);
	Poly p;
	p.m_vertex0.m_position = p0.m_vertex0.m_position * (1.0f-clampedInterp) +
			p1.m_vertex0.m_position * clampedInterp;
	p.m_vertex1.m_position = p0.m_vertex1.m_position * (1.0f-clampedInterp) +
			p1.m_vertex1.m_position * clampedInterp;
	p.m_vertex2.m_position = p0.m_vertex2.m_position * (1.0f-clampedInterp) +
			p1.m_vertex2.m_position * clampedInterp;
	p.m_vertex3.m_position = p0.m_vertex3.m_position * (1.0f-clampedInterp) +
			p1.m_vertex3.m_position * clampedInterp;
	p.m_vertex0.m_normal = p0.m_vertex0.m_normal * (1.0f-clampedInterp) +
			p1.m_vertex0.m_normal * clampedInterp;
	p.m_vertex1.m_normal = p0.m_vertex1.m_normal * (1.0f-clampedInterp) +
			p1.m_vertex1.m_normal * clampedInterp;
	p.m_vertex2.m_normal = p0.m_vertex2.m_normal * (1.0f-clampedInterp) +
			p1.m_vertex2.m_normal * clampedInterp;
	p.m_vertex3.m_normal = p0.m_vertex3.m_normal * (1.0f-clampedInterp) +
			p1.m_vertex3.m_normal * clampedInterp;
	p.m_vertex0.m_uv = p0.m_vertex0.m_uv * (1.0f-clampedInterp) +
			p1.m_vertex0.m_uv * clampedInterp;
	p.m_vertex1.m_uv = p0.m_vertex1.m_uv * (1.0f-clampedInterp) +
			p1.m_vertex1.m_uv * clampedInterp;
	p.m_vertex2.m_uv = p0.m_vertex2.m_uv * (1.0f-clampedInterp) +
			p1.m_vertex2.m_uv * clampedInterp;
	p.m_vertex3.m_uv = p0.m_vertex3.m_uv * (1.0f-clampedInterp) +
			p1.m_vertex3.m_uv * clampedInterp;
	return p;
}

spaceCore::Aabb InterpolatedObj::GetElementAabb(const unsigned int& primID) {
	spaceCore::Aabb aabb0 = m_obj0->GetElementAabb(primID);
	spaceCore::Aabb aabb1 = m_obj1->GetElementAabb(primID);
	dls::math::vec3f combinedMin = std::min(aabb0.m_min, aabb1.m_min);
	dls::math::vec3f combinedMax = std::max(aabb0.m_max, aabb1.m_max);
	dls::math::vec3f combinedCentroid = (aabb0.m_centroid + aabb1.m_centroid) / 2.0f;
	return spaceCore::Aabb(combinedMin, combinedMax, combinedCentroid, aabb0.m_id);
}

unsigned int InterpolatedObj::GetNumberOfElements() {
	return m_obj0->GetNumberOfElements();
}
}
