// TAKUA Render: Physically Based Renderer
// Written by Yining Karl Li
//
// File: mesh.cpp
// Implements mesh.hpp

#include "mesh.hpp"
#include <delsace/math/outils.hh>

namespace geomCore {

//====================================
// MeshContainer Class
//====================================

MeshContainer::MeshContainer()
{
	m_meshFrames = nullptr;
	m_geomTransforms = nullptr;
	m_prePersist = false;
	m_postPersist = false;
	m_frameInterval = 1;
	m_frameOffset = 0;
}

MeshContainer::MeshContainer(const unsigned int& numberOfFrames,
							 const unsigned int& frameOffset,
							 const unsigned int& frameInterval,
							 const bool& prePersist,
							 const bool& postPersist,
							 GeomTransform** geomTransforms,
							 spaceCore::Bvh<objCore::Obj>** meshFrames)
{
	m_numberOfFrames = numberOfFrames;
	m_geomTransforms = geomTransforms;
	m_meshFrames = meshFrames;
	m_frameOffset = frameOffset;
	m_frameInterval = frameInterval;
	m_prePersist = prePersist;
	m_postPersist = postPersist;
}

MeshContainer::MeshContainer(MeshContainerData data)
{
	m_numberOfFrames = data.m_numberOfFrames;
	m_geomTransforms = data.m_geomTransforms;
	m_meshFrames = data.m_meshFrames;
	m_id = data.m_id;
}

GeomType MeshContainer::GetType()
{
	return MESH;
}

unsigned int MeshContainer::GetID()
{
	return m_id;
}

spaceCore::Bvh<objCore::Obj>* MeshContainer::GetMeshFrame(const float& frame)
{
	auto clampedFrame = (frame - float(m_frameOffset)) / float(m_frameInterval);
	clampedFrame = dls::math::restreint(clampedFrame, 0.0f, float(m_numberOfFrames-1));
	auto lowerFrame = static_cast<unsigned int>(std::floor(clampedFrame));
	return m_meshFrames[lowerFrame];
}

spaceCore::Aabb MeshContainer::GetAabb(const float& frame)
{
	dls::math::mat4x4f transform;
	dls::math::mat4x4f inverse;

	if (GetTransforms(frame, transform, inverse)==true) {
		spaceCore::Aabb box = GetMeshFrame(frame)->m_nodes[1].m_bounds;
		return box.Transform(transform);
	}

	return spaceCore::Aabb();
}       

bool MeshContainer::IsInFrame(const float& frame)
{
	float clampedFrame = (frame - float(m_frameOffset)) / float(m_frameInterval);

	if ((clampedFrame<0.0f && m_prePersist==false) ||
			(clampedFrame>=static_cast<float>(m_numberOfFrames) && m_postPersist==false)) {
		return false;
	}

	return true;
}

bool MeshContainer::GetTransforms(
		const float& frame,
		dls::math::mat4x4f& transform,
		dls::math::mat4x4f& inversetransform)
{
	//translate frame into local frame space
	auto clampedFrame = (frame - float(m_frameOffset)) / float(m_frameInterval);

	if ((clampedFrame<0.0f && m_prePersist==false) ||
			(clampedFrame>=static_cast<float>(m_numberOfFrames) && m_postPersist==false)) {
		return false;
	}

	clampedFrame = dls::math::restreint(clampedFrame, 0.0f, float(m_numberOfFrames-1));
	//ceil/floor to get frame indices
	auto upperFrame = static_cast<unsigned int>(std::ceil(clampedFrame));
	auto lowerFrame = static_cast<unsigned int>(std::floor(clampedFrame));
	auto lerpWeight = clampedFrame - float(lowerFrame);
	//grab relevant transforms and LERP to get interpolated transform, apply inverse to ray
	auto interpT = m_geomTransforms[lowerFrame]->m_translation * (1.0f-lerpWeight) +
			m_geomTransforms[upperFrame]->m_translation * lerpWeight;
	auto interpR = m_geomTransforms[lowerFrame]->m_rotation * (1.0f-lerpWeight) +
			m_geomTransforms[upperFrame]->m_rotation * lerpWeight;
	auto interpS = m_geomTransforms[lowerFrame]->m_scale * (1.0f-lerpWeight) +
			m_geomTransforms[upperFrame]->m_scale * lerpWeight;
	inversetransform = utilityCore::buildInverseTransformationMatrix(interpT, interpR, interpS);
	transform = utilityCore::buildTransformationMatrix(interpT, interpR, interpS);

	return true;
}

void MeshContainer::Intersect(const rayCore::Ray& r, 
							  spaceCore::TraverseAccumulator& result) {
	dls::math::mat4x4f transform;
	dls::math::mat4x4f inversetransform;
	if (GetTransforms(r.m_frame, transform, inversetransform)==false) {
		return;
	}
	rayCore::Ray transformedR = r.Transform(inversetransform);
	//run intersection, transform result back into worldspace
	GetMeshFrame(r.m_frame)->Traverse(transformedR, result);
	result.Transform(transform);
}

bool MeshContainer::IsDynamic()
{
	return !(m_prePersist==true && m_postPersist==true && m_numberOfFrames==1);
}

//====================================
// AnimatedMeshContainer Class
//====================================

AnimatedMeshContainer::AnimatedMeshContainer()
{
	m_meshFrames = nullptr;
	m_geomTransforms = nullptr;
	m_prePersist = false;
	m_postPersist = false;
	m_frameInterval = 1;
	m_frameOffset = 0;
}

AnimatedMeshContainer::AnimatedMeshContainer(const unsigned int& numberOfFrames,
											 const unsigned int& frameOffset,
											 const unsigned int& frameInterval,
											 const bool& prePersist,
											 const bool& postPersist,
											 GeomTransform** geomTransforms,
											 spaceCore::Bvh<objCore::InterpolatedObj>**
											 meshFrames)
{
	m_numberOfFrames = numberOfFrames;
	m_geomTransforms = geomTransforms;
	m_meshFrames = meshFrames;
	m_frameOffset = frameOffset;
	m_frameInterval = frameInterval;
	m_prePersist = prePersist;
	m_postPersist = postPersist;
}

AnimatedMeshContainer::AnimatedMeshContainer(AnimatedMeshContainerData data)
{
	m_numberOfFrames = data.m_numberOfFrames;
	m_geomTransforms = data.m_geomTransforms;
	m_meshFrames = data.m_meshFrames;
	m_id = data.m_id;
}

spaceCore::Aabb AnimatedMeshContainer::GetAabb(const float& frame)
{
	dls::math::mat4x4f transform;
	dls::math::mat4x4f inverse;

	if (GetTransforms(frame, transform, inverse)==true) {
		spaceCore::Aabb box = GetMeshFrame(frame)->m_nodes[1].m_bounds;
		return box.Transform(transform);
	}

	return spaceCore::Aabb();
}

GeomType AnimatedMeshContainer::GetType()
{
	return ANIMMESH;
}

unsigned int AnimatedMeshContainer::GetID()
{
	return m_id;
}

bool AnimatedMeshContainer::IsInFrame(const float& frame)
{
	float clampedFrame = (frame - float(m_frameOffset)) / float(m_frameInterval);
	if ((clampedFrame<0.0f && m_prePersist==false) ||
			(clampedFrame>=static_cast<float>(m_numberOfFrames) && m_postPersist==false)) {
		return false;
	}

	return true;
}

bool AnimatedMeshContainer::GetTransforms(
		const float& frame,
		dls::math::mat4x4f& transform,
		dls::math::mat4x4f& inversetransform)
{
	//translate frame into local frame space
	float clampedFrame = (frame - float(m_frameOffset)) / float(m_frameInterval);

	if ((clampedFrame<0.0f && m_prePersist==false) ||
			(clampedFrame>=static_cast<float>(m_numberOfFrames) && m_postPersist==false)) {
		return false;
	}

	clampedFrame = dls::math::restreint(clampedFrame, 0.0f, float(m_numberOfFrames-1));
	//ceil/floor to get frame indices
	auto upperFrame = static_cast<unsigned int>(std::ceil(clampedFrame));
	auto lowerFrame = static_cast<unsigned int>(std::floor(clampedFrame));
	auto lerpWeight = clampedFrame - float(lowerFrame);
	//grab relevant transforms and LERP to get interpolated transform, apply inverse to ray
	auto interpT = m_geomTransforms[lowerFrame]->m_translation * (1.0f-lerpWeight) +
			m_geomTransforms[upperFrame]->m_translation * lerpWeight;
	auto interpR = m_geomTransforms[lowerFrame]->m_rotation * (1.0f-lerpWeight) +
			m_geomTransforms[upperFrame]->m_rotation * lerpWeight;
	auto interpS = m_geomTransforms[lowerFrame]->m_scale * (1.0f-lerpWeight) +
			m_geomTransforms[upperFrame]->m_scale * lerpWeight;
	inversetransform = utilityCore::buildInverseTransformationMatrix(interpT, interpR, interpS);
	transform = utilityCore::buildTransformationMatrix(interpT, interpR, interpS);

	return true;
}

float AnimatedMeshContainer::GetInterpolationWeight(const float& frame)
{
	//translate frame into local frame space
	auto clampedFrame = (frame - float(m_frameOffset)) / float(m_frameInterval);

	if ((clampedFrame<0.0f && m_prePersist==false) ||
			(clampedFrame>=static_cast<float>(m_numberOfFrames) && m_postPersist==false)) {
		return false;
	}

	clampedFrame = dls::math::restreint(clampedFrame, 0.0f, float(m_numberOfFrames-1));
	//ceil/floor to get frame indices
	// auto upperFrame = std::ceil(clampedFrame);
	auto lowerFrame = std::floor(clampedFrame);
	return clampedFrame - lowerFrame;
}

spaceCore::Bvh<objCore::InterpolatedObj>* AnimatedMeshContainer::GetMeshFrame(
		const float& frame)
{
	auto clampedFrame = (frame - float(m_frameOffset)) / float(m_frameInterval);
	clampedFrame = dls::math::restreint(clampedFrame, 0.0f, float(m_numberOfFrames-1));
	auto lowerFrame = static_cast<unsigned int>(std::floor(clampedFrame));
	return m_meshFrames[lowerFrame];
}

void AnimatedMeshContainer::Intersect(const rayCore::Ray& r, 
									  spaceCore::TraverseAccumulator& result) {
	dls::math::mat4x4f transform;
	dls::math::mat4x4f inversetransform;
	if (GetTransforms(r.m_frame, transform, inversetransform)==false) {
		return;
	}

	auto transformedR = r.Transform(inversetransform);
	//run intersection, transform result back into worldspace
	float clampedFrame = (r.m_frame - float(m_frameOffset)) / float(m_frameInterval);
	clampedFrame = dls::math::restreint(clampedFrame, 0.0f, float(m_numberOfFrames-1));
	auto lowerFrame = static_cast<unsigned int>(std::floor(clampedFrame));
	m_meshFrames[lowerFrame]->Traverse(transformedR, result);
	result.Transform(transform);
}

bool AnimatedMeshContainer::IsDynamic()
{
	return true;
}

}
