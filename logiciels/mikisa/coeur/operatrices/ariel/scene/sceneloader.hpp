// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: sceneloader.hpp
// Simple JSON loader for fluid sim setups

#ifndef SCENELOADER_HPP
#define SCENELOADER_HPP

#include <map>
#include <vector>

//#include <json/json.h>
#include "../utilities/utilities.h"
#include "../geom/geomlist.hpp"
#include "scene.hpp"

namespace Json {
class Value;
}

namespace sceneCore {
//====================================
// Class Declarations
//====================================

class SceneLoader {
public:
	SceneLoader(const std::string& filename);
	~SceneLoader();

	SceneLoader(SceneLoader const &) = default;
	SceneLoader &operator=(SceneLoader const &) = default;

	Scene* GetScene();
	float GetDensity();
	dls::math::vec3f GetDimensions();
	float GetStepsize();

	dls::math::vec3f       m_cameraRotate{};
	dls::math::vec3f       m_cameraTranslate{};
	float           m_cameraLookat{};
	dls::math::vec2f       m_cameraResolution{};
	dls::math::vec2f       m_cameraFov{};

private:
	void LoadSettings(const Json::Value& jsonsettings);
	void LoadCamera(const Json::Value& jsoncamera);
	void LoadGlobalForces(const Json::Value& jsonforces);

	void LoadGeomTransforms(const Json::Value& jsontransforms);
	void LoadMeshFiles(const Json::Value& jsonmeshfiles);
	void LoadAnimMeshSequences(const Json::Value& jsonanimmesh);
	void LoadGeom(const Json::Value& jsongeom);
	void LoadSim(const Json::Value& jsonsim);

	Scene*                                  m_s{};
	dls::math::vec3f                               m_dimensions{};
	float                                   m_density{};
	float                                   m_stepsize{};
	std::string                             m_relativePath{};
	std::string                             m_imagePath{};
	std::string                             m_meshPath{};
	std::string                             m_vdbPath{};
	std::string                             m_partioPath{};
	dls::tableau<dls::math::vec3f>                  m_externalForces{};

	std::map<std::string, unsigned int>                         m_linkNames{};
	dls::tableau< dls::tableau<
	spaceCore::Bvh<objCore::InterpolatedObj>* > >  m_animMeshSequences{};
};
}

#endif
