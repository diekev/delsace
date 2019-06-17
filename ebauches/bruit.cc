

template <typename T, size_t... Ns>
class vecteur;

template <typename T, size_t... Ns>
struct etat_bruit {
	static const unsigned int N = 128;
	int table_perm[N];
	int table_originelle[N];
	vecteur<T, Ns...> base[N];
	vecteur<T, Ns...> base_originelle[N];
	T taux_tournoiement[N];
	vecteur<T, Ns...> axe_tournoiement[N];
};

template <int DIM>
float bruit(float co[DIM])
{
	float fco[DIM];
	int ico[DIM];
	float rco[DIM];
	float sco[DIM];

	for (int i = 0; i < DIM; ++i) {
		// find cube that contains point
		fco[i] = std::floor(co[i]);
		ico[i] = static_cast<int>(fco[i]);

		// find relative co of point in cube
		rco[i] = co[i] - fco[i];
		// compute fade curves for each of co
		sco[i] = fade(rco[i]);
	}

	if constexpr (DIM == 2) {
		const auto floorx = std::floor(x);
		const auto floory = std::floor(y);

		const auto i = static_cast<int>(floorx);
		const auto j = static_cast<int>(floory);

		const auto fx = x - floorx;
		const auto fy = y - floory;
		const auto sx = fade(fx);
		const auto sy = fade(fy);

		const auto &n00 = m_basis[hash_index(i,j)];
		const auto &n10 = m_basis[hash_index(i+1,j)];
		const auto &n01 = m_basis[hash_index(i,j+1)];
		const auto &n11 = m_basis[hash_index(i+1,j+1)];

		return interp_bilineaire(
					fx * n00[0] + fy * n00[1],
					(fx - 1) * n10[0] + fy * n10[1],
					fx * n01[0] + (fy - 1) * n01[1],
					(fx - 1) * n11[0] + (fy - 1) * n11[1],
					sx, sy);
	}
}
