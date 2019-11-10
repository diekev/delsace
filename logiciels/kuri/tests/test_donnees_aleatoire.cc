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

#include "decoupage/analyseuse_grammaire.h"
#include "decoupage/assembleuse_arbre.h"
#include "decoupage/contexte_generation_code.h"
#include "decoupage/decoupeuse.h"
#include "decoupage/modules.hh"

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
		auto module = contexte.cree_module("", "");
		module->tampon = lng::tampon_source(texte);

		decoupeuse_texte decoupeuse(module);
		decoupeuse.genere_morceaux();

		auto assembleuse = assembleuse_arbre(contexte);
		contexte.assembleuse = &assembleuse;
		auto analyseuse = analyseuse_grammaire(contexte, module, "");

		std::ostream os(nullptr);
		analyseuse.lance_analyse(os);
	}
	catch (...) {

	}

	return 0;
}

} // namespace test_decoupage

namespace test_analyse {

static id_morceau sequence_declaration_fonction[] = {
	id_morceau::FONC,
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
	id_morceau::POINT,
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
	visiteur(id_morceau::PARENTHESE_OUVRANTE);
	centre->visite(visiteur);
	visiteur(id_morceau::PARENTHESE_FERMANTE);
}

struct appel_fonction : public expression {
	dls::tableau<expression *> params{};

	virtual void visite(visiteur_arbre visiteur) override;
};

void appel_fonction::visite(visiteur_arbre visiteur)
{
	visiteur(id_morceau::CHAINE_CARACTERE);
	visiteur(id_morceau::PARENTHESE_OUVRANTE);

	for (auto enfant : params) {
		enfant->visite(visiteur);
	}

	visiteur(id_morceau::PARENTHESE_FERMANTE);
}

struct acces_tableau : public expression {
	expression *param{};

	virtual void visite(visiteur_arbre visiteur) override;
};

void acces_tableau::visite(visiteur_arbre visiteur)
{
	visiteur(id_morceau::CHAINE_CARACTERE);
	visiteur(id_morceau::CROCHET_OUVRANT);
	param->visite(visiteur);
	visiteur(id_morceau::CROCHET_FERMANT);
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
	auto const max_morceaux = taille_tampon / sizeof(DonneesMorceaux);

	dls::tableau<DonneesMorceaux> morceaux;
	morceaux.reserve(max_morceaux);

	auto dm = DonneesMorceaux{};
	dm.chaine = "texte_test";
	dm.ligne_pos = 0ul;

	for (auto id : sequence_declaration_fonction) {
		dm.identifiant = id;
		morceaux.pousse(dm);
	}

	for (auto n = morceaux.taille(); n < max_morceaux - 1; ++n) {
		auto arbre = arbre_expression::arbre{};
		arbre.construit_expression();

		auto visiteur = [&](id_morceau id)
		{
			dm.identifiant = static_cast<id_morceau>(id);
			morceaux.pousse(dm);
		};

		arbre.visite(visiteur);

		dm.identifiant = id_morceau::POINT_VIRGULE;
		morceaux.pousse(dm);

		n += arbre.noeuds.taille();
	}

	dm.identifiant = id_morceau::ACCOLADE_FERMANTE;
	morceaux.pousse(dm);

	auto const taille_octet = sizeof(DonneesMorceaux) * morceaux.taille();

	memcpy(donnees, morceaux.donnees(), std::min(taille_tampon, taille_octet));
#else
	auto const max_morceaux = taille_tampon / sizeof(id_morceau);

	dls::tableau<id_morceau> morceaux;
	morceaux.reserve(static_cast<long>(max_morceaux));

	for (auto id : sequence_declaration_fonction) {
		morceaux.pousse(id);
	}

	for (auto n = morceaux.taille(); n < static_cast<long>(max_morceaux) - 1; ++n) {
		auto arbre = arbre_expression::arbre{};
		arbre.construit_expression();

		auto visiteur = [&](id_morceau id)
		{
			morceaux.pousse(id);
		};

		arbre.visite(visiteur);

		morceaux.pousse(id_morceau::POINT_VIRGULE);

		n += arbre.noeuds.taille();
	}

	morceaux.pousse(id_morceau::ACCOLADE_FERMANTE);

	auto const taille_octet = sizeof(DonneesMorceaux) * static_cast<size_t>(morceaux.taille());

	memcpy(donnees, morceaux.donnees(), std::min(taille_tampon, taille_octet));
#endif
}

static void rempli_tampon_aleatoire(u_char *donnees, size_t taille_tampon)
{
#if 0
	auto const max_morceaux = taille_tampon / sizeof(DonneesMorceaux);

	dls::tableau<DonneesMorceaux> morceaux;
	morceaux.reserve(max_morceaux);

	std::random_device device{};
	std::uniform_int_distribution<u_char> rng{
		static_cast<int>(id_morceau::EXCLAMATION),
		static_cast<int>(id_morceau::INCONNU)
	};

	auto dm = DonneesMorceaux{};
	dm.chaine = "texte_test";
	dm.ligne_pos = 0ul;

	for (auto id : sequence_declaration_fonction) {
		dm.identifiant = id;
		morceaux.pousse(dm);
	}

	for (auto n = morceaux.taille(); n < max_morceaux - 1; ++n) {
		dm.identifiant = static_cast<id_morceau>(rng(device));
		morceaux.pousse(dm);
	}

	dm.identifiant = id_morceau::ACCOLADE_FERMANTE;
	morceaux.pousse(dm);

	auto const taille_octet = sizeof(DonneesMorceaux) * morceaux.taille();

	memcpy(donnees, morceaux.donnees(), std::min(taille_tampon, taille_octet));
#else
	auto const max_morceaux = taille_tampon / sizeof(id_morceau);

	std::random_device device{};
	std::uniform_int_distribution<u_char> rng{
		static_cast<int>(id_morceau::EXCLAMATION),
		static_cast<int>(id_morceau::INCONNU)
	};

	dls::tableau<id_morceau> morceaux;
	morceaux.reserve(static_cast<long>(max_morceaux));

	for (auto id : sequence_declaration_fonction) {
		morceaux.pousse(id);
	}

	for (auto n = morceaux.taille(); n < static_cast<long>(max_morceaux) - 1; ++n) {
		morceaux.pousse(static_cast<id_morceau>(rng(device)));
	}

	morceaux.pousse(id_morceau::ACCOLADE_FERMANTE);

	auto const taille_octet = sizeof(DonneesMorceaux) * static_cast<size_t>(morceaux.taille());

	memcpy(donnees, morceaux.donnees(), std::min(taille_tampon, taille_octet));
#endif
}

static int test_entree_aleatoire(const u_char *donnees, size_t taille)
{
	auto donnees_morceaux = reinterpret_cast<const id_morceau *>(donnees);
	auto nombre_morceaux = taille / sizeof(id_morceau);

	dls::tableau<DonneesMorceaux> morceaux;
	morceaux.reserve(static_cast<long>(nombre_morceaux));

	auto dm = DonneesMorceaux{};
	dm.chaine = "texte_test";
	dm.module = 0;

	for (size_t i = 0; i < nombre_morceaux; ++i) {
		dm.identifiant = donnees_morceaux[i];
		morceaux.pousse(dm);
	}

	try {
		auto contexte = ContexteGenerationCode{};
		auto module = contexte.cree_module("", "");
		module->tampon = lng::tampon_source("texte_test");
		module->morceaux = morceaux;
		auto assembleuse = assembleuse_arbre(contexte);
		contexte.assembleuse = &assembleuse;
		auto analyseuse = analyseuse_grammaire(contexte, module, "");

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
