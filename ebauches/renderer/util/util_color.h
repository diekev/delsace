#ifndef __UTIL_COLOR_H__
#define __UTIL_COLOR_H__

#include <glm/glm.hpp>

constexpr auto gamma_exp = 1.0f / 2.4f;

float linear_to_srgb(float c)
{
	if (c < 0.0031308f) {
		return (c < 0.0f) ? 0.0f : c * 12.92f;
	}
	else {
		return 1.055f * glm::pow(c, gamma_exp) - 0.055f;
	}
}

#endif /* __UTIL_COLOR_H__ */

