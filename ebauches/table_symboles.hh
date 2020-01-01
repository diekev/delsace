// ébauche d'une table de symboles fait durant un courte lecture de :
// Basics of Compiler Design
// http://hjemmesider.diku.dk/~torbenm/Basics/basics_lulu2.pdf

enum TypeObjet {
	NUL,
	VARIABLE,
	MODULE,
	ENUMERATION,
	STRUCT,
	ENUM,
	UNION,
	FONCTION,
};

enum TypePortee {
	LOCALE,
	GLOBALE
};

struct InformationSymbol {
	TypeObjet type = TypeObjet::NUL;
	int index; // DonneesStructure, DonneesVariable, DonneesFonction
	noeud::base *declaration;
	TypePortee portee = TypePortee::LOCAL; // local, global
};

template <typename T>
struct Resultat {
	T resultat{};

	bool ok = false;
};

// une table de symbole par module, contenant toutes ces informations ?
// une table de symbole par fonction, gardée pour la validation et la génération de code ?
// une table de symbole par portée ?
// les portées doivent avoir des identifiants
struct TableSymbole {
	dls::tableau<InformationSymbole> symboles{}

	bool est_vide() const
	{
		return symboles.est_vide();
	}

	void attache(nom, objet)
	{
		// ajoute l'objet au début de la liste
	}

	// retourne un type somme pour aider à déterminer si nous avons trouver quelquechose
	Resultat<InformationSymbole> cherche(dls::vue_chaine_compacte const &nom)
	{
		// cherche à partir du début de la liste
		for (auto const &symbole : symboles) {
			if (symbole.nom == nom) {
				retourne symbole;
			}
		}

		return erreur;
	}

	void entre_portee()
	{
		// garde une copie de la liste
		pile_liste.pousse(objets)
	}

	void sors_portee()
	{
		objets = pile_liste.depile()
	}
};

// optimisations (avant la génération de code) :
// -- supression de code mort
// -- débouclement (tableau fixes, plages connues)
// -- évaluation d'expressions constantes
