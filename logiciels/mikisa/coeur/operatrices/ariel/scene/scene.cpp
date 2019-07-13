// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: scene.cpp
// Implements scene.hpp

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <openvdb/openvdb.h>
#include <openvdb/tools/MeshToVolume.h>
#pragma GCC diagnostic pop

#ifdef WITH_PARTIO
#	include <partio/Partio.h>
#endif

#include "scene.hpp"

namespace sceneCore{

Scene::Scene()
{
	m_solidLevelSet = new fluidCore::LevelSet();
	m_liquidLevelSet = new fluidCore::LevelSet();
	m_permaSolidLevelSet = new fluidCore::LevelSet();
	m_liquidParticleCount = 0;
}

Scene::~Scene()
{
	delete m_solidLevelSet;
	delete m_liquidLevelSet;
	delete m_permaSolidLevelSet;
}

void Scene::SetPaths(
		const dls::chaine& imagePath,
		const dls::chaine& meshPath,
		const dls::chaine& vdbPath,
		const dls::chaine& partioPath)
{
	m_imagePath = imagePath;
	m_meshPath = meshPath;
	m_vdbPath = vdbPath;
	m_partioPath = partioPath;
}

void Scene::ExportParticles(
		dls::tableau<fluidCore::Particle*> particles,
		const float& maxd,
		const int& frame,
		const bool& VDB,
		const bool& OBJ,
		const bool& PARTIO)
{
	auto const particlesCount = particles.taille();

	dls::tableau<fluidCore::Particle*> sdfparticles;
	for (unsigned int i = 0; i<particlesCount; i++) {
		if (particles[i]->m_type == fluidCore::geomtype::FLUID && !particles[i]->m_invalid) {
			sdfparticles.pousse(particles[i]);
		}
	}

	dls::chaine frameString = utilityCore::padString(4, utilityCore::convertIntToString(frame));

	if (PARTIO) {
#ifdef WITH_PARTIO
		auto sdfparticlesCount = sdfparticles.taille();

		dls::chaine partiofilename = m_partioPath;
		dls::tableau<dls::chaine> tokens = utilityCore::tokenizeString(partiofilename, ".");
		dls::chaine ext = "." + tokens[tokens.taille()-1];
		if (strcmp(ext.c_str(), ".gz")==0) {
			ext = "." + tokens[tokens.taille()-2] + ext;
		}

		utilityCore::replaceString(partiofilename, ext, "."+frameString+ext);

		Partio::ParticlesDataMutable* partioData = Partio::create();
		partioData->addParticles(sdfparticlesCount);
		Partio::ParticleAttribute positionAttr = partioData->addAttribute("position",
																		  Partio::VECTOR, 3);
		Partio::ParticleAttribute velocityAttr = partioData->addAttribute("v", Partio::VECTOR, 3);
		Partio::ParticleAttribute idAttr = partioData->addAttribute("id", Partio::INT, 1);

		for (unsigned int i = 0; i<sdfparticlesCount; i++) {
			float* pos = partioData->dataWrite<float>(positionAttr, i);
			pos[0] = sdfparticles[i]->m_p.x * maxd;
			pos[1] = sdfparticles[i]->m_p.y * maxd;
			pos[2] = sdfparticles[i]->m_p.z * maxd;
			float* vel = partioData->dataWrite<float>(velocityAttr, i);
			vel[0] = sdfparticles[i]->m_u.x;
			vel[1] = sdfparticles[i]->m_u.y;
			vel[2] = sdfparticles[i]->m_u.z;
			int* id = partioData->dataWrite<int>(idAttr, i);
			id[0] = i;
		}

		Partio::write(partiofilename.c_str() ,*partioData);
		partioData->release();
#endif
	}

	if (VDB || OBJ) {
		dls::chaine vdbfilename = m_vdbPath;
		utilityCore::replaceString(vdbfilename, ".vdb", "."+frameString+".vdb");

		dls::chaine objfilename = m_meshPath;
		utilityCore::replaceString(objfilename, ".obj", "."+frameString+".obj");

		fluidCore::LevelSet* fluidSDF = new fluidCore::LevelSet(sdfparticles, maxd);

		if (VDB) {
			fluidSDF->WriteVDBGridToFile(vdbfilename);
		}

		if (OBJ) {
			fluidSDF->WriteObjToFile(objfilename);
		}
		delete fluidSDF;
	}
}

void Scene::AddExternalForce(dls::math::vec3f force)
{
	m_externalForces.pousse(force);
}

dls::tableau<dls::math::vec3f>& Scene::GetExternalForces()
{
	return m_externalForces;
}

dls::tableau<geomCore::Geom*>& Scene::GetSolidGeoms()
{
	return m_solids;
}

dls::tableau<geomCore::Geom*>& Scene::GetLiquidGeoms()
{
	return m_liquids;
}

void Scene::BuildPermaSolidGeomLevelSet()
{
	delete m_permaSolidLevelSet;
	m_permaSolidLevelSet = new fluidCore::LevelSet();

	auto solidObjectsCount = m_solids.taille();
	auto permaSolidSDFCreated = false;

	for (unsigned int i=0; i< solidObjectsCount; i++) {
		auto type = m_solids[i]->m_geom->GetType();

		if (type == MESH && m_solids[i]->m_geom->IsDynamic() == false) {
			dls::math::mat4x4f transform;
			dls::math::mat4x4f inversetransform;

			if (m_solids[i]->m_geom->GetTransforms(0, transform, inversetransform)==true) {
				auto m = dynamic_cast<geomCore::MeshContainer*>(m_solids[i]->m_geom);
				auto o = &m->GetMeshFrame(0)->m_basegeom;

				if (permaSolidSDFCreated==false) {
					delete m_permaSolidLevelSet;
					m_permaSolidLevelSet = new fluidCore::LevelSet(o, transform);
					permaSolidSDFCreated = true;
				}
				else {
					fluidCore::LevelSet* objectSDF = new fluidCore::LevelSet(o, transform);
					m_permaSolidLevelSet->Merge(*objectSDF);
					delete objectSDF;
				}
			}
		}
	}
}

void Scene::BuildSolidGeomLevelSet(const int& frame)
{
	delete m_solidLevelSet;
	m_solidLevelSet = new fluidCore::LevelSet();

	//build levelsets for all varying geoms, then merge in cached permanent solid level set
	auto solidObjectsCount = m_solids.taille();
	auto solidSDFCreated = false;

	for (unsigned int i=0; i<solidObjectsCount; i++) {
		if (m_solids[i]->m_geom->IsDynamic() != true) {
			continue;
		}
		dls::math::mat4x4f transform;
		dls::math::mat4x4f inversetransform;

		if (m_solids[i]->m_geom->GetTransforms(static_cast<float>(frame), transform, inversetransform)==true) {
			GeomType type = m_solids[i]->m_geom->GetType();

			if (type==MESH) {
				auto m = dynamic_cast<geomCore::MeshContainer*>(m_solids[i]->m_geom);
				auto o = &m->GetMeshFrame(static_cast<float>(frame))->m_basegeom;

				if (solidSDFCreated==false) {
					delete m_solidLevelSet;
					m_solidLevelSet = new fluidCore::LevelSet(o, transform);
					solidSDFCreated = true;
				}
				else {
					auto objectSDF = new fluidCore::LevelSet(o, transform);
					m_solidLevelSet->Merge(*objectSDF);
					delete objectSDF;
				}
			}
			else if (type==ANIMMESH) {
				auto m = dynamic_cast<geomCore::AnimatedMeshContainer *>(m_solids[i]->m_geom);
				auto o = &m->GetMeshFrame(static_cast<float>(frame))->m_basegeom;
				auto interpolationWeight = m->GetInterpolationWeight(static_cast<float>(frame));

				if (solidSDFCreated==false) {
					delete m_solidLevelSet;
					m_solidLevelSet = new fluidCore::LevelSet(o, interpolationWeight,
															  transform);
					solidSDFCreated = true;
				}
				else {
					auto objectSDF = new fluidCore::LevelSet(o, interpolationWeight, transform);
					m_solidLevelSet->Merge(*objectSDF);
					delete objectSDF;
				}
			}
		}
	}

	if (solidSDFCreated==false) {
		m_solidLevelSet->Copy(*m_permaSolidLevelSet);
	}
	else {
		m_solidLevelSet->Merge(*m_permaSolidLevelSet);
	}
}

void Scene::BuildLiquidGeomLevelSet(const int& frame)
{
	delete m_liquidLevelSet;
	m_liquidLevelSet = new fluidCore::LevelSet();

	auto liquidObjectsCount = m_liquids.taille();
	auto liquidSDFCreated = false;

	for (unsigned int i=0; i<liquidObjectsCount; i++) {
		dls::math::mat4x4f transform;
		dls::math::mat4x4f inversetransform;

		if (m_liquids[i]->m_geom->GetTransforms(static_cast<float>(frame), transform, inversetransform) != true) {
			continue;
		}

		auto type = m_liquids[i]->m_geom->GetType();

		if (type==MESH) {
			auto m = dynamic_cast<geomCore::MeshContainer*>(m_liquids[i]->m_geom);
			auto o = &m->GetMeshFrame(static_cast<float>(frame))->m_basegeom;

			if (liquidSDFCreated==false) {
				delete m_liquidLevelSet;
				m_liquidLevelSet = new fluidCore::LevelSet(o, transform);
				liquidSDFCreated = true;
			}
			else {
				auto objectSDF = new fluidCore::LevelSet(o, transform);
				m_liquidLevelSet->Merge(*objectSDF);
				delete objectSDF;
			}
		}
		else if (type==ANIMMESH) {
			auto m=dynamic_cast<geomCore::AnimatedMeshContainer*>
					(m_liquids[i]->m_geom);
			auto o = &m->GetMeshFrame(static_cast<float>(frame))->m_basegeom;
			float interpolationWeight = m->GetInterpolationWeight(static_cast<float>(frame));

			if (liquidSDFCreated==false) {
				delete m_liquidLevelSet;
				m_liquidLevelSet = new fluidCore::LevelSet(o, interpolationWeight,
														   transform);
				liquidSDFCreated = true;
			}
			else {
				auto objectSDF = new fluidCore::LevelSet(o,interpolationWeight, transform);
				m_liquidLevelSet->Merge(*objectSDF);
				delete objectSDF;
			}
		}
	}
}

void Scene::BuildLevelSets(const int& frame)
{
	BuildLiquidGeomLevelSet(frame);
	BuildSolidGeomLevelSet(frame);
}

unsigned int Scene::GetLiquidParticleCount()
{
	return m_liquidParticleCount;
}

void Scene::GenerateParticles(dls::tableau<fluidCore::Particle*>& particles,
		const dls::math::vec3i &dimensions,
		const float& density,
		fluidCore::ParticleGrid* pgrid,
		const int& frame)
{
	std::cout << __func__ << std::endl;
	auto dims = dls::math::vec3f(
				static_cast<float>(dimensions.x),
				static_cast<float>(dimensions.y),
				static_cast<float>(dimensions.z));

	std::cout << "Dimensions : " << dims << std::endl;
	auto maxdimension = std::max(std::max(dims.x, dims.y), dims.z);

	auto thickness = 1.0f/maxdimension;
	auto w = density*thickness;

	//store list of pointers to particles we need to delete for later deletion in the locked block
	dls::tableau<fluidCore::Particle*> particlesToDelete;
	particlesToDelete.reserve(static_cast<long>(m_solidParticles.size()));
	particlesToDelete.insere(particlesToDelete.fin(), m_solidParticles.begin(),
							 m_solidParticles.end());

	tbb::concurrent_vector<fluidCore::Particle*>().swap(m_solidParticles);

	//place fluid particles
	//for each fluid geom in the frame, loop through voxels in the geom's AABB to place particles

#if 1
	auto liquidvelocity = dls::math::vec3f(0.0f);
	auto id = 0u;
	auto lminf = dls::math::vec3f(std::floor(-1.0f), std::floor(-1.0f), std::floor(-1.0f));
	auto lmaxf = dls::math::vec3f(std::ceil(1.0f), std::ceil(1.0f), std::ceil(1.0f));
	lminf = std::max(lminf, dls::math::vec3f(0.0f))/density; /* À FAIRE : extrait_min_max */
	lmaxf = std::min(lmaxf, dims+dls::math::vec3f(1.0f))/density; /* À FAIRE : extrait_min_max */

	auto lmin = dls::math::vec3<unsigned int>(
				static_cast<unsigned int>(lminf.x),
				static_cast<unsigned int>(lminf.y),
				static_cast<unsigned int>(lminf.z));

	auto lmax = dls::math::vec3<unsigned int>(
				static_cast<unsigned int>(lmaxf.x),
				static_cast<unsigned int>(lmaxf.y),
				static_cast<unsigned int>(lmaxf.z));

	std::cout << "lmin : " << lmin << std::endl;
	std::cout << "lmax : " << lmax << std::endl;

	//place particles in AABB
	tbb::parallel_for(tbb::blocked_range<unsigned int>(lmin.x,lmax.x),
					  [&](const tbb::blocked_range<unsigned int>& r)
	{
		for (unsigned int i=r.begin(); i!=r.end(); ++i) {
			for (unsigned int j = lmin.y; j<lmax.y; ++j) {
				for (unsigned int k = lmin.z; k<lmax.z; ++k) {
					float x = (static_cast<float>(i) * w) + (w / 2.0f);
					float y = (static_cast<float>(j) * w) + (w / 2.0f);
					float z = (static_cast<float>(k) * w) + (w / 2.0f);
					AddLiquidParticle(dls::math::vec3f(x,y,z), liquidvelocity,
									  3.0f/maxdimension, maxdimension,
									  frame, id);
				}
			}
		}
	});
	std::cout << "Generé : " << m_liquidParticles.size() << " particules liquides\n" << std::endl;
#else
	auto liquidCount = m_liquids.taille();

	for (unsigned int l=0; l<liquidCount; ++l) {
		auto liquidaabb = m_liquids[l]->m_geom->GetAabb(static_cast<float>(frame));
		auto liquidvelocity = m_liquidStartingVelocities[l];

		if (m_liquids[l]->m_geom->IsInFrame(static_cast<float>(frame))) {
			//clip AABB to sim boundaries, account for density
			auto lminf = dls::math::vec3f(std::floor(liquidaabb.m_min.x), std::floor(liquidaabb.m_min.y), std::floor(liquidaabb.m_min.z));
			auto lmaxf = dls::math::vec3f(std::ceil(liquidaabb.m_max.x), std::ceil(liquidaabb.m_max.y), std::ceil(liquidaabb.m_max.z));
			lminf = std::max(lminf, dls::math::vec3f(0.0f))/density; /* À FAIRE : extrait_min_max */
			lmaxf = std::min(lmaxf, dims+dls::math::vec3f(1.0f))/density; /* À FAIRE : extrait_min_max */

			auto lmin = dls::math::vec3<unsigned int>(
						static_cast<unsigned int>(lminf.x),
						static_cast<unsigned int>(lminf.y),
						static_cast<unsigned int>(lminf.z));

			auto lmax = dls::math::vec3<unsigned int>(
						static_cast<unsigned int>(lmaxf.x),
						static_cast<unsigned int>(lmaxf.y),
						static_cast<unsigned int>(lmaxf.z));

			//place particles in AABB
			tbb::parallel_for(tbb::blocked_range<unsigned int>(lmin.x,lmax.x),
							  [&](const tbb::blocked_range<unsigned int>& r)
			{
				for (unsigned int i=r.begin(); i!=r.end(); ++i) {
					for (unsigned int j = lmin.y; j<lmax.y; ++j) {
						for (unsigned int k = lmin.z; k<lmax.z; ++k) {
							float x = (static_cast<float>(i) * w) + (w / 2.0f);
							float y = (static_cast<float>(j) * w) + (w / 2.0f);
							float z = (static_cast<float>(k) * w) + (w / 2.0f);
							AddLiquidParticle(dls::math::vec3f(x,y,z), liquidvelocity,
											  3.0f/maxdimension, maxdimension,
											  frame, m_liquids[l]->m_id);
						}
					}
				}
			});
		}
	}
#endif

	auto solidCount = m_solids.taille();
	for (unsigned int l=0; l<solidCount; ++l) {
		auto solidaabb = m_solids[l]->m_geom->GetAabb(static_cast<float>(frame));

		if ((frame==0 && m_solids[l]->m_geom->IsDynamic()==false) ||
				(m_solids[l]->m_geom->IsDynamic()==true && m_solids[l]->m_geom->IsInFrame(static_cast<float>(frame)))) {
			//clip AABB to sim boundaries, account for density
			lminf = dls::math::vec3f(std::floor(solidaabb.m_min.x), std::floor(solidaabb.m_min.y), std::floor(solidaabb.m_min.z));
			lmaxf = dls::math::vec3f(std::ceil(solidaabb.m_max.x), std::ceil(solidaabb.m_max.y), std::ceil(solidaabb.m_max.z));
			lminf = std::max(lminf, dls::math::vec3f(0.0f))/density; /* À FAIRE : extrait_min_max */
			lmaxf = std::min(lmaxf, dims+dls::math::vec3f(1.0f))/density; /* À FAIRE : extrait_min_max */

			lmin = dls::math::vec3<unsigned int>(
						static_cast<unsigned int>(lminf.x),
						static_cast<unsigned int>(lminf.y),
						static_cast<unsigned int>(lminf.z));

			lmax = dls::math::vec3<unsigned int>(
						static_cast<unsigned int>(lmaxf.x),
						static_cast<unsigned int>(lmaxf.y),
						static_cast<unsigned int>(lmaxf.z));

			//place particles in AABB
			tbb::parallel_for(tbb::blocked_range<unsigned int>(lmin.x,lmax.x),
							  [&](const tbb::blocked_range<unsigned int>& r)
			{
				for (unsigned int i=r.begin(); i!=r.end(); ++i) {
					for (unsigned int j = lmin.y; j<lmax.y; ++j) {
						for (unsigned int k = lmin.z; k<lmax.z; ++k) {
							float x = (static_cast<float>(i) * w) + (w / 2.0f);
							float y = (static_cast<float>(j) * w) + (w / 2.0f);
							float z = (static_cast<float>(k) * w) + (w / 2.0f);
							AddSolidParticle(dls::math::vec3f(x,y,z), 3.0f/maxdimension,
											 maxdimension, frame, m_solids[l]->m_id);
						}
					}
				}
			});
		}
	}

	m_particleLock.lock();

	//delete old particles
	auto delParticleCount = particlesToDelete.taille();
	tbb::parallel_for(tbb::blocked_range<unsigned int>(0, static_cast<unsigned int>(delParticleCount)),
					  [&](const tbb::blocked_range<unsigned int>& r)
	{
		for (unsigned int i=r.begin(); i!=r.end(); ++i) {
			delete particlesToDelete[i];
		}
	});

	//add new particles to main particles list
	dls::tableau<fluidCore::Particle*>().echange(particles);
	particles.reserve(static_cast<long>(m_liquidParticles.size()+m_permaSolidParticles.size()+
					  m_solidParticles.size()));
	particles.insere(particles.fin(), m_liquidParticles.begin(), m_liquidParticles.end());
	particles.insere(particles.fin(), m_permaSolidParticles.begin(), m_permaSolidParticles.end());
	particles.insere(particles.fin(), m_solidParticles.begin(), m_solidParticles.end());
	m_liquidParticleCount = static_cast<unsigned int>(m_liquidParticles.size());

	//std::cout << "Solid+Fluid particles: " << particles.taille() << std::endl;

	m_particleLock.unlock();
}

bool Scene::CheckPointInsideGeomByID(
		const dls::math::vec3f& p,
		const float& frame,
		const unsigned int& geomID)
{
	if (geomID<m_geoms.taille()) {
		rayCore::Ray r;
		r.m_origin = p;
		r.m_frame = frame;
		r.m_direction = normalise(dls::math::vec3f(0.0f, 0.0f, 1.0f));
		//unsigned int hits = 0;
		spaceCore::HitCountTraverseAccumulator traverser(p);
		m_geoms[geomID].Intersect(r, traverser);
		//bool hit = false;

		if (traverser.m_intersection.m_hit==true) {
			if ((traverser.m_numberOfHits)%2==1) {
				return true;
			}
		}
	}

	return false;
}

bool Scene::CheckPointInsideSolidGeom(
		const dls::math::vec3f& p,
		const float& frame,
		unsigned int& solidGeomID)
{
	rayCore::Ray r;
	r.m_origin = p;
	r.m_frame = frame;
	r.m_direction = normalise(dls::math::vec3f(0.0f, 0.0f, 1.0f));

	auto solidGeomCount = m_solids.taille();

	for (unsigned int i=0; i<solidGeomCount; i++) {
		//unsigned int hits = 0;
		spaceCore::HitCountTraverseAccumulator traverser(p);
		m_solids[i]->Intersect(r, traverser);
		//bool hit = false;

		if (traverser.m_intersection.m_hit==true) {
			if ((traverser.m_numberOfHits)%2==1) {
				solidGeomID = i;
				return true;
			}
		}
	}

	return false;
}

bool Scene::CheckPointInsideLiquidGeom(
		const dls::math::vec3f& p,
		const float& frame,
		unsigned int& liquidGeomID)
{
	rayCore::Ray r;
	r.m_origin = p;
	r.m_frame = frame;
	r.m_direction = normalise(dls::math::vec3f(0.0f, 0.0f, 1.0f));

	auto liquidGeomCount = m_liquids.taille();

	for (unsigned int i=0; i<liquidGeomCount; i++) {
		spaceCore::HitCountTraverseAccumulator traverser(p);
		m_liquids[i]->Intersect(r, traverser);
	//	bool hit = false;

		if (traverser.m_intersection.m_hit==true) {
			if ((traverser.m_numberOfHits)%2==1) {
				liquidGeomID = i;
				return true;
			}
		}
	}

	return false;
}

rayCore::Intersection Scene::IntersectSolidGeoms(const rayCore::Ray& r)
{
	rayCore::Intersection bestHit;
	auto solidGeomCount = m_solids.taille();

	for (unsigned int i=0; i<solidGeomCount; i++) {
		spaceCore::TraverseAccumulator traverser;
		m_solids[i]->Intersect(r, traverser);
		bestHit = bestHit.CompareClosestAgainst(traverser.m_intersection, r.m_origin);
	}

	return bestHit;
}

void Scene::AddLiquidParticle(
		const dls::math::vec3f& pos,
		const dls::math::vec3f& vel,
		const float& thickness,
		const float& scale,
		const int& frame,
		const unsigned int& liquidGeomID)
{
	//auto worldpos = pos*scale;

	/* À FAIRE : CheckPointInsideGeomByID */
//	if (CheckPointInsideGeomByID(worldpos, static_cast<float>(frame), liquidGeomID)==true) {
		//if particles are in a solid, don't generate them
		//unsigned int solidGeomID;
		//if (CheckPointInsideSolidGeom(worldpos, static_cast<float>(frame), solidGeomID)==false) {
			fluidCore::Particle* p = new fluidCore::Particle;
			p->m_p = pos;
			p->m_u = vel;
			p->m_n = dls::math::vec3f(0.0f);
			p->m_density = 10.0f;
			p->m_type = fluidCore::FLUID;
			p->m_mass = 1.0f;
			p->m_invalid = false;
			m_liquidParticles.push_back(p);
		//}
//	}
}

void Scene::AddSolidParticle(
		const dls::math::vec3f& pos,
		const float& thickness,
		const float& scale,
		const int& frame,
		const unsigned int& solidGeomID)
{
	auto worldpos = pos*scale;

	if (CheckPointInsideGeomByID(worldpos, static_cast<float>(frame), solidGeomID)==true) {
		fluidCore::Particle* p = new fluidCore::Particle;
		p->m_p = pos;
		p->m_u = dls::math::vec3f(0.0f);
		p->m_n = dls::math::vec3f(0.0f);
		p->m_density = 10.0f;
		p->m_type = fluidCore::SOLID;
		p->m_mass = 10.0f;
		p->m_invalid = false;

		if (frame==0 && m_geoms[solidGeomID].m_geom->IsDynamic()==false) {
			m_permaSolidParticles.push_back(p);
		}
		else if (m_geoms[solidGeomID].m_geom->IsDynamic()==true) {
			m_solidParticles.push_back(p);
		}
		else {
			delete p;
		}
	}
}

fluidCore::LevelSet* Scene::GetSolidLevelSet()
{
	return m_solidLevelSet;
}

fluidCore::LevelSet* Scene::GetLiquidLevelSet()
{
	return m_liquidLevelSet;
}

}
