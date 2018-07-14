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

#include "analyseuse_logique.h"

#include <iostream>
#include <stack>

#include "expression.h"
#include "graphe_contrainte.h"
#include "manipulable.h"

//#define DEBOGUE_ANALYSEUSE
#define DEBOGUE_EXPRESSION

#ifdef DEBOGUE_ANALYSEUSE
# define LOG std::cout
#else
struct Loggeuse {};

template <typename T>
Loggeuse &operator<<(Loggeuse &l, const T &) { return l; }

Loggeuse loggeuse_analyseuse;

# define LOG loggeuse_analyseuse
#endif

namespace danjo {

enum {
	EXPRESSION_ENTREE,
	EXPRESSION_INTERFACE,
	EXPRESSION_RELATION,
	EXPRESSION_SORTIE,
};

AnalyseuseLogique::AnalyseuseLogique(Manipulable *manipulable, bool initialise_manipulable)
	: m_manipulable(manipulable)
	, m_initialise_manipulable(initialise_manipulable)
{}

void AnalyseuseLogique::lance_analyse(const std::vector<DonneesMorceaux> &identifiants)
{
	m_identifiants = identifiants;
	m_position = 0;

	if (!requiers_identifiant(IDENTIFIANT_FEUILLE)) {
		lance_erreur("Le script doit commencer avec 'feuille' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
		lance_erreur("Attendu le nom de la feuille après 'feuille' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_corps();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin du script !");
	}
}

void AnalyseuseLogique::analyse_corps()
{
	LOG << __func__ << '\n';

	if (est_identifiant(IDENTIFIANT_ENTREE)) {
		analyse_entree();
	}
	else if (est_identifiant(IDENTIFIANT_INTERFACE)) {
		analyse_interface();
	}
	else if (est_identifiant(IDENTIFIANT_LOGIQUE)) {
		analyse_logique();
	}
	else if (est_identifiant(IDENTIFIANT_SORTIE)) {
		analyse_sortie();
	}
	else {
		return;
	}

	analyse_corps();

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_entree()
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(IDENTIFIANT_ENTREE)) {
		lance_erreur("Attendu la déclaration 'entrée' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_declaration(EXPRESSION_ENTREE);

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de l'entrée !");
	}

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_declaration(const int type)
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
		recule();
		return;
	}

	const auto nom = m_identifiants[position()].contenu;

	if (!requiers_identifiant(IDENTIFIANT_EGAL)) {
		lance_erreur("Attendu '=' !");
	}

	analyse_expression(nom, type);

	if (!requiers_identifiant(IDENTIFIANT_POINT_VIRGULE)) {
		lance_erreur("Attendu un point virgule !");
	}

	analyse_declaration(type);

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_expression(const std::string &nom, const int type)
{
	LOG << __func__ << '\n';

	const auto construit_graphe = !m_initialise_manipulable
								  && (type == EXPRESSION_RELATION
									  || type == EXPRESSION_SORTIE);
	contrainte *cont = nullptr;
	Variable *sortie = nullptr;

	if (!m_assembleuse.variable_connue(nom)) {
		sortie = m_assembleuse.ajoute_variable(nom);
	}
	else {
		sortie = m_assembleuse.variable(nom);
	}

	if (construit_graphe) {
		/* Crée une contrainte. */
		cont = new contrainte;
		cont->m_sortie = sortie;
	}

	/* Algorithme de Dijkstra pour générer une notation polonaise inversée. */

	std::vector<Symbole> expression;
	std::stack<Symbole> pile;

	Symbole symbole;
	std::string valeur;

	while (!est_identifiant(IDENTIFIANT_POINT_VIRGULE)) {
		symbole.identifiant = identifiant_courant();
		valeur = m_identifiants[position() + 1].contenu;

		if (est_identifiant(IDENTIFIANT_NOMBRE)) {
			symbole.valeur = std::experimental::any(std::stoi(valeur));
			expression.push_back(symbole);
		}
		else if (est_identifiant(IDENTIFIANT_NOMBRE_DECIMAL)) {
			symbole.valeur = std::experimental::any(std::stof(valeur));
			expression.push_back(symbole);
		}
		else if (est_identifiant(IDENTIFIANT_VRAI) || est_identifiant(IDENTIFIANT_FAUX)) {
			symbole.valeur = (valeur == "vrai");
			symbole.identifiant = IDENTIFIANT_BOOL;
			expression.push_back(symbole);
		}
		else if (est_identifiant(IDENTIFIANT_COULEUR)) {
			/* À FAIRE */
			expression.push_back(symbole);
		}
		else if (est_identifiant(IDENTIFIANT_VECTEUR)) {
			/* À FAIRE */
			expression.push_back(symbole);
		}
		else if (est_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
			symbole.valeur = valeur;
			expression.push_back(symbole);
		}
		else if (est_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
			if (!m_assembleuse.variable_connue(valeur)) {
				lance_erreur("Variable inconnue : " + valeur);
			}

			if (construit_graphe) {
				auto variable = m_assembleuse.variable(valeur);
				connecte(cont, variable);
			}

			symbole.valeur = std::experimental::any(valeur);
			expression.push_back(symbole);
		}
		else if (est_operateur(symbole.identifiant)) {
			while (!pile.empty()
				   && est_operateur(pile.top().identifiant)
				   && (precedence_faible(symbole.identifiant, pile.top().identifiant)))
			{
				expression.push_back(pile.top());
				pile.pop();
			}

			symbole.valeur = std::experimental::any(valeur);
			pile.push(symbole);
		}
		else if (est_identifiant(IDENTIFIANT_PARENTHESE_OUVRANTE)) {
			pile.push(symbole);
		}
		else if (est_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
			if (pile.empty()) {
				lance_erreur("Il manque une paranthèse dans l'expression !");
			}

			while (pile.top().identifiant != IDENTIFIANT_PARENTHESE_OUVRANTE) {
				expression.push_back(pile.top());
				pile.pop();
			}

			/* Enlève la parenthèse restante de la pile. */
			if (pile.top().identifiant == IDENTIFIANT_PARENTHESE_OUVRANTE) {
				pile.pop();
			}
		}

		avance();
	}

	while (!pile.empty()) {
		if (pile.top().identifiant == IDENTIFIANT_PARENTHESE_OUVRANTE) {
			lance_erreur("Il manque une paranthèse dans l'expression !");
		}

		expression.push_back(pile.top());
		pile.pop();
	}

#ifdef DEBOGUE_EXPRESSION
	std::ostream &os = std::cerr;

	os << "Expression (" << nom << ") : " ;

	for (const Symbole &symbole : expression) {
		imprime_valeur_symbole(symbole, os);
	}

	os << '\n';
#endif

	if (type == EXPRESSION_ENTREE || type == EXPRESSION_INTERFACE) {
		auto resultat = evalue_expression(expression, m_manipulable);

		switch (resultat.identifiant) {
			case IDENTIFIANT_NOMBRE:
				m_manipulable->ajoute_propriete(nom, TypePropriete::ENTIER, resultat.valeur);
				break;
			case IDENTIFIANT_NOMBRE_DECIMAL:
				m_manipulable->ajoute_propriete(nom, TypePropriete::DECIMAL, resultat.valeur);
				break;
			case IDENTIFIANT_BOOL:
				m_manipulable->ajoute_propriete(nom, TypePropriete::BOOL, resultat.valeur);
				break;
			case IDENTIFIANT_CHAINE_LITTERALE:
				m_manipulable->ajoute_propriete(nom, TypePropriete::CHAINE_CARACTERE, resultat.valeur);
				break;
			case IDENTIFIANT_COULEUR:
				m_manipulable->ajoute_propriete(nom, TypePropriete::COULEUR, resultat.valeur);
				break;
			case IDENTIFIANT_VECTEUR:
				m_manipulable->ajoute_propriete(nom, TypePropriete::VECTEUR, resultat.valeur);
				break;
		}

#ifdef DEBOGUE_EXPRESSION
		os << "Résultat : ";
		imprime_valeur_symbole(resultat, os);
		os << '\n';
#endif
	}

	if (construit_graphe) {
		m_assembleuse.ajoute_contrainte(cont);
		cont->m_expression = expression;
	}

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_interface()
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(IDENTIFIANT_INTERFACE)) {
		lance_erreur("Attendu la déclaration 'interface' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_declaration(EXPRESSION_INTERFACE);

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de l'interface !");
	}

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_logique()
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(IDENTIFIANT_LOGIQUE)) {
		lance_erreur("Attendu la déclaration 'logique' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_relation();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de la logique !");
	}

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_relation()
{
	LOG << __func__ << '\n';

	if (!est_identifiant(IDENTIFIANT_RELATION) && !est_identifiant(IDENTIFIANT_QUAND)) {
		return;
	}

	if (est_identifiant(IDENTIFIANT_QUAND)) {
		avance();

		if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_OUVRANTE)) {
			lance_erreur("Attendu une paranthèse ouvrante !");
		}

		if (!requiers_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
			lance_erreur("Attendu le nom d'une variable !");
		}

		if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
			lance_erreur("Attendu une paranthèse fermante !");
		}
	}

	if (!requiers_identifiant(IDENTIFIANT_RELATION)) {
		lance_erreur("Attendu la déclaration 'relation' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_declaration(EXPRESSION_RELATION);

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de la relation !");
	}

	analyse_relation();

	LOG << __func__ << " fin\n";
}

void AnalyseuseLogique::analyse_sortie()
{
	LOG << __func__ << '\n';

	if (!requiers_identifiant(IDENTIFIANT_SORTIE)) {
		lance_erreur("Attendu la déclaration 'sortie' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	analyse_declaration(EXPRESSION_SORTIE);

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de la sortie !");
	}

	LOG << __func__ << " fin\n";
}

}  /* namespace danjo */
