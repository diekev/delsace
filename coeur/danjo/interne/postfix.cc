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

#include "manipulable.h"
#include "morceaux.h"

namespace danjo {

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
		case IDENTIFIANT_PLUS:
		case IDENTIFIANT_DIVISE:
		case IDENTIFIANT_FOIS:
		case IDENTIFIANT_MOINS:
		case IDENTIFIANT_EGALITE:
		case IDENTIFIANT_DIFFERENCE:
		case IDENTIFIANT_INFERIEUR:
		case IDENTIFIANT_SUPERIEUR:
		case IDENTIFIANT_INFERIEUR_EGAL:
		case IDENTIFIANT_SUPERIEUR_EGAL:
		case IDENTIFIANT_ESPERLUETTE:
		case IDENTIFIANT_BARRE:
		case IDENTIFIANT_CHAPEAU:
			return true;
		default:
			return false;
	}
}

auto est_operateur_logique(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_TILDE:
			return true;
		default:
			return false;
	}
}

auto evalue_operation(const double op1, const double op2, int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_PLUS:
			return op1 + op2;
		case IDENTIFIANT_MOINS:
			return op1 - op2;
		case IDENTIFIANT_FOIS:
			return op1 * op2;
		case IDENTIFIANT_DIVISE:
			return op1 / op2;
		case IDENTIFIANT_EGALITE:
			return static_cast<double>(op1 == op2);
		case IDENTIFIANT_DIFFERENCE:
			return static_cast<double>(op1 != op2);
		case IDENTIFIANT_INFERIEUR:
			return static_cast<double>(op1 < op2);
		case IDENTIFIANT_SUPERIEUR:
			return static_cast<double>(op1 > op2);
		case IDENTIFIANT_INFERIEUR_EGAL:
			return static_cast<double>(op1 <= op2);
		case IDENTIFIANT_SUPERIEUR_EGAL:
			return static_cast<double>(op1 >= op2);
		case IDENTIFIANT_ESPERLUETTE:
			return static_cast<double>(static_cast<long int>(op1) & static_cast<long int>(op2));
		case IDENTIFIANT_BARRE:
			return static_cast<double>(static_cast<long int>(op1) | static_cast<long int>(op2));
		case IDENTIFIANT_CHAPEAU:
			return static_cast<double>(static_cast<long int>(op1) ^ static_cast<long int>(op2));
	}

	return 0.0;
}

auto evalue_operation_logique(const double op1, int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_TILDE:
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
	switch (identifiant) {
		case IDENTIFIANT_PLUS:
		case IDENTIFIANT_MOINS:
			return { GAUCHE, 0};
		case IDENTIFIANT_FOIS:
		case IDENTIFIANT_DIVISE:
		/* À FAIRE : modulo */
			return { GAUCHE, 1};
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

	return (p1.first == GAUCHE && p1.second <= p2.second)
			|| ((p2.first == DROITE) && (p1.second < p2.second));
}

Symbole evalue_expression(const std::vector<Symbole> &expression, Manipulable *manipulable)
{
	std::stack<Symbole> stack;

	/* Push a zero on the stack in case the expression starts with a negative
	 * number, or is empty. */
	stack.push({std::experimental::any(0), IDENTIFIANT_NOMBRE});

	for (const Symbole &symbole : expression) {
		if (est_operateur(symbole.identifiant)) {
			auto op1 = stack.top();
			stack.pop();

			auto op2 = stack.top();
			stack.pop();

			auto valeur = evalue_operation(
							  std::experimental::any_cast<int>(op2.valeur),
							  std::experimental::any_cast<int>(op1.valeur),
							  symbole.identifiant);

			Symbole resultat;
			resultat.valeur = std::experimental::any(int(valeur));
			resultat.identifiant = IDENTIFIANT_NOMBRE;

			stack.push(resultat);
		}
		else if (est_operateur_logique(symbole.identifiant)) {
			auto op1 = stack.top();
			stack.pop();

			auto valeur = evalue_operation_logique(
							  std::experimental::any_cast<int>(op1.valeur),
							  symbole.identifiant);

			Symbole resultat;
			resultat.valeur = std::experimental::any(int(valeur));
			resultat.identifiant = IDENTIFIANT_NOMBRE;

			stack.push(resultat);
		}
		else {
			if (symbole.identifiant == IDENTIFIANT_NOMBRE) {
				stack.push(symbole);
			}
			else if (symbole.identifiant == IDENTIFIANT_CHAINE_CARACTERE) {
				auto nom = std::experimental::any_cast<std::string>(symbole.valeur);

				Symbole tmp;
				tmp.valeur = manipulable->evalue_entier(nom);
				tmp.identifiant = IDENTIFIANT_NOMBRE;

				stack.push(tmp);
			}
		}
	}

	return stack.top();
}

}  /* namespace danjo */
