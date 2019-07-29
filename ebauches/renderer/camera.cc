#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>

#include "camera.h"

Camera::Camera()
	: m_resolution(640, 480)
	, m_num_samples(2)
{
	Camera::recomputeTransforms();
}

void Camera::setPosition(const VectorCurve &curve)
{
	m_position = curve;
	recomputeTransforms();
}

glm::vec3 Camera::position(const PTime &time) const
{
	return m_position.interpolate(time);
}

void Camera::setOrientation(const QuatCurve &curve)
{
	m_orientation = curve;
	recomputeTransforms();
}

glm::fquat Camera::orientation(const PTime &time) const
{
	return m_orientation.interpolate(time);
}

void Camera::setResolution(const glm::ivec2 &resolution)
{
	m_resolution = resolution;
	recomputeTransforms();
}

const glm::ivec2 &Camera::resolution() const
{
	return m_resolution;
}

void Camera::setNumTimeSamples(const unsigned int numSamples)
{
	if (numSamples == 0) {
//		throw TimeSamplesException("Zero");
	}

	m_num_samples = numSamples;
	recomputeTransforms();
}

unsigned int Camera::numTimeSamples() const
{
	return m_num_samples;
}

glm::vec3 Camera::worldToCamera(const glm::vec3 &wsP, const PTime &time) const
{
	return transformPoint(wsP, m_world_to_cam, time);
}

glm::vec3 Camera::cameraToWorld(const glm::vec3 &wsP, const PTime &time) const
{
	return transformPoint(wsP, m_cam_to_world, time);
}

const Camera::MatrixVec &Camera::worldToCameraMatrices() const
{
	return m_world_to_cam;
}

const Camera::MatrixVec &Camera::cameraToWorldMatrices() const
{
	return m_cam_to_world;
}

void Camera::recomputeTransforms()
{
	m_cam_to_world.resize(m_num_samples);
	m_world_to_cam.resize(m_num_samples);

	for (auto i = 0u; i < m_num_samples; ++i) {
		PTime time(math::parametric(i, m_num_samples));
		m_cam_to_world[i] = computeCameraToWorld(time);
		m_world_to_cam[i] = glm::inverse(m_cam_to_world[i]);
	}
}

glm::vec3 Camera::transformPoint(const glm::vec3 &p, const MatrixVec &matrices, const PTime &time) const
{
	auto step_size = 1.0f / static_cast<float>(m_num_samples - 1);
	auto t = time / step_size;
	auto first = static_cast<unsigned int>(std::floor(t));
	auto second = std::min(first + 1, m_num_samples - 1);
	auto factor = t - static_cast<float>(first);

	glm::vec4 t0 = glm::vec4(p, 0.0f) * matrices[first];
	glm::vec4 t1 = glm::vec4(p, 0.0f) * matrices[second];

	glm::vec4 result = glm::lerp(t0, t1, factor);
	return glm::vec3(result.x, result.y, result.z);
}

glm::mat4 Camera::computeCameraToWorld(const PTime &time) const
{
	glm::vec3 position = m_position.interpolate(time);
	glm::fquat orientation = m_orientation.interpolate(time);

	glm::mat4 translation = glm::translate(glm::mat4(0.0f), position);
	glm::mat4 rotation = glm::mat4_cast(orientation);
	glm::mat4 flip_z = glm::scale(glm::mat4(0.0f), glm::vec3(1.0f, 1.0f, -1.0f));

	return flip_z * rotation * translation;
}

/* ************************** PerspectiveCamera **************************** */

PerspectiveCamera::PerspectiveCamera()
	: Camera()
	, m_near(1.0f)
	, m_far(100.0f)
{
	FloatCurve fov;
	fov.addSample(0.0f, 45.0f);
	setVerticalFOV(fov);
}

void PerspectiveCamera::setClipPlanes(const float near, const float far)
{
	m_near = near;
	m_far = far;
}

void PerspectiveCamera::setVerticalFOV(const FloatCurve &curve)
{
	m_vert_fov = curve;
}

const Camera::MatrixVec &PerspectiveCamera::worldToScreenMatrices() const
{
	return m_world_to_screen;
}

