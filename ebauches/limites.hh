template <typename T>
struct limites {
	T min;
	T max;

	etend();
	entresecte();
};

template <typename T>
bool se_chevauchent(limites<T> const &l1, limites<T> const &l2)
{
	return (l1.min <= l2.min && l2.min <= l1.max) || (l1.min <= l2.max && l2.max <= l1.max);
}

using limites3d = limites<vec3d>;
using limites2d = limites<vec2d>;
using limites1d = limites<float>;

dls::math::rayon

dls::math::entresection

dls::math::limites

// axis aligned bounding box
// boite englobante

// Robust skin simulation
// http://graphics.pixar.com/library/SkinForIncredibles2/paper.pdf

struct ancre {
	vec3f co;
};

auto mat = mat3x2f{};

for (auto &a0 : ancres) {
	// position candidate a_etoile qui n'est pas usuellement sur la surface kinétique
	auto a_etoile = vec3f();

	auto d = a_etoile - a0;

	// triangle qui contient a0
	auto triangle = {};

	auto Dm = mat3x2(triangle.v1 - triangle.v0, triangle.v2 - triangle.v0);

	auto Q = mat3x2f{};
	auto R = mat2x2f{};

	decompose_QR(Dm, Q, R);

	d = inverse(R) * transpose(Q) * D;

	a_chapeau = inverse(R) * transpose(Q) * (a - triangle.v0);

	auto rayon = rayon2f;
	rayon.origin = a_chapeau;
	rayon.direction = d;

	// entresecte avec les trois cotés {(0,0), (0,1)}, {(0,0), (1,0)}, {(1,0), (0,1)}

	h_chapeau = {}; // point d'entresection

	// transforme espace mondiale
	h = Dm * h_chapeau + triangle.v0;
}
