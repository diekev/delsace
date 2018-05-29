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

#include "expression.h"

#include <cmath>
#include <iostream>
#include <stack>
#include <vector>

#include "manipulable.h"
#include "morceaux.h"

namespace danjo {

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

#if 0
auto peut_evaleur_symboles(const Symbole &s1, const Symbole &s2)
{
	if (s1.identifiant == s2.identifiant) {
		return true;
	}

	if (s1.identifiant == IDENTIFIANT_NOMBRE) {
		return s2.identifiant == IDENTIFIANT_NOMBRE_DECIMAL;
	}

	if (s2.identifiant == IDENTIFIANT_NOMBRE) {
		return s1.identifiant == IDENTIFIANT_NOMBRE_DECIMAL;
	}

	return false;
}

auto evalue_operation(const Symbole &s1, const Symbole &s2, int operation)
{
	if (!peut_evaleur_symboles(s1, s2)) {
		return Symbole();
	}

	switch (operation) {
		case IDENTIFIANT_PLUS:
			break;
		default:
			break;
	}

	return Symbole();
}
#else
auto evalue_operation(const Symbole &s1, const Symbole &s2, int identifiant)
{
	Symbole resultat;
	resultat.identifiant = IDENTIFIANT_NOMBRE;
	resultat.valeur = 0;

	auto op1 = std::experimental::any_cast<int>(s1.valeur);
	auto op2 = std::experimental::any_cast<int>(s2.valeur);

	switch (identifiant) {
		case IDENTIFIANT_PLUS:
			resultat.valeur = op1 + op2;
			break;
		case IDENTIFIANT_MOINS:
			resultat.valeur = op1 - op2;
			break;
		case IDENTIFIANT_FOIS:
			resultat.valeur = op1 * op2;
			break;
		case IDENTIFIANT_DIVISE:
			resultat.valeur = op1 / op2;
			break;
		case IDENTIFIANT_EGALITE:
			resultat.valeur = static_cast<int>(op1 == op2);
			break;
		case IDENTIFIANT_DIFFERENCE:
			resultat.valeur = static_cast<int>(op1 != op2);
			break;
		case IDENTIFIANT_INFERIEUR:
			resultat.valeur = static_cast<int>(op1 < op2);
			break;
		case IDENTIFIANT_SUPERIEUR:
			resultat.valeur = static_cast<int>(op1 > op2);
			break;
		case IDENTIFIANT_INFERIEUR_EGAL:
			resultat.valeur = static_cast<int>(op1 <= op2);
			break;
		case IDENTIFIANT_SUPERIEUR_EGAL:
			resultat.valeur = static_cast<int>(op1 >= op2);
			break;
		case IDENTIFIANT_ESPERLUETTE:
			resultat.valeur = static_cast<int>(static_cast<int>(op1) & static_cast<int>(op2));
			break;
		case IDENTIFIANT_BARRE:
			resultat.valeur = static_cast<int>(static_cast<int>(op1) | static_cast<int>(op2));
			break;
		case IDENTIFIANT_CHAPEAU:
			resultat.valeur = static_cast<int>(static_cast<int>(op1) ^ static_cast<int>(op2));
			break;
	}

	return resultat;
}
#endif

auto evalue_operation_logique(const Symbole &s1, int identifiant)
{
	Symbole resultat;
	resultat.identifiant = IDENTIFIANT_NOMBRE;
	resultat.valeur = 0;

	auto op1 = std::experimental::any_cast<int>(s1.valeur);

	switch (identifiant) {
		case IDENTIFIANT_TILDE:
			resultat.valeur = static_cast<int>(~static_cast<int>(op1));
			break;
	}

	return resultat;
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
	std::stack<Symbole> pile;

	/* Pousse un zéro sur la pile si jamais l'expression est vide ou démarre
	 * avec un nombre négatif. */
	pile.push({std::experimental::any(0), IDENTIFIANT_NOMBRE});

	for (const Symbole &symbole : expression) {
		if (est_operateur(symbole.identifiant)) {
			auto s2 = pile.top();
			pile.pop();

			auto s1 = pile.top();
			pile.pop();

			auto resultat = evalue_operation(s1, s2, symbole.identifiant);
			pile.push(resultat);
		}
		else if (est_operateur_logique(symbole.identifiant)) {
			auto s1 = pile.top();
			pile.pop();

			auto resultat = evalue_operation_logique(s1, symbole.identifiant);
			pile.push(resultat);
		}
		else {
			switch (symbole.identifiant) {
				case IDENTIFIANT_BOOL:
				case IDENTIFIANT_NOMBRE:
				case IDENTIFIANT_NOMBRE_DECIMAL:
				{
					pile.push(symbole);
					break;
				}
				case IDENTIFIANT_CHAINE_CARACTERE:
				{
					auto nom = std::experimental::any_cast<std::string>(symbole.valeur);

					Symbole tmp;

					switch (manipulable->type_propriete(nom)) {
						case TypePropriete::ENTIER:
						{
							tmp.valeur = manipulable->evalue_entier(nom);
							tmp.identifiant = IDENTIFIANT_NOMBRE;
							break;
						}
						case TypePropriete::DECIMAL:
						{
							tmp.valeur = manipulable->evalue_decimal(nom);
							tmp.identifiant = IDENTIFIANT_NOMBRE_DECIMAL;
							break;
						}
						case TypePropriete::BOOL:
						{
							auto valeur = manipulable->evalue_bool(nom);
							tmp.valeur = valeur;
							tmp.identifiant = IDENTIFIANT_BOOL;
							break;
						}
						default:
						{
							std::cerr << "Le type de propriété n'est pas supporté !\n";
							break;
						}
					}

					pile.push(tmp);
					break;
				}
			}
		}
	}

	return pile.top();
}

}  /* namespace danjo */
