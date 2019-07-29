#include "renderer.h"

Renderer::Renderer()
	: m_film(Film::create())
{}

Renderer::Ptr Renderer::create()
{
	return Ptr(new Renderer);
}

Renderer::Ptr Renderer::clone() const
{
	Ptr renderer(new Renderer(*this));

	renderer->m_film = m_film->clone();
//	renderer->m_scene = m_scene->clone();

	return renderer;
}

void Renderer::setCamera(Camera::CPtr camera)
{
	m_camera = camera;

	auto res = m_camera->resolution();
	m_film = Film::create();
	m_film->setSize(res.x, res.y);
}

#if 0
Scene::Ptr Renderer::scene() const
{
	return m_scene;
}
#endif

size_t Renderer::numPixelSamples() const
{
	return 1;
}

void Renderer::execute()
{
	auto res = m_camera->resolution();

	for (int y = 0; y < res.y; ++y) {
		for (int x = 0; x < res.x; ++x) {
			m_film->setPixel(x, y, glm::vec3(1.0f, 0.0f, 0.0f));
			m_film->setAlpha(x, y, 1.0f);
		}
	}
}

void Renderer::saveImage(const std::string &filename) const
{
	m_film->write(filename, Film::RGBA);
}

