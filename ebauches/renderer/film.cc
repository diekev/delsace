#include "film.h"
#include "util_color.h"

Film::Ptr Film::create()
{
	return Ptr(new Film);
}

Film::Ptr Film::clone()
{
	return Ptr(new Film(*this));
}

void Film::setSize(const size_t width, const size_t height)
{
	OpenImageIO::ImageSpec spec(width, height, 4, OpenImageIO::TypeDesc::FLOAT);
	spec.attribute("oiio:ColorSpace", "Linear");
	m_ibuf.reset(spec);
}

void Film::setPixel(const size_t x, const size_t y, const glm::vec3 &value)
{
	m_ibuf.setpixel(x, y, &value.x, 3);
}

void Film::setAlpha(const size_t x, const size_t y, const float value)
{
	float col[4];
	m_ibuf.getpixel(x, y, col, 3);
	col[3] = value;
	m_ibuf.setpixel(x, y, col, 4);
}

glm::vec2 Film::size() const
{
	return glm::vec2(m_ibuf.xmax() + 1, m_ibuf.ymax() + 1);
}

glm::vec3 Film::pixel(const size_t x, const size_t y) const
{
	glm::vec3 col;
	m_ibuf.getpixel(x, y, &col.x, 3);
	return col;
}

float Film::alpha(const size_t x, const size_t y) const
{
	float value[4];
	m_ibuf.getpixel(x, y, value, 4);
	return value[3];
}

void Film::write(const std::string &filename, Channels channels) const
{
	auto len = filename.size();

	if (filename.substr(len - 3, len) != "exr") {
		auto spec = m_ibuf.spec();
		spec.attribute("oiio:ColorSpace", "sRGB");

		OpenImageIO::ImageBuf buffer("", spec);

		for (int j = 0; j <= buffer.ymax(); ++j) {
			auto inv_j = buffer.ymax() - j;
			for (int i = 0; i <= buffer.xmax(); ++i) {
				float pixel[4];
				m_ibuf.getpixel(i, j, pixel, 4);

				for (int c = 0; c < 3; ++c) {
					pixel[c] = linear_to_srgb(pixel[c]);
				}

				pixel[3] = 1.0f;

				buffer.setpixel(i, inv_j, pixel);
			}
		}

		buffer.write(filename);
	}
	else {
		OpenImageIO::ImageBuf buffer("", m_ibuf.spec());

		for (int j = 0; j <= buffer.ymax(); ++j) {
			auto inv_j = buffer.ymax() - j;
			for (int i = 0; i <= buffer.xmax(); ++i) {
				float pixel[4];
				m_ibuf.getpixel(i, j, pixel, 4);
				buffer.setpixel(i, inv_j, pixel);
			}
		}

		buffer.write(filename);
	}

	static_cast<void>(channels);
}

