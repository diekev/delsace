

struct NuanceurVolume : public Nuanceur {
	double densite = 1.0;
	Spectre sigma_a = Spectre(1.0);
	Spectre sigma_s = Spectre(1.0);
	Spectre absorption = Spectre(2.0);

	Spectre evalue(GNA &gna, const ParametresRendu &parametres, ContexteNuancage &contexte, uint profondeur) const override;

	static Nuanceur *defaut();

	Spectre transmittance(const ParametresRendu &parametres, const numero7::math::point3d &P0, const numero7::math::point3d &P1) const;
};

Spectre NuanceurVolume::evalue(GNA &/*gna*/, const ParametresRendu &parametres, ContexteNuancage &contexte, uint /*profondeur*/) const
{
	/* Lance un rayon pour définir le point de sortie du volume. */
	auto rayon_local = contexte.rayon;
	rayon_local.origine = contexte.P;

	auto isect = parametres.acceleratrice->intersecte(parametres.scene, rayon_local, 1000.0);

	if (isect.type_objet == OBJET_TYPE_AUCUN) {
		return Spectre(0.0);
	}

	auto P = rayon_local.origine + isect.distance * rayon_local.direction;

//	auto L = Spectre(0.0);
	auto T = transmittance(parametres, P, contexte.P);
//	auto w = Spectre(0.0);

	/* Met à jour le point d'où doit partir le rayon après l'évaluation. */
	contexte.P = P;

	return T;
}

Spectre NuanceurVolume::transmittance(
		const ParametresRendu &/*parametres*/,
		const numero7::math::point3d &P0,
		const numero7::math::point3d &P1) const
{
	auto taille = longueur(P0 - P1);

	auto tr = Spectre(1.0);
	tr[0] = std::exp(absorption[0] * -taille);
	tr[1] = std::exp(absorption[1] * -taille);
	tr[2] = std::exp(absorption[2] * -taille);
	return tr;
}

/* ****************************************************************** */

struct Temps {
    float valeur;
};

struct Courbe {
    std::vector<std::pair<const Temps, float>> valeurs;
};

bool operator<(const Courbe &c1, const Courbe &c2)
{
    return c1.first < c2.first;
}

class GrilleVolume {
    std::vector<Courbe> m_donnees;
    int m_res_x;
    int m_res_y;
    int m_res_z;

    size_t calcul_index(size_t x, size_t y, size_t z)
    {
        return x + (y + z * m_res_y) * m_res_x;
    }

public:
    void insert(int x, int y, int z, float valeur, const Temps &temps)
    {
        const auto index = calcul_index(x, y, z);
        m_donnees[index].valeurs.push_back(std::make_pair(temps, valeur));
    }

    void tri_donnees()
    {
        for (auto &courbe : m_donnees) {
            std::sort(courbe.valeurs.begin(), courbe.valeurs.end());
        }
    }

    float valeur(int x, int y, int z, const Temps &temps) const
    {
        const auto index = calcul_index(x, y, z);

        const auto &courbe = m_donnees[index];

        if (courbe.valeurs.size() == 0) {
            return 0.0f;
        }

        if (courbe.valeurs.size() == 1) {
            if (courbe.valeurs[0] < temps) {
                return 0.0f;
            }

            if (courbe.valeurs[0] > temps) {
                return 0.0f;
            }

            return 0.0f;
        }
    }
};
