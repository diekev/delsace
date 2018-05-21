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
#include <unordered_map>
#include <stack>

#include "postfix.h"

#define DEBOGUE_ANALYSEUSE
#define DEBOGUE_EXPRESSION

namespace kangao {

#if 0
struct NoeudPropriete {
	std::vector<NoeudExpression *> noeuds_expressions;
};

struct NoeudExpression {
	std::vector<NoeudPropriete *> noeuds_proprietes;
	Expression *expression;
};

struct LiensNoeud;

class GrapheDependence {
	std::vector<NoeudPropriete *> m_noeuds_proprietes;
	std::vector<NoeudExpression *> m_noeuds_expressions;

public:
	void ajoute(NoeudExpression *noeud)
	{
		m_noeuds_expressions.push_back(noeud);
	}

	void ajoute(NoeudPropriete *noeud)
	{
		m_noeuds_proprietes.push_back(noeud);
	}
};
#endif

enum {
	EXPRESSION_ENTREE,
	EXPRESSION_INTERFACE,
	EXPRESSION_RELATION,
};

class AssembleuseLogique {
	std::unordered_map<std::string, std::vector<Variable>> m_expressions_entree;
	std::unordered_map<std::string, std::vector<Variable>> m_expressions_interface;
	std::unordered_map<std::string, std::vector<Variable>> m_expressions_relation;
	std::unordered_map<std::string, Variable> m_variables;

public:
	void ajoute_expression(const std::string &nom, const int type, const std::vector<Variable> &expression)
	{
		switch (type) {
			case EXPRESSION_ENTREE:
				m_expressions_entree.insert({nom, expression});
				break;
			case EXPRESSION_INTERFACE:
				m_expressions_interface.insert({nom, expression});
				break;
			case EXPRESSION_RELATION:
				m_expressions_relation.insert({nom, expression});
				break;
		}
	}

	void ajoute_variable(const std::string &nom)
	{
		m_variables.insert({nom, Variable()});
	}

	bool variable_connue(const std::string &nom)
	{
		return m_variables.find(nom) != m_variables.end();
	}
};

AssembleuseLogique m_assembleuse;

/* ************************************************************************** */

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
#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << '\n';
#endif

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

#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << " fin\n";
#endif
}

void AnalyseuseLogique::analyse_entree()
{
#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << '\n';
#endif

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

#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << " fin\n";
#endif
}

void AnalyseuseLogique::analyse_declaration(const int type)
{
#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << '\n';
#endif

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
		recule();
		return;
	}

	const auto nom = m_identifiants[position()].contenu;

	m_assembleuse.ajoute_variable(nom);

	if (!requiers_identifiant(IDENTIFIANT_EGAL)) {
		lance_erreur("Attendu '=' !");
	}

	analyse_expression(nom, type);

	if (!requiers_identifiant(IDENTIFIANT_POINT_VIRGULE)) {
		lance_erreur("Attendu un point virgule !");
	}

	analyse_declaration(type);

#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << " fin\n";
#endif
}

void AnalyseuseLogique::analyse_expression(const std::string &nom, const int type)
{
#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << '\n';
#endif

	/* Algorithme de Dijkstra pour générer une notation polonaire inversée. */

	std::vector<Variable> output;
	std::stack<Variable> stack;

	Variable variable;

	while (!est_identifiant(IDENTIFIANT_POINT_VIRGULE)) {
		variable.identifiant = identifiant_courant();
		variable.valeur = m_identifiants[position() + 1].contenu;

		if (est_identifiant(IDENTIFIANT_NOMBRE)) {
			output.push_back(variable);
		}
		else if (est_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
			if (!m_assembleuse.variable_connue(variable.valeur)) {
				lance_erreur("Variable inconnue : " + variable.valeur);
			}

			output.push_back(variable);
		}
		else if (est_operateur(variable.identifiant)) {
			while (!stack.empty()
				   && est_operateur(stack.top().identifiant)
				   && (precedence_faible(variable.identifiant, stack.top().identifiant)))
			{
				output.push_back(stack.top());
				stack.pop();
			}

			stack.push(variable);
		}
		else if (est_identifiant(IDENTIFIANT_PARENTHESE_OUVRANTE)) {
			stack.push(variable);
		}
		else if (est_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
			if (stack.empty()) {
				lance_erreur("Il manque une paranthèse dans l'expression !");
			}

			while (stack.top().identifiant != IDENTIFIANT_PARENTHESE_OUVRANTE) {
				output.push_back(stack.top());
				stack.pop();
			}

			/* Enlève la parenthèse restante de la pile. */
			if (stack.top().identifiant == IDENTIFIANT_PARENTHESE_OUVRANTE) {
				stack.pop();
			}
		}

		avance();
	}

	while (!stack.empty()) {
		if (stack.top().identifiant == IDENTIFIANT_PARENTHESE_OUVRANTE) {
			lance_erreur("Il manque une paranthèse dans l'expression !");
		}

		output.push_back(stack.top());
		stack.pop();
	}

#ifdef DEBOGUE_EXPRESSION
	std::cerr << "Expression (" << nom << ") : " ;
	for (const Variable &variable : output) {
		std::cerr << variable.valeur << ' ';
	}
	std::cerr << '\n';

	auto resultat = evalue_expression(output);

	std::cerr << "Résultat : " << resultat << '\n';
#endif

	m_assembleuse.ajoute_expression(nom, type, output);

#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << " fin\n";
#endif
}

void AnalyseuseLogique::analyse_interface()
{
#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << '\n';
#endif

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

#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << " fin\n";
#endif
}

void AnalyseuseLogique::analyse_logique()
{
#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << '\n';
#endif

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

#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << " fin\n";
#endif
}

void AnalyseuseLogique::analyse_relation()
{
#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << '\n';
#endif

	if (!est_identifiant(IDENTIFIANT_RELATION) && !est_identifiant(IDENTIFIANT_QUAND)) {
		return;
	}

	if (est_identifiant(IDENTIFIANT_QUAND)) {
		avance();

		if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_OUVRANTE)) {
			lance_erreur("Attendu une paranthèse ouvrante !");
		}

		if (!requiers_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
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

#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << " fin\n";
#endif
}

void AnalyseuseLogique::analyse_sortie()
{
#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << '\n';
#endif
	if (!requiers_identifiant(IDENTIFIANT_SORTIE)) {
		lance_erreur("Attendu la déclaration 'sortie' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante !");
	}

	/******/

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin de la sortie !");
	}

#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << " fin\n";
#endif
}

}  /* namespace kangao */