const Camera::MatrixVec &PerspectiveCamera::screenToWorldMatrices() const
{
	return m_screen_to_world;
}

glm::vec3 PerspectiveCamera::worldToScreen(const glm::vec3 &wsP, const PTime &time) const
{
	return transformPoint(wsP, m_world_to_screen, time);
}

glm::vec3 PerspectiveCamera::screenToWorld(const glm::vec3 &wsP, const PTime &time) const
{
	return transformPoint(wsP, m_screen_to_world, time);
}

glm::vec3 PerspectiveCamera::worldToRaster(const glm::vec3 &wsP, const PTime &time) const
{
	return transformPoint(wsP, m_world_to_raster, time);
}

glm::vec3 PerspectiveCamera::rasterToWorld(const glm::vec3 &wsP, const PTime &time) const
{
	return transformPoint(wsP, m_raster_to_world, time);
}

bool PerspectiveCamera::canTransformNegativeCamZ() const
{
	return false;
}

void PerspectiveCamera::recomputeTransforms()
{
	Camera::recomputeTransforms();

	m_world_to_screen.resize(m_num_samples);
	m_screen_to_world.resize(m_num_samples);
	m_world_to_raster.resize(m_num_samples);
	m_raster_to_world.resize(m_num_samples);

	glm::mat4 camera_to_screen, screen_to_raster;

	for (unsigned int i = 0; i < m_num_samples; ++i) {
		PTime time(math::parametric(i, m_num_samples));
		getTransforms(time, camera_to_screen, screen_to_raster);

		m_world_to_screen[i] = m_world_to_cam[i] * camera_to_screen;
		m_screen_to_world[i] = glm::inverse(m_world_to_screen[i]);
		m_world_to_raster[i] = m_world_to_screen[i] * screen_to_raster;
		m_raster_to_world[i] = glm::inverse(m_world_to_raster[i]);
	}
}

void PerspectiveCamera::getTransforms(const PTime &time, glm::mat4 &camera_to_screen, glm::mat4 &screen_to_raster) const
{
	static_cast<void>(time);
	/* Standard projection matrix */
	auto perspective = glm::perspective(1.0f, 1.0f, m_near, m_far);

	/* field of view */
	auto fov_degrees = m_vert_fov.interpolate(0.0f);
	auto fov_radians = glm::radians(fov_degrees);
	auto inv_tan = 1.0f / glm::tan(fov_radians / 2.0f);
	auto aspect_ratio = static_cast<float>(m_resolution.x) / static_cast<float>(m_resolution.y);
	auto fov = glm::scale(glm::mat4(0.0f), glm::vec3(inv_tan / aspect_ratio, inv_tan, 1.0f));

	/* Build camera to screen matrix */
	camera_to_screen = perspective * fov;

	/* NDC to screen space */
	auto ndc_translate = glm::translate(glm::mat4(0.0f), glm::vec3(1.0f, 1.0f, 0.0f));
	auto ndc_scale = glm::scale(glm::mat4(0.0f), glm::vec3(0.5f, 0.5f, 0.0f));
	auto screen_to_ndc = ndc_translate * ndc_scale;

	/* Raster to NDC space */
	auto ndc_to_raster = glm::scale(glm::mat4(0.0f), glm::vec3(m_resolution.x, m_resolution.y, 1.0f));
//	auto raster_to_ndc = glm::inverse(ndc_to_raster);

	/* Build screen to raster matrix */
	screen_to_raster = screen_to_ndc * ndc_to_raster;
}

/* ************************** SphericalCamera **************************** */

glm::vec3 SphericalCamera::worldToScreen(const glm::vec3 &wsP, const PTime &time) const
{
	auto csP = worldToCamera(wsP, time);
	SphericalCoords sc = cartToSphere(csP);
	return glm::vec3(sc.longitude / glm::pi<float>(), sc.latitude / (glm::pi<float>() * 0.5f), sc.radius);
}

