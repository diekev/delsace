// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: scene.hpp
// Scene class, meant for storing the scene state and checking particle positions

#ifndef SCENE_HPP
#define SCENE_HPP

#include "biblinternes/structures/tableau.hh"
#include <tbb/tbb.h>
#include <tbb/concurrent_vector.h>
#include "../utilities/utilities.h"
#include "../grid/grid.hpp"
#include "../geom/mesh.hpp"
#include "../grid/particlegrid.hpp"
#include "../grid/levelset.hpp"
#include "../spatial/bvh.hpp"

namespace sceneCore {
//====================================
// Class Declarations
//====================================

class Scene {
	friend class SceneLoader;
public:
	Scene();
	~Scene();

	Scene(Scene const &) = default;
	Scene &operator=(Scene const &) = default;

	void GenerateParticles(dls::tableau<fluidCore::Particle*>& particles,
						   const dls::math::vec3i& dimensions, const float& density,
						   fluidCore::ParticleGrid* pgrid, const int& frame);

	void AddExternalForce(dls::math::vec3f force);
	dls::tableau<dls::math::vec3f>& GetExternalForces();

	fluidCore::LevelSet* GetSolidLevelSet();
	fluidCore::LevelSet* GetLiquidLevelSet();

	void BuildLevelSets(const int& frame);
	void BuildLiquidGeomLevelSet(const int& frame);
	void BuildSolidGeomLevelSet(const int& frame);
	void BuildPermaSolidGeomLevelSet();

	void SetPaths(const dls::chaine& imagePath, const dls::chaine& meshPath,
				  const dls::chaine& vdbPath, const dls::chaine& partioPath);

	void ExportParticles(dls::tableau<fluidCore::Particle*> particles,
						 const float& maxd, const int& frame, const bool& VDB,
						 const bool& OBJ, const bool& PARTIO);

	dls::tableau<geomCore::Geom*>& GetSolidGeoms();
	dls::tableau<geomCore::Geom*>& GetLiquidGeoms();

	rayCore::Intersection IntersectSolidGeoms(const rayCore::Ray& r);
	bool CheckPointInsideSolidGeom(const dls::math::vec3f& p, const float& frame,
								   unsigned int& solidGeomID);
	bool CheckPointInsideLiquidGeom(const dls::math::vec3f& p, const float& frame,
									unsigned int& liquidGeomID);
	bool CheckPointInsideGeomByID(const dls::math::vec3f& p, const float& frame,
								  const unsigned int& geomID);

	unsigned int GetLiquidParticleCount();

	dls::chaine                                                 m_imagePath{};
	dls::chaine                                                 m_meshPath{};
	dls::chaine                                                 m_vdbPath{};
	dls::chaine                                                 m_partioPath{};

	tbb::mutex                                                  m_particleLock{};
	dls::tableau<geomCore::Geom*>                                m_liquids{};

private:
	void AddLiquidParticle(const dls::math::vec3f& pos, const dls::math::vec3f& vel, const float& thickness,
						   const float& scale, const int& frame,
						   const unsigned int& liquidGeomID);
	void AddSolidParticle(const dls::math::vec3f& pos, const float& thickness, const float& scale,
						  const int& frame, const unsigned int& solidGeomID);

	fluidCore::LevelSet*                                        m_solidLevelSet{};
	fluidCore::LevelSet*                                        m_permaSolidLevelSet{};
	fluidCore::LevelSet*                                        m_liquidLevelSet{};
	dls::tableau<dls::math::vec3f>                                      m_externalForces{};

	dls::tableau<geomCore::GeomTransform>                        m_geomTransforms{};
	dls::tableau<spaceCore::Bvh<objCore::Obj> >                  m_meshFiles{};
	dls::tableau<spaceCore::Bvh<objCore::InterpolatedObj> >      m_animMeshes{};
	dls::tableau<geomCore::Geom>                                 m_geoms{};
	dls::tableau<geomCore::MeshContainer>                        m_meshContainers{};
	dls::tableau<geomCore::AnimatedMeshContainer>                m_animmeshContainers{};
	dls::tableau<geomCore::Geom*>                                m_solids{};
	dls::tableau<dls::math::vec3f>                                      m_liquidStartingVelocities{};

	tbb::concurrent_vector<fluidCore::Particle*>                m_liquidParticles{};
	tbb::concurrent_vector<fluidCore::Particle*>                m_permaSolidParticles{};
	tbb::concurrent_vector<fluidCore::Particle*>                m_solidParticles{};

	unsigned int                                                m_liquidParticleCount{};

};
}

#endif

