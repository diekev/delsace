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

#include "compilation/syntaxeuse.hh"
#include "compilation/assembleuse_arbre.h"
#include "compilation/contexte_generation_code.h"
#include "compilation/lexeuse.hh"
#include "compilation/modules.hh"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>

#include "biblinternes/tests/test_aleatoire.hh"

namespace test_decoupage {

static int test_entree_aleatoire(const u_char *donnees, size_t taille)
{
	try {
		auto donnees_char = reinterpret_cast<const char *>(donnees);

		dls::chaine texte;
		texte.reserve(static_cast<long>(taille) + 1l);

		for (auto i = 0ul; i < taille; ++i) {
			texte.pousse(donnees_char[i]);
		}

		auto contexte = ContexteGenerationCode{};
		auto fichier = contexte.cree_fichier("", "");
		fichier->tampon = lng::tampon_source(texte);

		Lexeuse lexeuse(fichier);
		lexeuse.performe_lexage();

		auto assembleuse = assembleuse_arbre(contexte);
		contexte.assembleuse = &assembleuse;
		auto analyseuse = Syntaxeuse(contexte, fichier, "");

		std::ostream os(nullptr);
		analyseuse.lance_analyse(os);
	}
	catch (...) {

	}

	return 0;
}

} // namespace test_decoupage

namespace test_analyse {

static GenreLexeme sequence_declaration_fonction[] = {
	GenreLexeme::FONC,
	GenreLexeme::CHAINE_CARACTERE,
	GenreLexeme::PARENTHESE_OUVRANTE,
	GenreLexeme::PARENTHESE_FERMANTE,
	GenreLexeme::DOUBLE_POINTS,
	GenreLexeme::RIEN,
	GenreLexeme::ACCOLADE_OUVRANTE
};

namespace arbre_expression {

using visiteur_arbre = std::function<void(GenreLexeme)>;

static GenreLexeme id_operateurs_unaire[] = {
	/* on utilise PLUS et MOINS, et non PLUS_UNAIRE et MOINS_UNAIRE, pour
	 * pouvoir tester la détection des opérateurs unaires. */
	GenreLexeme::PLUS,
	GenreLexeme::MOINS,
	GenreLexeme::AROBASE,
	GenreLexeme::EXCLAMATION,
	GenreLexeme::TILDE,
	GenreLexeme::CROCHET_OUVRANT,
};

static GenreLexeme id_operateurs_binaire[] = {
	GenreLexeme::PLUS,
	GenreLexeme::MOINS,
	GenreLexeme::FOIS,
	GenreLexeme::DIVISE,
	GenreLexeme::ESPERLUETTE,
	GenreLexeme::POURCENT,
	GenreLexeme::INFERIEUR,
	GenreLexeme::INFERIEUR_EGAL,
	GenreLexeme::SUPERIEUR,
	GenreLexeme::SUPERIEUR_EGAL,
	GenreLexeme::DECALAGE_DROITE,
	GenreLexeme::DECALAGE_GAUCHE,
	GenreLexeme::DIFFERENCE,
	GenreLexeme::ESP_ESP,
	GenreLexeme::EGALITE,
	GenreLexeme::BARRE_BARRE,
	GenreLexeme::BARRE,
	GenreLexeme::CHAPEAU,
	GenreLexeme::EGAL,
	GenreLexeme::POINT,
};

static GenreLexeme id_variables[] = {
	GenreLexeme::CHAINE_CARACTERE,
	GenreLexeme::NOMBRE_BINAIRE,
	GenreLexeme::NOMBRE_ENTIER,
	GenreLexeme::NOMBRE_HEXADECIMAL,
	GenreLexeme::NOMBRE_OCTAL,
	GenreLexeme::NOMBRE_REEL,
	GenreLexeme::CARACTERE,
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
	expression *droite{};

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
	expression *gauche{};
	expression *droite{};

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
	expression *centre{};

	virtual void visite(visiteur_arbre visiteur) override;
};

void parenthese::visite(visiteur_arbre visiteur)
{
	visiteur(GenreLexeme::PARENTHESE_OUVRANTE);
	centre->visite(visiteur);
	visiteur(GenreLexeme::PARENTHESE_FERMANTE);
}

struct appel_fonction : public expression {
	dls::tableau<expression *> params{};

	virtual void visite(visiteur_arbre visiteur) override;
};

void appel_fonction::visite(visiteur_arbre visiteur)
{
	visiteur(GenreLexeme::CHAINE_CARACTERE);
	visiteur(GenreLexeme::PARENTHESE_OUVRANTE);

	for (auto enfant : params) {
		enfant->visite(visiteur);
	}

	visiteur(GenreLexeme::PARENTHESE_FERMANTE);
}

struct acces_tableau : public expression {
	expression *param{};

