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

#include "analyseuse_langage.h"

#include <iostream>
#include <stack>
#include <sys/time.h>

#include "postfix.h"

namespace langage {

inline double maintenant()
{
	struct timeval now;
	gettimeofday(&now, nullptr);

	return now.tv_sec + now.tv_usec * 1e-6;
}

void AnalyseuseLangage::lance_analyse(const std::vector<DonneesMorceaux> &morceaux)
{
	m_position = 0;
	m_identifiants = morceaux;

	analyse_declaration();
}

void AnalyseuseLangage::analyse_declaration()
{
	const auto identifiant = identifiant_courant();

	auto requiers_nouvelle_ligne = true;

	switch (identifiant) {
		case IDENTIFIANT_NUL:
			return;
		case IDENTIFIANT_IMPRIME:
			analyse_imprime();
			break;
		case IDENTIFIANT_CHAINE_CARACTERE:
			analyse_variable();
			break;
		case IDENTIFIANT_CHRONOMETRE:
			analyse_chronometre();
			break;
		case IDENTIFIANT_FONCTION:
			requiers_nouvelle_ligne = false;
			analyse_fonction();
			break;
	}

	if (requiers_nouvelle_ligne && !requiers_identifiant(IDENTIFIANT_NOUVELLE_LIGNE)) {
		lance_erreur("Attendu un point virgule en fin de déclaration !");
	}

	analyse_declaration();
}

void AnalyseuseLangage::analyse_imprime()
{
	if (!requiers_identifiant(IDENTIFIANT_IMPRIME)) {
		lance_erreur("Attendu la déclaration 'imprime'");
	}

	auto imprime_temps = [this]()
	{
		if (!requiers_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
			lance_erreur("Attendu une chaîne de caractère après 'temps'");
		}

		auto valeur = m_identifiants[position()].contenu;
		auto iter = m_chronometres.find(valeur);

		if (iter != m_chronometres.end()) {
			std::cout << maintenant() - iter->second  << "s";
		}
		else {
			lance_erreur("Chronomètre inconnu");
		}
	};

	if (identifiant_courant() == IDENTIFIANT_CHAINE_CARACTERE) {
		avance();

		auto valeur = m_identifiants[position()].contenu;
		auto iter = m_donnees_script.variables.find(valeur);

		if (iter != m_donnees_script.variables.end()) {
			const DonneesVariables &donnees_variable = iter->second;
			std::cout << donnees_variable.valeur;
		}
		else {
			std::cout << valeur;
		}

		if (identifiant_courant() == IDENTIFIANT_VIRGULE) {
			avance();

			if (identifiant_courant() == IDENTIFIANT_TEMPS) {
				avance();
				std::cout << ' ';
				imprime_temps();
			}
			else {
				lance_erreur("Déclaration d'impression invalide");
			}
		}
	}
	else if (identifiant_courant() == IDENTIFIANT_TEMPS) {
		avance();
		imprime_temps();
	}
	else {
		lance_erreur("Déclaration d'impression invalide");
	}

	std::cout << '\n';
}

void AnalyseuseLangage::analyse_variable()
{
	if (!requiers_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère");
	}

	auto nom_variable = m_identifiants[position()].contenu;

	if (!requiers_identifiant(IDENTIFIANT_EGAL)) {
		lance_erreur("Attendu la déclaration '='");
	}

	DonneesVariables donnees_variable;

	if (identifiant_courant() == IDENTIFIANT_NOMBRE) {
		avance();

		donnees_variable.valeur = m_identifiants[position()].contenu;
	}
	else if (identifiant_courant() == IDENTIFIANT_EXPRIME) {
		avance();

		if (!requiers_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
			lance_erreur("Attendu une chaîne de caractère après 'exprime'");
		}

		auto valeur = m_identifiants[position()].contenu;

		std::cout << valeur << '\n';
		std::string reponse;
		std::getline(std::cin, reponse);

		donnees_variable.valeur = reponse;
	}

	m_donnees_script.variables.insert({nom_variable, donnees_variable});
}

void AnalyseuseLangage::analyse_chronometre()
{
	if (!requiers_identifiant(IDENTIFIANT_CHRONOMETRE)) {
		lance_erreur("Attendu la déclaration 'chronomètre'");
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère");
	}

	m_chronometres.insert({m_identifiants[position()].contenu, maintenant()});
}

void AnalyseuseLangage::analyse_fonction()
{
	if (!requiers_identifiant(IDENTIFIANT_FONCTION)) {
		lance_erreur("Attendu la déclaration 'fonction'");
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaîne de caractère");
	}

	const auto nom_fonction = m_identifiants[position()].contenu;
	DonneesFonction donnees_fonction;

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_OUVERTE)) {
		lance_erreur("Attendu une parenthèse ouverte");
	}

	//analyse_liste_params();

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_FERMEE)) {
		lance_erreur("Attendu une parenthèse fermée");
	}

	if (!requiers_identifiant(IDENTIFIANT_NOUVELLE_LIGNE)) {
		lance_erreur("Attendu une nouvelle ligne");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVERTE)) {
		lance_erreur("Attendu une accolade ouverte");
	}

	if (!requiers_identifiant(IDENTIFIANT_NOUVELLE_LIGNE)) {
		lance_erreur("Attendu une nouvelle ligne");
	}

	analyse_expression(donnees_fonction);

	if (est_identifiant(IDENTIFIANT_RETOURNE)) {
		avance();

		while (!est_identifiant(IDENTIFIANT_NOUVELLE_LIGNE)) {
			avance();
		}
		avance();
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMEE)) {
		lance_erreur("Attendu une accolade fermée");
	}

	m_donnees_script.fonctions.insert({nom_fonction, donnees_fonction});
}

