template <typename T>
struct pair_temps_valeur {
	T valeur;
	float temps;
};

struct Block {
	int offsets[N * N * N + 1];
	float *valeurs;
	float *temps;
};

tempalte <typename T>
class GrilleTemporelle {
	std::vector<pair_temps_valeur<T>> donnees{};
	T arrier_plan{};
	vec3f min;
	vec3f max;
	float taille_voxel{};

public:
	GrilleTemporelle()
		: min()
		, max()
		, taille_voxel()
		, arriere_plan()
	{}

	/* insertion */

	void insere(float x, float y, float z, float t, T valeur)
	{
		auto const ix = ((x - min.x) / (max.x - min.x)) / taille_voxel;
		auto const iy = ((y - min.y) / (max.y - min.y)) / taille_voxel;
		auto const iz = ((z - min.z) / (max.z - min.z)) / taille_voxel;

		auto index = ix + iy * rx + iz * rx * ry;

		donnees[index].push_back(pair_temps_valeur{valeur, t});
	}

	void insere(vec3f const &co, float t, T valeur)
	{
		insere(co.x, co.y, co.z, t, valeur);
	}

	void insere(vec4f const &co_t, T valeur)
	{
		insere(co_t.x, co_t.y, co_t.z, co_t.w, valeur);
	}

	/* accession */

	T valeur(float x, float y, float z, float t) const
	{
		auto const ix = ((x - min.x) / (max.x - min.x)) / taille_voxel;
		auto const iy = ((y - min.y) / (max.y - min.y)) / taille_voxel;
		auto const iz = ((z - min.z) / (max.z - min.z)) / taille_voxel;

		auto index = ix + iy * rx + iz * rx * ry;

		auto const &valeurs = donnees[index];

		if (valeurs.empty()) {
			return arriere_plan;
		}

		if (valeurs.size() == 1) {
			return valeurs[0].valeur;
		}

		if (t <= valeurs[0]) {
			return valeurs[0].valeur;
		}

		if (t >= valeurs[valeurs.size() - 1]) {
			return valeurs[valeurs.size() - 1].valeur;
		}

		auto v = 0.0f;

		for (size_t i = 1; i < valeurs.size(); ++i) {
			auto const &p0 = valeurs[i - 1];
			auto const &p1 = valeurs[i];

			if (p0.temps >= t && t <= p1.temps) {
				auto fac = (t - p0.temps) / (p1.temps - p0.temps);
				v = p0.valeur * fac + p1.valeur * (1.0f * fac);
				break;
			}
		}

		return v;
	}

	T valeur(vec3f const &co, float t) const
	{
		return valeur(co.x, co.y, co.z, t);
	}

	T valeur(vec4f const &co_t) const
	{
		return valeur(co_t.x, co_t.y, co_t.z, co_t.w);
	}
};