glm::vec3 SphericalCamera::screenToWorld(const glm::vec3 &ssP, const PTime &time) const
{
	SphericalCoords sc;
	sc.longitude = ssP.x * glm::pi<float>();
	sc.latitude = ssP.y * (glm::pi<float>() * 0.5f);
	sc.radius = ssP.z;
	auto csP = sphereToCart(sc);
	return cameraToWorld(csP, time);
}

glm::vec3 SphericalCamera::worldToRaster(const glm::vec3 &wsP, const PTime &time) const
{
	auto ssP = worldToScreen(wsP, time);
	auto rsP = glm::vec4(ssP, 0.0f) * m_screen_to_raster;
	return glm::vec3(rsP.x, rsP.y, rsP.z);
}

glm::vec3 SphericalCamera::rasterToWorld(const glm::vec3 &rsP, const PTime &time) const
{
	auto ssP = glm::vec4(rsP, 0.0f) * m_raster_to_screen;
	return screenToWorld(glm::vec3(ssP.x, ssP.y, ssP.z), time);
}

bool SphericalCamera::canTransformNegativeCamZ() const
{
	return true;
}

void SphericalCamera::recomputeTransforms()
{
	Camera::recomputeTransforms();

	/* NDC to screen space */
	auto ndc_translate = glm::translate(glm::mat4(0.0f), glm::vec3(1.0f, 1.0f, 0.0f));
	auto ndc_scale = glm::scale(glm::mat4(0.0f), glm::vec3(0.5f, 0.5f, 1.0f));
	auto screen_to_ndc = ndc_translate * ndc_scale;

	/* Raster to NDC space */
	auto ndc_to_raster = glm::scale(glm::mat4(0.0f), glm::vec3(1.0f / m_resolution.x, 1.0f / m_resolution.y, 1.0f));
//	auto raster_to_ndc = glm::inverse(ndc_to_raster);

	/* Build screen to raster matrix */
	m_screen_to_raster = screen_to_ndc * ndc_to_raster;
	m_raster_to_screen = glm::inverse(m_screen_to_raster);
}

SphericalCoords SphericalCamera::cartToSphere(const glm::vec3 &cc) const
{
	SphericalCoords sc;

	sc.radius = glm::sqrt(cc.x * cc.x + cc.y * cc.y + cc.z * cc.z);

	if (sc.radius == 0.0f) {
		return SphericalCoords();
	}

	sc.longitude = std::atan2(cc.x, cc.z);
	sc.latitude = glm::pi<float>() * 0.5f - glm::acos(cc.y / sc.radius);

	return sc;
}

glm::vec3 SphericalCamera::sphereToCart(const SphericalCoords &sc) const
{
	const auto rho = sc.radius;
	const auto theta = 0.5f * glm::pi<float>() - sc.latitude;
	const auto phi = sc.longitude;

	return glm::vec3(rho * glm::sin(phi) * glm::sin(theta),
	                 rho * glm::cos(theta),
	                 rho * glm::cos(phi) * glm::sin(theta));
}

float calculateVerticalFov(const float focalLength, const float aperture_h, const glm::ivec2 &resolution)
{
	auto aperture_v = (static_cast<float>(resolution.y) * aperture_h) / static_cast<float>(resolution.x);
	return 2.0f * glm::atan((aperture_v / 2.0f) * focalLength);
}

#if 0
Camera::Camera(glm::vec3 pos, glm::vec3 lookat, glm::vec3 up, float fov, float aspect)
{
	// create view vector
	glm::vec3 view = lookat - pos;
	// find right vector
	glm::vec3 right = glm::cross(view, up);
	// orthogonalise up vector
	glm::vec3 m_up = glm::cross(right, view);

	// scale up and right vectors
	m_up *= glm::tan(fov / 2.0f);
	right *= glm::tan(fov / 2.0f) * aspect;

	// adjust for image space normalisation
	view -= (right + m_up);
	right *= 2.0f;
	m_up *= 2.0f;

	m_origin = pos;
	m_transform = glm::mat3(right, u, v);
}

Camera::~Camera()
{}

Ray Camera::generate_ray(float x, float y)
{
	Ray ray;
	glm::vec3 p(x, y, 1);

	ray.orig = m_origin;
	ray.dir = glm::normalize(m_transform * p);

	return ray;
}
#endif

