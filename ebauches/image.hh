
class Tampon {

};

struct Carreau {
	void *tampon;
	int res_x = 0;
	int res_y = 0;
};

static constexpr auto TAILLE_CARREAU = 32;

class Image {
	dls::tableau<Carreau> m_carreaux{};
	long res_x = 0;
	long res_y = 0;

public:
	Image() = delete;

	Image(long x, long y)
	{
		auto const nombre_carreaux_x = x / TAILLE_CARREAU;
		auto const nombre_carreaux_y = y / TAILLE_CARREAU;

		auto const nombre_carreaux = nombre_carreaux_x * nombre_carreaux_y;
	}

	Carreau carreau(long x, long y)
	{
		auto const index_x = x / TAILLE_CARREAU;
		auto const index_y = y / TAILLE_CARREAU;
		auto const index = index_x + index_y * nombre_carreaux_y;

		return m_carreaux[index];
	}

	long nombre_carreau()
	{
		return m_carreaux.taille();
	}
};


