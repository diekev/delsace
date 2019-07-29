#ifndef __RENDERGLOBALS_H__
#define __RENDERGLOBALS_H__

#include <memory>

class Camera;
class Scene;

using ScenePtr = std::shared_ptr<const Camera>;
using CameraPtr = std::shared_ptr<const Scene>;

class RenderGlobals {
	static float m_fps;
	static float m_shutter;
	static float m_dt;
	static ScenePtr m_scene;
	static CameraPtr m_camera;

public:
	static void setupMotionBlur(const float fps, const float shutter);
	static void setScene(ScenePtr scene);
	static void setCamera(CameraPtr camera);

	static float fps();
	static float shutter();
	static float dt();
	static ScenePtr scene();
	static CameraPtr camera();
};

#endif // __RENDERGLOBALS_H__
