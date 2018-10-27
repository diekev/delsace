/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <llvm/IR/Module.h>

#include "analyseuse_grammaire.h"
#include "contexte_generation_code.h"
#include "decoupeuse.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <sys/wait.h>
#include <unistd.h>

namespace test_decoupage {

static int test_entree_aleatoire(const u_char *donnees, size_t taille)
{
	try {
		auto donnees_char = reinterpret_cast<const char *>(donnees);

		std::string texte;
		texte.reserve(static_cast<size_t>(taille) + 1);

		for (auto i = 0ul; i < taille; ++i) {
			texte.push_back(donnees_char[i]);
		}

		auto tampon = TamponSource(texte);

		decoupeuse_texte decoupeuse(tampon);
		decoupeuse.genere_morceaux();

		auto contexte = ContexteGenerationCode{tampon};
		auto assembleuse = assembleuse_arbre();
		auto analyseuse = analyseuse_grammaire(contexte, decoupeuse.morceaux(), tampon, &assembleuse);
		analyseuse.lance_analyse();
	}
	catch (...) {

	}

	return 0;
}

} // namespace test_decoupage

namespace test_analyse {

static id_morceau sequence_declaration_fonction[] = {
	id_morceau::FONCTION,
	id_morceau::CHAINE_CARACTERE,
	id_morceau::PARENTHESE_OUVRANTE,
	id_morceau::PARENTHESE_FERMANTE,
	id_morceau::DOUBLE_POINTS,
	id_morceau::RIEN,
	id_morceau::ACCOLADE_OUVRANTE
};

namespace arbre_expression {

using visiteur_arbre = std::function<void(id_morceau)>;

static id_morceau id_operateurs_unaire[] = {
	/* on utilise PLUS et MOINS, et non PLUS_UNAIRE et MOINS_UNAIRE, pour
	 * pouvoir tester la détection des opérateurs unaires. */
	id_morceau::PLUS,
	id_morceau::MOINS,
	id_morceau::AROBASE,
	id_morceau::EXCLAMATION,
	id_morceau::TILDE,
	id_morceau::CROCHET_OUVRANT,
};

static id_morceau id_operateurs_binaire[] = {
	id_morceau::PLUS,
	id_morceau::MOINS,
	id_morceau::FOIS,
	id_morceau::DIVISE,
	id_morceau::ESPERLUETTE,
	id_morceau::POURCENT,
	id_morceau::INFERIEUR,
	id_morceau::INFERIEUR_EGAL,
	id_morceau::SUPERIEUR,
	id_morceau::SUPERIEUR_EGAL,
	id_morceau::DECALAGE_DROITE,
	id_morceau::DECALAGE_GAUCHE,
	id_morceau::DIFFERENCE,
	id_morceau::ESP_ESP,
	id_morceau::EGALITE,
	id_morceau::BARRE_BARRE,
	id_morceau::BARRE,
	id_morceau::CHAPEAU,
	id_morceau::DE,
	id_morceau::EGAL,
};

static id_morceau id_variables[] = {
	id_morceau::CHAINE_CARACTERE,
	id_morceau::NOMBRE_BINAIRE,
	id_morceau::NOMBRE_ENTIER,
	id_morceau::NOMBRE_HEXADECIMAL,
	id_morceau::NOMBRE_OCTAL,
	id_morceau::NOMBRE_REEL,
	id_morceau::CARACTERE,
};

struct expression {
	virtual ~expression();
	virtual void visite(visiteur_arbre visiteur) = 0;
};

expression::~expression() {}

struct variable : public expression {
	virtual void visite(visiteur_arbre visiteur) override;
};

void variable::visite(visiteur_arbre visiteur)
{
	std::random_device device{};
	std::uniform_int_distribution<int> dist{0, sizeof(*id_variables) - 1};

	visiteur(id_variables[dist(device)]);
}

struct operation_unaire : public expression {
	expression *droite;

	virtual void visite(visiteur_arbre visiteur) override;
};

void operation_unaire::visite(visiteur_arbre visiteur)
{
	std::random_device device{};
	std::uniform_int_distribution<int> dist{0, sizeof(*id_operateurs_unaire) - 1};

	visiteur(id_operateurs_unaire[dist(device)]);
	droite->visite(visiteur);
}

struct operation_binaire : public expression {
	expression *gauche;
	expression *droite;

