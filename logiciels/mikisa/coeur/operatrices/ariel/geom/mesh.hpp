// TAKUA Render: Physically Based Renderer
// Written by Yining Karl Li
//
// File: mesh.hpp
// Mesh class inherits geomContainer for obj meshes

#ifndef MESH_HPP
#define MESH_HPP

#ifdef __CUDACC__
#define HOST __host__
#define DEVICE __device__
#define SHARED __shared__
#else
#define HOST
#define DEVICE
#define SHARED
#endif

#include "geom.hpp"
#include "obj/obj.hpp"
#include "../spatial/bvh.hpp"

namespace geomCore {

//====================================
// Struct Declarations
//====================================

//Only used as a temp data transfer container for CUDA memory setup
struct AnimatedMeshContainerData {
	spaceCore::Bvh<objCore::InterpolatedObj>**      m_meshFrames;
	GeomTransform**                                 m_geomTransforms;
	unsigned int                                    m_numberOfFrames;
	unsigned int                                    m_id;
	unsigned int                                    m_frameOffset;
	unsigned int                                    m_frameInterval;
};

struct MeshContainerData {
	spaceCore::Bvh<objCore::Obj>**                  m_meshFrames;
	GeomTransform**                                 m_geomTransforms;
	unsigned int                                    m_numberOfFrames;
	unsigned int                                    m_id;
	unsigned int                                    m_frameOffset;
	unsigned int                                    m_frameInterval;
};

//====================================
// Class Declarations
//====================================

class MeshContainer: public virtual GeomInterface {
public:
	MeshContainer();
	MeshContainer(const unsigned int& numberOfFrames,
				  const unsigned int& frameOffset,
				  const unsigned int& frameInterval,
				  const bool& prePersist,
				  const bool& postPersist,
				  GeomTransform** geomTransforms,
				  spaceCore::Bvh<objCore::Obj>** meshFrames);
	MeshContainer(MeshContainerData data);
	~MeshContainer() = default;

	MeshContainer(MeshContainer const &) = default;
	MeshContainer &operator=(MeshContainer const &) = default;

	GeomType GetType();
	unsigned int GetID();
	void Intersect(const rayCore::Ray& r, spaceCore::TraverseAccumulator& result);

	bool GetTransforms(const float& frame, dls::math::mat4x4f& transform,
					   dls::math::mat4x4f& inversetransform);
	spaceCore::Bvh<objCore::Obj>* GetMeshFrame(const float& frame);
	bool IsDynamic();
	bool IsInFrame(const float& frame);

	spaceCore::Aabb GetAabb(const float& frame);

	spaceCore::Bvh<objCore::Obj>**                  m_meshFrames{};
	GeomTransform**                                 m_geomTransforms{};
	unsigned int                                    m_numberOfFrames{};
	unsigned int                                    m_id{};
	unsigned int                                    m_frameOffset{};
	unsigned int                                    m_frameInterval{};
	bool                                            m_prePersist{};
	bool                                            m_postPersist{};
};

class AnimatedMeshContainer: public virtual GeomInterface {
public:
	AnimatedMeshContainer();
	AnimatedMeshContainer(const unsigned int& numberOfFrames,
						  const unsigned int& frameOffset,
						  const unsigned int& frameInterval,
						  const bool& prePersist,
						  const bool& postPersist,
						  GeomTransform** geomTransforms,
						  spaceCore::Bvh<objCore::InterpolatedObj>** meshFrames);
	AnimatedMeshContainer(AnimatedMeshContainerData data);
	~AnimatedMeshContainer() = default;

	AnimatedMeshContainer(AnimatedMeshContainer const &) = default;
	AnimatedMeshContainer &operator=(AnimatedMeshContainer const &) = default;

	GeomType GetType();
	unsigned int GetID();
	void Intersect(const rayCore::Ray& r, spaceCore::TraverseAccumulator& result);

	bool GetTransforms(const float& frame, dls::math::mat4x4f& transform,
					   dls::math::mat4x4f& inversetransform);
	spaceCore::Bvh<objCore::InterpolatedObj>* GetMeshFrame(const float& frame);
	float GetInterpolationWeight(const float& frame);
	bool IsDynamic();
	bool IsInFrame(const float& frame);

	spaceCore::Aabb GetAabb(const float& frame);

	spaceCore::Bvh<objCore::InterpolatedObj>**      m_meshFrames{};
	GeomTransform**                                 m_geomTransforms{};
	unsigned int                                    m_numberOfFrames{};
	unsigned int                                    m_id{};
	unsigned int                                    m_frameOffset{};
	unsigned int                                    m_frameInterval{};
	bool                                            m_prePersist{};
	bool                                            m_postPersist{};
};
}

#endif
