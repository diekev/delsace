
#include <list>

namespace delsace {
struct empreinte_string_view {
	static size_t empreinte(const std::string_view &chaine)
	{
		size_t empreinte = 5381;

		for (const auto c : chaine) {
			empreinte = ((empreinte << 5) + empreinte) + static_cast<size_t>(c);
		}

		return empreinte;
	}
};

template <
	typename Cle,
	typename Valeur,
	typename FonctionEmpreinte
>
class table_hachage {
	static constexpr auto TAILLE_TABLE = 19997;

	std::list<std::pair<Cle, Valeur>> m_table[TAILLE_TABLE];
	size_t m_taille{0};

public:
	size_t taille() const
	{
		return m_taille;
	}

	size_t compte_alveole() const
	{
		return 19997;
	}

	double facteur_charge() const
	{
		return this->taille() / static_cast<double>(this->compte_alveole());
	}

	void insert(const std::pair<Cle, Valeur> &paire)
	{
		const auto empreinte = FonctionEmpreinte::empreinte(paire.first);
		const auto index = empreinte % TAILLE_TABLE;

		m_table[index].push_back(paire);
	}

	bool trouve(const Cle &cle) const
	{
		const auto empreinte = FonctionEmpreinte::empreinte(cle);
		const auto index = empreinte % TAILLE_TABLE;

		if (m_table[index].empty()) {
			return false;
		}

		for (const auto &paire : m_table[index]) {
			if (paire.first != cle) {
				continue;
			}

			return true;
		}

		return false;
	}

	const Valeur &operator[](const Cle &cle) const
	{
		const auto empreinte = FonctionEmpreinte::empreinte(cle);
		const auto index = empreinte % TAILLE_TABLE;

		if (m_table[index].empty()) {
			throw "Impossible de trouver la clé dans la table !";
		}

		for (const auto &paire : m_table[index]) {
			if (paire.first != cle) {
				continue;
			}

			return paire.second;
		}

		throw "Impossible de trouver la clé dans la table !";
	}
};
}