	virtual void visite(visiteur_arbre visiteur) override;
};

void acces_tableau::visite(visiteur_arbre visiteur)
{
	visiteur(GenreLexeme::CHAINE_CARACTERE);
	visiteur(GenreLexeme::CROCHET_OUVRANT);
	param->visite(visiteur);
	visiteur(GenreLexeme::CROCHET_FERMANT);
}

struct arbre {
	expression *racine{nullptr};
	dls::tableau<expression *> noeuds{};
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
		this->racine = construit_expression_ex(1.0, 0);
	}

	expression *construit_expression_ex(double prob, int profondeur)
	{
		auto p = this->rng(this->device) * prob;

		if (profondeur >= 32) {
			auto noeud = new variable{};
			this->noeuds.pousse(noeud);
			return noeud;
		}

		if (p > 0.5) {
			auto noeud = new parenthese{};
			noeud->centre = construit_expression_ex(prob / 1.2, profondeur + 1);
			this->noeuds.pousse(noeud);
			return noeud;
		}

		auto pi = static_cast<size_t>(this->rng(this->device) * 4);

		switch (pi) {
			case 0:
			{
				auto noeud = new variable{};
				this->noeuds.pousse(noeud);
				return noeud;
			}
			case 1:
			{
				auto noeud = new operation_unaire{};
				noeud->droite = construit_expression_ex(prob / 1.2, profondeur + 1);
				this->noeuds.pousse(noeud);
				return noeud;
			}
			case 2:
			{
				auto noeud = new appel_fonction{};

				auto n = static_cast<size_t>(this->rng(this->device) * 10);

				for (auto i = 0ul; i < n; ++i) {
					/* construction d'une nouvelle expression, donc réinitialise prob */
					auto enfant = construit_expression_ex(1.0, profondeur + 1);
					noeud->params.pousse(enfant);
				}

				this->noeuds.pousse(noeud);
				return noeud;
			}
			case 3:
			{
				auto noeud = new acces_tableau{};
				noeud->param = construit_expression_ex(prob / 1.2, profondeur + 1);
				this->noeuds.pousse(noeud);
				return noeud;
			}
			default:
			case 4:
			{
				auto noeud = new operation_binaire{};
				noeud->droite = construit_expression_ex(prob / 1.2, profondeur + 1);
				noeud->gauche = construit_expression_ex(prob / 1.2, profondeur + 1);
				this->noeuds.pousse(noeud);
				return noeud;
			}
		}
	}
};

} // namespace arbre_expression

static void rempli_tampon(u_char *donnees, size_t taille_tampon)
{
#if 0
	auto const max_lexemes = taille_tampon / sizeof(DonneesLexeme);

	dls::tableau<DonneesLexeme> lexemes;
	lexemes.reserve(max_lexemes);

	auto dm = DonneesLexeme{};
	dm.chaine = "texte_test";
	dm.ligne_pos = 0ul;

	for (auto id : sequence_declaration_fonction) {
		dm.genre = id;
		lexemes.pousse(dm);
	}

	for (auto n = lexemes.taille(); n < max_lexemes - 1; ++n) {
		auto arbre = arbre_expression::arbre{};
		arbre.construit_expression();

		auto visiteur = [&](id_lexeme id)
		{
			dm.genre = static_cast<id_lexeme>(id);
			lexemes.pousse(dm);
		};

		arbre.visite(visiteur);

		dm.genre = id_lexeme::POINT_VIRGULE;
		lexemes.pousse(dm);

		n += arbre.noeuds.taille();
	}

	dm.genre = id_lexeme::ACCOLADE_FERMANTE;
	lexemes.pousse(dm);

	auto const taille_octet = sizeof(DonneesLexeme) * lexemes.taille();

	memcpy(donnees, lexemes.donnees(), std::min(taille_tampon, taille_octet));
#else
	auto const max_lexemes = taille_tampon / sizeof(GenreLexeme);

	dls::tableau<GenreLexeme> lexemes;
	lexemes.reserve(static_cast<long>(max_lexemes));

	for (auto id : sequence_declaration_fonction) {
		lexemes.pousse(id);
	}

	for (auto n = lexemes.taille(); n < static_cast<long>(max_lexemes) - 1; ++n) {
		auto arbre = arbre_expression::arbre{};
		arbre.construit_expression();

		auto visiteur = [&](GenreLexeme id)
		{
			lexemes.pousse(id);
		};

		arbre.visite(visiteur);

		lexemes.pousse(GenreLexeme::POINT_VIRGULE);

		n += arbre.noeuds.taille();
	}

	lexemes.pousse(GenreLexeme::ACCOLADE_FERMANTE);

	auto const taille_octet = sizeof(DonneesLexeme) * static_cast<size_t>(lexemes.taille());

	memcpy(donnees, lexemes.donnees(), std::min(taille_tampon, taille_octet));
#endif
}

