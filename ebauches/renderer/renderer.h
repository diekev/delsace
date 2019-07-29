#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <memory>

#include "camera.h"
#include "film.h"

class Scene;

class Renderer {
	Film::Ptr m_film;
	//Scene::Ptr m_scene;
	Camera::CPtr m_camera;

public:
	using Ptr = std::shared_ptr<Renderer>;

	Renderer();
	static Ptr create();
	Ptr clone() const;

	void setCamera(Camera::CPtr camera);

	/* Pointer to the current scene */
	//Scene::Ptr scene() const;

	size_t numPixelSamples() const;

	void execute();
	void saveImage(const std::string &filename) const;
};

#endif // __RENDERER_H__

