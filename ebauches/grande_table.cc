#include <string>
#include <vector>

enum TypeDonnees {
	TYPE_CARACTERE,
	TYPE_ENTIER16,
	TYPE_ENTIER32,
	TYPE_ENTIER64,
	TYPE_DECIMAL32,
	TYPE_DECIMAL64,
	TYPE_UTF8,
	TYPE_UTF16,
	TYPE_UTF32,
};

char taille_octet_donnee(TypeDonnees donnees)
{
	switch (donnees) {
		case TYPE_UTF8:
		case TYPE_CARACTERE:
			return 1;
		case TYPE_UTF16:
		case TYPE_ENTIER16:
			return 2;
		case TYPE_UTF32:
		case TYPE_ENTIER32:
		case TYPE_DECIMAL32:
			return 4;
		case TYPE_ENTIER64:
		case TYPE_DECIMAL64:
			return 8;
	}

	return -1;
}

struct colonne {
	std::string nom;

	TypeDonnees type_donnees;

	int taille;

	/* nombre d'octet depuis le début de la table. */
	int decalage;
};

struct table {
	std::string nom;
	std::vector<colonne> colonnes;
};

struct base_donnees {
	std::vector<table> tables;
};

void ajoute_table(base_donnees &bdd, const std::string &nom, const std::vector<colonne> &colonnes)
{
	bdd.tables.push_back({nom, colonnes});
}

void ajoute_colonne(std::vector<colonne> &colonnes,
                    const std::string &nom,
                    TypeDonnees type_donnees,
                    int taille)
{
	colonne c;
	c.nom = nom;
	c.type_donnees = type_donnees;
	c.taille = taille;
	c.decalage = 0;

	colonnes.push_back(c);
}

int main()
{
	std::vector<colonne> colonnes;

	ajoute_colonne(colonnes, "id", TypeDonnees::TYPE_ENTIER32, 1);
	ajoute_colonne(colonnes, "nom", TypeDonnees::TYPE_CARACTERE, 255);
	ajoute_colonne(colonnes, "prenom", TypeDonnees::TYPE_CARACTERE, 255);
	ajoute_colonne(colonnes, "courriel", TypeDonnees::TYPE_CARACTERE, 255);
	ajoute_colonne(colonnes, "mdp", TypeDonnees::TYPE_CARACTERE, 60);
	ajoute_colonne(colonnes, "date_naissance", TypeDonnees::TYPE_ENTIER32, 1);

	ajoute_table(bdd, "utilisateurs", colonnes);
}
