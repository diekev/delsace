// Ariel: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: flip.hpp
// A flip simulator!

#ifndef FLIP_HPP
#define FLIP_HPP

#include <tbb/tbb.h>
#include "../grid/grid.hpp"
#include "../grid/particlegrid.hpp"
#include "../scene/scene.hpp"

namespace fluidCore {
//====================================
// Class Declarations
//====================================

class FlipSim{
public:
	FlipSim(const dls::math::vec3i& maxres, const float& density, const float& stepsize,
			sceneCore::Scene* scene, const bool& verbose);
	~FlipSim();

	FlipSim(FlipSim const &) = default;
	FlipSim &operator=(FlipSim const &) = default;

	void Init();
	void Step(bool saveVDB, bool saveOBJ, bool savePARTIO);

	std::vector<Particle*>* GetParticles();
	dls::math::vec3i GetDimensions();
	sceneCore::Scene* GetScene();

	int                                     m_frame{};

private:
	void StoreTempParticleVelocities();
	void CheckParticleSolidConstraints();
	void AdjustParticlesStuckInSolids();
	void ComputeDensity();
	void ApplyExternalForces();
	void SubtractPreviousGrid();
	void StorePreviousGrid();
	void SubtractPressureGradient();
	void ExtrapolateVelocity();
	void Project();
	void SolvePicFlip();
	void AdvectParticles();
	bool IsCellFluid(const int& x, const int& y, const int& z);

	dls::math::vec3i                              m_dimensions{};
	std::vector<Particle*>                  m_particles{};
	MacGrid                                 m_mgrid{};
	MacGrid                                 m_mgrid_previous{};
	ParticleGrid*                           m_pgrid{};

	int                                     m_subcell{};
	float                                   m_density{};
	float                                   m_max_density{};
	float                                   m_densitythreshold{};
	float                                   m_picflipratio{};

	sceneCore::Scene*                       m_scene{};

	bool                                    m_verbose{};
	float                                   m_stepsize{};
};

class FlipTask: public tbb::task {
public:
	FlipTask(FlipSim* sim, bool dumpVDB, bool dumpOBJ, bool dumpPARTIO);

	FlipTask(FlipTask const &) = default;
	FlipTask &operator=(FlipTask const &) = default;

	tbb::task* execute();
private:
	FlipSim*                                m_sim{};
	bool                                    m_dumpPARTIO{};
	bool                                    m_dumpOBJ{};
	bool                                    m_dumpVDB{};
};
}

#endif
