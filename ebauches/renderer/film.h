#include <glm/glm.hpp>
#include <OpenImageIO/imagebuf.h>
#include <memory>

class Film {
	OpenImageIO::ImageBuf m_ibuf;

public:
	using Ptr = std::shared_ptr<Film>;

	static Ptr create();
	Ptr clone();

	enum Channels {
		RGB,
		RGBA
	};

	void setSize(const size_t width, const size_t height);
	void setPixel(const size_t x, const size_t y, const glm::vec3 &value);
	void setAlpha(const size_t x, const size_t y, const float value);

	glm::vec2 size() const;
	glm::vec3 pixel(const size_t x, const size_t y) const;
	float alpha(const size_t x, const size_t y) const;
	void write(const std::string &filename, Channels channels) const;
};