	virtual void visite(visiteur_arbre visiteur) override;
};

void operation_binaire::visite(visiteur_arbre visiteur)
{
	std::random_device device{};
	std::uniform_int_distribution<int> dist{0, sizeof(*id_operateurs_binaire) - 1};

	gauche->visite(visiteur);
	visiteur(id_operateurs_binaire[dist(device)]);
	droite->visite(visiteur);
}

struct parenthese : public expression {
	expression *centre;

	virtual void visite(visiteur_arbre visiteur) override;
};

void parenthese::visite(visiteur_arbre visiteur)
{
	visiteur(id_morceau::PARENTHESE_OUVRANTE);
	centre->visite(visiteur);
	visiteur(id_morceau::PARENTHESE_FERMANTE);
}


struct appel_fonction : public expression {
	expression *param;

	virtual void visite(visiteur_arbre visiteur) override;
};

void appel_fonction::visite(visiteur_arbre visiteur)
{
	visiteur(id_morceau::CHAINE_CARACTERE);
	visiteur(id_morceau::PARENTHESE_OUVRANTE);
	param->visite(visiteur);
	visiteur(id_morceau::PARENTHESE_FERMANTE);
}

struct arbre {
	expression *racine{nullptr};
	std::vector<expression *> noeuds{};
	std::random_device device{};
	std::uniform_real_distribution<double> rng{0.0, 1.0};

	~arbre()
	{
		for (auto n : noeuds) {
			delete n;
		}
	}

	void visite(visiteur_arbre visiteur)
	{
		this->racine->visite(visiteur);
	}

	void construit_expression()
	{
		this->racine = construit_expression_ex(1.0);
	}

	expression *construit_expression_ex(double prob)
	{
		auto p = this->rng(this->device);

		/* À FAIRE :
		 * - opérateur[]
		 * - nombre aléatoire de paramètres de fonctions
		 */

		if (p > prob) {
			auto noeud = new variable{};
			this->noeuds.push_back(noeud);
			return noeud;
		}

		p = this->rng(this->device);

		if (p > 0.5) {
			auto noeud = new operation_unaire{};
			noeud->droite = construit_expression_ex(prob / 1.2);
			this->noeuds.push_back(noeud);
			return noeud;
		}

		p = this->rng(this->device);

		if (p > 0.5) {
			auto noeud = new parenthese{};
			noeud->centre = construit_expression_ex(prob / 1.2);
			this->noeuds.push_back(noeud);
			return noeud;
		}

		if (p > 0.5) {
			auto noeud = new appel_fonction{};
			/* construction d'une nouvelle expression, donc réinitialise prob */
			noeud->param = construit_expression_ex(1.0);
			this->noeuds.push_back(noeud);
			return noeud;
		}

		auto noeud = new operation_binaire{};
		noeud->droite = construit_expression_ex(prob / 1.2);
		noeud->gauche = construit_expression_ex(prob / 1.2);
		this->noeuds.push_back(noeud);
		return noeud;
	}
};

} // namespace arbre_expression

static void rempli_tampon(u_char *donnees, size_t taille_tampon)
{
	const auto max_morceaux = taille_tampon / sizeof(DonneesMorceaux);

	std::vector<DonneesMorceaux> morceaux;
	morceaux.reserve(max_morceaux);

	auto dm = DonneesMorceaux{};
	dm.chaine = "texte_test";
	dm.ligne_pos = 0ul;

	for (auto id : sequence_declaration_fonction) {
		dm.identifiant = id;
		morceaux.push_back(dm);
	}

	for (auto n = morceaux.size(); n < max_morceaux - 1; ++n) {
		auto arbre = arbre_expression::arbre{};
		arbre.construit_expression();

		auto visiteur = [&](id_morceau id)
		{
			dm.identifiant = static_cast<id_morceau>(id);
			morceaux.push_back(dm);
		};

		arbre.visite(visiteur);

		dm.identifiant = id_morceau::POINT_VIRGULE;
		morceaux.push_back(dm);

		n += arbre.noeuds.size();
	}

	dm.identifiant = id_morceau::ACCOLADE_FERMANTE;
	morceaux.push_back(dm);

	const auto taille_octet = sizeof(DonneesMorceaux) * morceaux.size();

	memcpy(donnees, morceaux.data(), std::min(taille_tampon, taille_octet));
}

static int test_entree_aleatoire(const u_char *donnees, size_t taille)
{
	auto donnees_morceaux = reinterpret_cast<const DonneesMorceaux *>(donnees);
	auto nombre_morceaux = taille / sizeof(DonneesMorceaux);

	std::vector<DonneesMorceaux> morceaux;
	morceaux.reserve(nombre_morceaux);

	for (size_t i = 0; i < nombre_morceaux; ++i) {
		morceaux.push_back(donnees_morceaux[i]);
	}

	try {
		auto tampon = TamponSource("texte_test");

		auto contexte = ContexteGenerationCode{tampon};
		auto assembleuse = assembleuse_arbre();
		auto analyseuse = analyseuse_grammaire(contexte, morceaux, tampon, &assembleuse);
		analyseuse.lance_analyse();
	}
	catch (...) {

	}

	return 0;
}

} // namespace test_analyse

