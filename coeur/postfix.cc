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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "postfix.h"

#include <cmath>
#include <iostream>
#include <stack>
#include <vector>

#include "morceaux.h"

namespace langage {

struct DonneesVariables {
	int identifiant;
	double valeur;
	std::string nom_propriete;
};

struct DonneesExpression {
	std::string propriete_sortie;
	std::vector<DonneesVariables> donnees;
};

bool est_operateur(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_ADDITION:
		case IDENTIFIANT_DIVISION:
		case IDENTIFIANT_MULTIPLICATION:
		case IDENTIFIANT_SOUSTRACTION:
		case IDENTIFIANT_EGALITE:
		case IDENTIFIANT_INEGALITE:
		case IDENTIFIANT_INFERIEUR:
		case IDENTIFIANT_SUPERIEUR:
		case IDENTIFIANT_INFERIEUR_EGAL:
		case IDENTIFIANT_SUPERIEUR_EGAL:
		case IDENTIFIANT_ET:
		case IDENTIFIANT_OU:
		case IDENTIFIANT_OUX:
		case IDENTIFIANT_EGAL:
			return true;
		default:
			return false;
	}
}

auto est_operateur_logique(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_NON:
			return true;
		default:
			return false;
	}
}

auto evalue_operation(const double op1, const double op2, int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_ADDITION:
			return op1 + op2;
		case IDENTIFIANT_SOUSTRACTION:
			return op1 - op2;
		case IDENTIFIANT_MULTIPLICATION:
			return op1 * op2;
		case IDENTIFIANT_DIVISION:
			return op1 / op2;
		case IDENTIFIANT_EGALITE:
			return static_cast<double>(op1 == op2);
		case IDENTIFIANT_INEGALITE:
			return static_cast<double>(op1 != op2);
		case IDENTIFIANT_INFERIEUR:
			return static_cast<double>(op1 < op2);
		case IDENTIFIANT_SUPERIEUR:
			return static_cast<double>(op1 > op2);
		case IDENTIFIANT_INFERIEUR_EGAL:
			return static_cast<double>(op1 <= op2);
		case IDENTIFIANT_SUPERIEUR_EGAL:
			return static_cast<double>(op1 >= op2);
		case IDENTIFIANT_ET:
			return static_cast<double>(static_cast<long int>(op1) & static_cast<long int>(op2));
		case IDENTIFIANT_OU:
			return static_cast<double>(static_cast<long int>(op1) | static_cast<long int>(op2));
		case IDENTIFIANT_OUX:
			return static_cast<double>(static_cast<long int>(op1) ^ static_cast<long int>(op2));
	}

	return 0.0;
}

auto evalue_operation_logique(const double op1, int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_NON:
			return static_cast<double>(~static_cast<long int>(op1));
	}

	return 0.0;
}

enum {
	GAUCHE,
	DROITE,
};

static std::pair<int, int> associativite(int identifiant)
{
	/* Retourne { associativite, précédence }, où plus la précédence est faible,
	 * plus l'opération est prioritaire. */
	switch (identifiant) {
		case IDENTIFIANT_EGAL:
			return { DROITE, 2 };
		case IDENTIFIANT_ADDITION:
		case IDENTIFIANT_SOUSTRACTION:
			return { GAUCHE, 1};
		case IDENTIFIANT_MULTIPLICATION:
		case IDENTIFIANT_DIVISION:
		/* À FAIRE : modulo */
			return { GAUCHE, 0};
#if 0
		case IDENTIFIANT_PUISSANCE:
			return { DROITE, 2};
#endif
	}

	return { GAUCHE, 0 };
}

bool precedence_faible(int identifiant1, int identifiant2)
{
	auto p1 = associativite(identifiant1);
	auto p2 = associativite(identifiant2);

	return (p1.first == GAUCHE && p1.second >= p2.second)
			|| ((p2.first == DROITE) && (p1.second > p2.second));
}

double evalue_expression(const std::vector<Variable> &expression)
{
	std::stack<double> stack;

	/* Push a zero on the stack in case the expression starts with a negative
	 * number, or is empty. */
	stack.push(0);

	for (const Variable &variable : expression) {
		if (est_operateur(variable.identifiant)) {
			auto op1 = stack.top();
			stack.pop();

			auto op2 = stack.top();
			stack.pop();

			auto result = evalue_operation(op2, op1, variable.identifiant);
			stack.push(result);
		}
		else if (est_operateur_logique(variable.identifiant)) {
			auto op1 = stack.top();
			stack.pop();

			auto result = evalue_operation_logique(op1, variable.identifiant);
			stack.push(result);
		}
		else {
			if (variable.identifiant == IDENTIFIANT_NOMBRE) {
				stack.push(std::stod(variable.valeur));
			}
			else if (variable.identifiant == IDENTIFIANT_CHAINE_CARACTERE) {
				/* À FAIRE : évalue la propriété. */
				stack.push(1.0);
			}
		}
	}

	return stack.top();
}

}  /* namespace langage */
