#include "renderglobals.h"

void RenderGlobals::setupMotionBlur(const float fps, const float shutter)
{
	m_fps = fps;
	m_shutter = shutter;
	m_dt = m_shutter / m_fps;
}

void RenderGlobals::setScene(ScenePtr scene)
{
	m_scene = scene;
}

void RenderGlobals::setCamera(CameraPtr camera)
{
	m_camera = camera;
}

float RenderGlobals::fps()
{
	return m_fps;
}

float RenderGlobals::shutter()
{
	return m_shutter;
}

float RenderGlobals::dt()
{
	return m_dt;
}

ScenePtr RenderGlobals::scene()
{
	return m_scene;
}

CameraPtr RenderGlobals::camera()
{
	return m_camera;
}

