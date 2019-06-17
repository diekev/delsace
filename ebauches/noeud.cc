#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

template <typename T>
void imprime(const T &t, std::ostream &os)
{
	os << t;
}

class noeud;
class noeud_ligne;

using type_graphe = std::vector<noeud>;

class noeud {
	struct concept {
		virtual ~concept() = default;
		virtual void imprime_impl(std::ostream &os) const = 0;
	};

	template <typename T>
	struct concept_noeud : public concept {
		concept_noeud(T t)
			: donnees(std::move(t))
		{}

		void imprime_impl(std::ostream &os) const override
		{
			imprime(donnees, os);
		}

		T donnees;
	};

	std::shared_ptr<const concept> donnees;

public:
	noeud() = default;
	template <typename T>
	noeud(T t)
		: donnees(std::make_shared<concept_noeud<T>>(std::move(t)))
	{}

	friend std::ostream &operator<<(std::ostream &os, const noeud &n);
};

std::ostream &operator<<(std::ostream &os, const noeud &n)
{
	n.donnees->imprime_impl(os);
	return os;
}

void imprime(const type_graphe &graphe, std::ostream &os)
{
	for (const noeud &n : graphe) {
		os << n << '\n';
	}
}

class noeud_ligne {
public:
};

void imprime(const noeud_ligne &n, std::ostream &os)
{
	os << "noeud_ligne";
}

struct parametres_projet {
	int largeur;
	int hauteur;

	int espace_colorimetrique;
};

struct projet {
	parametres_projet params;
	std::vector<noeud> graphe;
};

using historique = std::vector<projet>;

struct mikisa {
	historique historique_projet;

	mikisa()
	{
		historique_projet.emplace_back();
	}
};

projet &courant(historique &h)
{
	assert(h.size());
	return h.back();
}

void annule(historique &h)
{
	assert(h.size());
	return h.pop_back();
}

void commet(historique &h)
{
	assert(h.size());
	return h.push_back(h.back());
}

class commande {
public:
	~commande() = default;

	void execute_pre(mikisa &m)
	{
		commet(m.historique_projet);

		execute(m);
	}

	void execute(mikisa &m)
	{
		auto &projet_courant = courant(m.historique_projet);

		projet_courant.graphe.push_back(1);
	}
};

template <typename T>
void enregistre_noeud(
	std::unordered_map<std::string, noeud> &noeuds,
	const std::string &nom,
	const T &valeur)
{
	noeuds.insert({nom, valeur});
}

noeud contruit_noeud(
	std::unordered_map<std::string, noeud> &noeuds,
	const std::string &nom)
{
	return noeuds[nom];
}

int main()
{
	std::unordered_map<std::string, noeud> noeuds;
	enregistre_noeud(noeuds, "int", 0);
	enregistre_noeud(noeuds, "noeud_ligne", noeud_ligne());

	std::vector<noeud> graphe;
	graphe.push_back(contruit_noeud(noeuds, "int"));
	graphe.push_back("valeur");
	graphe.push_back(contruit_noeud(noeuds, "noeud_ligne"));
	graphe.push_back(3);

	imprime(graphe, std::cout);
}
