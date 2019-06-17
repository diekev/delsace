
#include <chrono>

namespace dls {

namespace memoire {

template <typename rep, size_t ratio>
struct taille_memoire {
	rep r;
};

using     octet = taille_memoire<size_t, 1>;
using kilooctet = taille_memoire<size_t, 1024>;
using megaoctet = taille_memoire<size_t, 1024 * 1024>;
using gigaoctet = taille_memoire<size_t, 1024 * 1024 * 1024>;
using teraoctet = taille_memoire<size_t, 1024LL * 1024 * 1024 * 1024>;

}

namespace chr {

using point_temps = std::chrono::system_clock::time_point;

using ms = std::chrono::duration<double, std::milli>;

auto maintenant()
{
	return std::chrono::system_clock::now();
}

template <typename cloche, typename duree>
auto delta(const std::chrono::time_point<cloche, duree> &time_point)
{
	auto d = std::chrono::system_clock::now() - time_point;
	return std::chrono::duration_cast<ms>(d).count();
}

}
}  /* namespace dls */