static void rempli_tampon_aleatoire(u_char *donnees, size_t taille_tampon)
{
#if 0
	auto const max_lexemes = taille_tampon / sizeof(DonneesLexeme);

	dls::tableau<DonneesLexeme> lexemes;
	lexemes.reserve(max_lexemes);

	std::random_device device{};
	std::uniform_int_distribution<u_char> rng{
		static_cast<int>(id_lexeme::EXCLAMATION),
		static_cast<int>(id_lexeme::INCONNU)
	};

	auto dm = DonneesLexeme{};
	dm.chaine = "texte_test";
	dm.ligne_pos = 0ul;

	for (auto id : sequence_declaration_fonction) {
		dm.genre = id;
		lexemes.pousse(dm);
	}

	for (auto n = lexemes.taille(); n < max_lexemes - 1; ++n) {
		dm.genre = static_cast<id_lexeme>(rng(device));
		lexemes.pousse(dm);
	}

	dm.genre = id_lexeme::ACCOLADE_FERMANTE;
	lexemes.pousse(dm);

	auto const taille_octet = sizeof(DonneesLexeme) * lexemes.taille();

	memcpy(donnees, lexemes.donnees(), std::min(taille_tampon, taille_octet));
#else
	auto const max_lexemes = taille_tampon / sizeof(GenreLexeme);

	std::random_device device{};
	std::uniform_int_distribution<u_char> rng{
		static_cast<int>(GenreLexeme::EXCLAMATION),
		static_cast<int>(GenreLexeme::INCONNU)
	};

	dls::tableau<GenreLexeme> lexemes;
	lexemes.reserve(static_cast<long>(max_lexemes));

	for (auto id : sequence_declaration_fonction) {
		lexemes.pousse(id);
	}

	for (auto n = lexemes.taille(); n < static_cast<long>(max_lexemes) - 1; ++n) {
		lexemes.pousse(static_cast<GenreLexeme>(rng(device)));
	}

	lexemes.pousse(GenreLexeme::ACCOLADE_FERMANTE);

	auto const taille_octet = sizeof(DonneesLexeme) * static_cast<size_t>(lexemes.taille());

	memcpy(donnees, lexemes.donnees(), std::min(taille_tampon, taille_octet));
#endif
}

static int test_entree_aleatoire(const u_char *donnees, size_t taille)
{
	auto donnees_lexemes = reinterpret_cast<const GenreLexeme *>(donnees);
	auto nombre_lexemes = taille / sizeof(GenreLexeme);

	dls::tableau<DonneesLexeme> lexemes;
	lexemes.reserve(static_cast<long>(nombre_lexemes));

	auto dm = DonneesLexeme{};
	dm.chaine = "texte_test";
	dm.fichier = 0;

	for (size_t i = 0; i < nombre_lexemes; ++i) {
		dm.genre = donnees_lexemes[i];
		lexemes.pousse(dm);
	}

	try {
		auto contexte = ContexteGenerationCode{};
		auto fichier = contexte.cree_fichier("", "");
		fichier->tampon = lng::tampon_source("texte_test");
		fichier->lexemes = lexemes;
		auto assembleuse = assembleuse_arbre(contexte);
		contexte.assembleuse = &assembleuse;
		auto analyseuse = Syntaxeuse(contexte, fichier, "");

		std::ostream os(nullptr);
		analyseuse.lance_analyse(os);
	}
	catch (...) {

	}

	return 0;
}

} // namespace test_analyse

int main()
{
	dls::test_aleatoire::Testeuse testeuse;
	testeuse.ajoute_tests("analyse", test_analyse::rempli_tampon, test_analyse::test_entree_aleatoire);
	testeuse.ajoute_tests("analyse", test_analyse::rempli_tampon_aleatoire, test_analyse::test_entree_aleatoire);
	testeuse.ajoute_tests("decoupage", nullptr, test_decoupage::test_entree_aleatoire);

	return testeuse.performe_tests(std::cerr);
}
