

struct Allocatrice {
	struct Noeud {
		Noeud *suivant = nullptr;
		long taille = 0;
	};

	Noeud *racine = nullptr;

	void *loge(long taille)
	{
		if (taille < 0) {
			return nullptr;
		}

		if (racine != nullptr) {
			auto ptr = racine;

			while (ptr && ptr->taille < taille) {
				ptr = ptr->suivant;
			}

			racine = racine->suivant;
			return ptr;
		}

		return malloc(static_cast<size_t>(taille));
	}

	void reloge(void *ptr, long ancienne_taille, long nouvelle_taille);

	void deloge(void *&ptr, long &taille)
	{
		auto noeud = static_cast<Noeud *>(ptr);
		noeud->taille = taille;
		noeud->suivant = racine;
		racine = noeud;

        ptr = nullptr;
        taille = 0;
	}
};

struct ChaineAllouee {
private:
    char *m_pointeur = nullptr;
    Allocatrice *m_allocatrice = nullptr;
    long m_taille = 0;
    long m_capacite = 0;

public:
    ChaineAllouee() = delete;

    ChaineAllouee(Allocatrice *allocatrice)
        : m_allocatrice(allocatrice)
    {}

    ChaineAllouee(ChaineAllouee const &autre)
    {
        if (m_pointeur) {
            m_allocatrice->deloge(m_pointeur, m_capacite);
        }

        if (!autre.m_pointeur) {
            return;
        }

        m_pointeur = m_allocatrice->loge(autre.m_taille);
    }

    long taille() const
    {
        return m_taille;
    }

    long capacite() const
    {
        return m_capacite;
    }

    const char *c_str() const
    {
        return m_pointeur;
    }

    long reserve(long taille) const
    {
        m_pointeur = m_allocatrice->loge(taille);
    }
};