using t_fonction_initialisation = std::function<void(u_char *, size_t)>;
using t_fonction_entree_test = std::function<int(const u_char *, size_t)>;

struct FonctionsTest {
	std::string nom;
	t_fonction_initialisation initialisation = nullptr;
	t_fonction_entree_test entree_test = nullptr;
};

struct Testeur {
	std::vector<FonctionsTest> fonctions;

	void ajoute_tests(
			const std::string &nom,
			t_fonction_initialisation initialisation,
			t_fonction_entree_test entree_test)
	{
		FonctionsTest foncs;
		foncs.nom = nom;
		foncs.initialisation = initialisation;
		foncs.entree_test = entree_test;

		fonctions.push_back(foncs);
	}
};

int main()
{
#if 1
	auto chemin = std::string("/tmp/test_");

	Testeur testeur;
	testeur.ajoute_tests("analyse", test_analyse::rempli_tampon, test_analyse::test_entree_aleatoire);
	testeur.ajoute_tests("decoupage", nullptr, test_decoupage::test_entree_aleatoire);

	std::random_device device{};
	std::uniform_int_distribution<u_char> rng{0, 255};
	std::uniform_int_distribution<size_t> rng_taille{32 * 1024, 64 * 1024};

	u_char tampon[64 * 1024];

	for (const auto &foncs : testeur.fonctions) {
		for (auto n = 0; n < 10; ++n) {
			size_t taille = rng_taille(device);

			if (foncs.initialisation) {
				foncs.initialisation(tampon, taille);
			}
			else {
				for (auto i = 0ul; i < taille; ++i) {
					tampon[i] = rng(device);
				}
			}

			auto pid = fork();

			if (pid == 0) {
				return foncs.entree_test(tampon, taille);
			}
			else if (pid > 0) {
				while (true) {
					int status;
					pid_t result = waitpid(pid, &status, WNOHANG);

					if (result == 0) {
						/* L'enfant est toujours en vie, continue. */
					}
					else if (result == -1) {
						std::cerr << "Erreur lors de l'attente\n";
						break;
					}
					else {
						if (!WIFEXITED(status)) {
							auto chemin_test = chemin + foncs.nom + std::to_string(n) + ".bin";
							std::ofstream of;
							of.open(chemin_test.c_str());
							of.write(reinterpret_cast<const char *>(tampon), static_cast<long>(taille));

							std::cerr << "Enfant a échoué : écriture du fichier...\n";
						}

						break;
					}
				}
			}
		}
	}
#else
	std::ifstream fichier("/tmp/test2");

	fichier.seekg(0, fichier.end);
	const auto taille_fichier = static_cast<size_t>(fichier.tellg());
	fichier.seekg(0, fichier.beg);

	char *donnees = new char[taille_fichier];

	fichier.read(donnees, static_cast<long>(taille_fichier));

	auto donnees_morceaux = reinterpret_cast<const DonneesMorceaux *>(donnees);
	auto nombre_morceaux = taille_fichier / sizeof(DonneesMorceaux);

	std::vector<DonneesMorceaux> morceaux;
	morceaux.reserve(nombre_morceaux);

	for (size_t i = 0; i < nombre_morceaux; ++i) {
		auto dm = donnees_morceaux[i];
		/* rétabli une chaîne car nous une décharge de la mémoire, donc les
		 * pointeurs sont mauvais. */
		dm.chaine = "texte_test";
		morceaux.push_back(dm);
	}

	std::cerr << "Il y a " << nombre_morceaux << " morceaux.\n";

	auto tampon = TamponSource("texte_test");

	try {
		auto contexte = ContexteGenerationCode{tampon};
		auto assembleuse = assembleuse_arbre();
		auto analyseuse = analyseuse_grammaire(contexte, morceaux, tampon, &assembleuse);
		analyseuse.lance_analyse();
	}
	catch (...) {

	}

	delete [] donnees;
#endif

	return 0;
}
