#include <memory>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>

#include "util/util_curve.h"
#include "util/util_time.h"

class Film;

struct Ray {
	glm::vec3 orig;
	glm::vec3 dir;
	float length;
};

#define STATIC_CLONE_FUNC(className) \
	static className::Ptr staticClone(className::CPtr object) \
	{ return Ptr(object->rawClone()); }

class Camera {
public:
	using Ptr = std::shared_ptr<Camera>;
	using CPtr = std::shared_ptr<const Camera>;
	using MatrixVec = std::vector<glm::mat4>;

	Camera();
	virtual ~Camera() = default;

	void setPosition(const VectorCurve &curve);
	glm::vec3 position(const PTime &time) const;
	void setOrientation(const QuatCurve &curve);
	glm::fquat orientation(const PTime &time) const;

	void setResolution(const glm::ivec2 &resolution);
	const glm::ivec2 &resolution() const;

	void setNumTimeSamples(const unsigned int numSmaples);
	unsigned int numTimeSamples() const;

	glm::vec3 worldToCamera(const glm::vec3 &wsP, const PTime &time) const;
	glm::vec3 cameraToWorld(const glm::vec3 &wsP, const PTime &time) const;

	const MatrixVec &worldToCameraMatrices() const;
	const MatrixVec &cameraToWorldMatrices() const;

	virtual glm::vec3 worldToScreen(const glm::vec3 &wsP, const PTime &time) const = 0;
	virtual glm::vec3 screenToWorld(const glm::vec3 &wsP, const PTime &time) const = 0;
	virtual glm::vec3 worldToRaster(const glm::vec3 &wsP, const PTime &time) const = 0;
	virtual glm::vec3 rasterToWorld(const glm::vec3 &wsP, const PTime &time) const = 0;

	virtual bool canTransformNegativeCamZ() const = 0;

	Camera::Ptr clone() const
	{
		return Ptr(rawClone());
	}

protected:
	virtual Camera *rawClone() const = 0;

	virtual void recomputeTransforms();
	glm::vec3 transformPoint(const glm::vec3 &p, const MatrixVec &matrices, const PTime &time) const;

	glm::mat4 computeCameraToWorld(const PTime &time) const;

	VectorCurve m_position;
	QuatCurve m_orientation;
	glm::ivec2 m_resolution;
	unsigned int m_num_samples;

	MatrixVec m_cam_to_world;
	MatrixVec m_world_to_cam;
};

class PerspectiveCamera : public Camera {
public:
	using Ptr = std::shared_ptr<PerspectiveCamera>;
	using CPtr = std::shared_ptr<const PerspectiveCamera>;

	PerspectiveCamera();

	static Ptr create()
	{
		return Ptr(new PerspectiveCamera);
	}

	void setClipPlanes(const float near, const float far);
	void setVerticalFOV(const FloatCurve &curve);

	const MatrixVec &worldToScreenMatrices() const;
	const MatrixVec &screenToWorldMatrices() const;

	STATIC_CLONE_FUNC(PerspectiveCamera)

	virtual glm::vec3 worldToScreen(const glm::vec3 &wsP, const PTime &time) const;
	virtual glm::vec3 screenToWorld(const glm::vec3 &wsP, const PTime &time) const;
	virtual glm::vec3 worldToRaster(const glm::vec3 &wsP, const PTime &time) const;
	virtual glm::vec3 rasterToWorld(const glm::vec3 &wsP, const PTime &time) const;

	virtual bool canTransformNegativeCamZ() const;

	PerspectiveCamera::Ptr clone() const
	{
		return Ptr(rawClone());
	}

protected:
	virtual void recomputeTransforms();

	void getTransforms(const PTime &time, glm::mat4 &cameraToScreen, glm::mat4 &screenToWorld) const;

	FloatCurve m_vert_fov;

	float m_near, m_far;
	MatrixVec m_world_to_screen;
	MatrixVec m_screen_to_world;
	MatrixVec m_world_to_raster;
	MatrixVec m_raster_to_world;

private:
	virtual PerspectiveCamera *rawClone() const
	{
		return new PerspectiveCamera(*this);
	}
};

struct SphericalCoords {
	float radius, latitude, longitude;

	SphericalCoords()
		: radius(0.0f)
		, latitude(0.0f)
		, longitude(0.0f)
	{}
};

class SphericalCamera : public Camera {
public:
	using Ptr = std::shared_ptr<SphericalCamera>;
	using CPtr = std::shared_ptr<const SphericalCamera>;

	SphericalCamera();

	static Ptr create()
	{
		return Ptr(new SphericalCamera);
	}

	STATIC_CLONE_FUNC(SphericalCamera)

	virtual glm::vec3 worldToScreen(const glm::vec3 &wsP, const PTime &time) const;
	virtual glm::vec3 screenToWorld(const glm::vec3 &wsP, const PTime &time) const;
	virtual glm::vec3 worldToRaster(const glm::vec3 &wsP, const PTime &time) const;
	virtual glm::vec3 rasterToWorld(const glm::vec3 &wsP, const PTime &time) const;

	virtual bool canTransformNegativeCamZ() const;

	SphericalCamera::Ptr clone() const
	{
		return Ptr(rawClone());
	}

protected:
	virtual void recomputeTransforms();

	SphericalCoords cartToSphere(const glm::vec3 &cs) const;
	glm::vec3 sphereToCart(const SphericalCoords &ss) const;

	glm::mat4 m_screen_to_raster;
	glm::mat4 m_raster_to_screen;

private:
	virtual SphericalCamera *rawClone() const
	{
		return new SphericalCamera(*this);
	}
};

