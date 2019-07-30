// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: cube.hpp. Adapted from Takua Render.
// Defines the unit cube geometry class, inherits from the generic geom class 

#ifndef CUBEGEN_HPP
#define CUBEGEN_HPP

#include "meshgen.hpp"

namespace geomCore {
//====================================
// Class Declarations
//====================================

class CubeGen: public MeshGen {
public:
	//Initializers
	CubeGen() = default;
	~CubeGen() = default;

	//Getters
	void Tesselate(objCore::Obj* o);
	void Tesselate(objCore::Obj* o, const dls::math::vec3f& lowerCorner, const dls::math::vec3f& upperCorner);
};
}

#endif