void AnalyseuseLangage::analyse_expression(DonneesFonction &donnees_fonction)
{
#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << '\n';
#endif

	if (identifiant_courant() != IDENTIFIANT_CHAINE_CARACTERE) {
		return;
	}

	donnees_fonction.variables_locales.insert({m_identifiants[position() + 1].contenu, DonneesVariables{}});

	/* Algorithme de Dijkstra pour générer une notation polonaire inversée. */

	std::vector<Variable> output;
	std::stack<Variable> stack;

	Variable variable;

	while (!est_identifiant(IDENTIFIANT_NOUVELLE_LIGNE)) {
		variable.identifiant = identifiant_courant();
		variable.valeur = m_identifiants[position() + 1].contenu;

		if (est_identifiant(IDENTIFIANT_NOMBRE)) {
			output.push_back(variable);
		}
		else if (est_identifiant(IDENTIFIANT_CHAINE_CARACTERE)) {
//			if (!m_assembleuse.variable_connue(variable.valeur)) {
//				lance_erreur("Variable inconnue : " + variable.valeur);
//			}

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
		else if (est_identifiant(IDENTIFIANT_PARENTHESE_OUVERTE)) {
			stack.push(variable);
		}
		else if (est_identifiant(IDENTIFIANT_PARENTHESE_FERMEE)) {
			if (stack.empty()) {
				lance_erreur("Il manque une paranthèse dans l'expression !");
			}

			while (stack.top().identifiant != IDENTIFIANT_PARENTHESE_OUVERTE) {
				output.push_back(stack.top());
				stack.pop();
			}

			/* Enlève la parenthèse restante de la pile. */
			if (stack.top().identifiant == IDENTIFIANT_PARENTHESE_OUVERTE) {
				stack.pop();
			}
		}

		avance();
	}

	while (!stack.empty()) {
		if (stack.top().identifiant == IDENTIFIANT_PARENTHESE_OUVERTE) {
			lance_erreur("Il manque une paranthèse dans l'expression !");
		}

		output.push_back(stack.top());
		stack.pop();
	}

	donnees_fonction.expressions.push_back(output);

#define DEBOGUE_EXPRESSION

#ifdef DEBOGUE_EXPRESSION
	std::cerr << "Expression  : " ;
	for (const Variable &variable : output) {
		std::cerr << variable.valeur << ' ';
	}
	std::cerr << '\n';

	auto resultat = evalue_expression(output);

	std::cerr << "Résultat : " << resultat << '\n';
#endif

//	m_assembleuse.ajoute_expression(nom, type, output);

#ifdef DEBOGUE_ANALYSEUSE
	std::cout << __func__ << " fin\n";
#endif

	if (!requiers_identifiant(IDENTIFIANT_NOUVELLE_LIGNE)) {
		lance_erreur("Attendu un point virgule à la fin de l'expression");
	}

	analyse_expression(donnees_fonction);
}

}  /* namespace langage */
