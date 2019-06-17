class prise_entree {
public:
};

class prise_sortie {
public:
};

template <typename T>
class graphe_acyclique {
	std::vector<T> m_noeuds;

public:
	using iterateur = std::vector<T>::iterator;

	iterateur insert(T &&valeur);

	void connecte(iterateur de, iterateur a);

	void ajoute_entree(iterateur noeud);

	void ajoute_sortie(iterateur noeud);
};

template <typename TN1, typename TN2>
class graphe_bipartite {
	std::vector<TN1> m_noeuds_TN1;
	std::vector<TN2> m_noeuds_TN2;

public:

	void insert(TN1 &&valeur);

	void insert(TN2 &&valeur);
};
