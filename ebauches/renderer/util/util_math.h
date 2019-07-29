#include <glm/gtx/compatibility.hpp>

namespace math {

template <typename S, typename T>
S fit(const T &t, const T &min, const T &max, const S &new_min, const S &new_max)
{
	auto interp = glm::lerp(t, min, max);
	return glm::lerp(new_min, new_max, interp);
}

float parametric(const int i, const int num)
{
	return static_cast<float>(i) / static_cast<float>(num - 1);
}


}


