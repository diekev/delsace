#include "renderer.h"

int main()
{
	Renderer::Ptr renderer = Renderer::create();
	PerspectiveCamera::Ptr camera = PerspectiveCamera::create();

	renderer->setCamera(camera);
	renderer->execute();
	renderer->saveImage("/home/kevin/moeru_kirin.exr");
}

