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

static int promeut(int id1, int id2)
{
	if (id1 == id2) {
		return id1;
	}

	return IDENTIFIANT_NOMBRE_DECIMAL;
}

#define DEFINI_FONCTION(__nom, __op) \
	template <typename T1, typename T2 = T1> \
	Symbole __nom(const Symbole &s1, const Symbole &s2) \
	{ \
		Symbole ret; \
		ret.valeur = std::experimental::any_cast<T1>(s1.valeur) \
					  __op std::experimental::any_cast<T2>(s2.valeur); \
		ret.identifiant = promeut(s1.identifiant, s2.identifiant); \
		return ret; \
	}

DEFINI_FONCTION(additionne, +)
DEFINI_FONCTION(soustrait, -)
DEFINI_FONCTION(divise, /)
DEFINI_FONCTION(multiplie, *)
DEFINI_FONCTION(compare_egalite, ==)
DEFINI_FONCTION(compare_difference, !=)
DEFINI_FONCTION(compare_inferiorite, <)
DEFINI_FONCTION(compare_superiorite, >)
DEFINI_FONCTION(compare_inf_egal, <=)
DEFINI_FONCTION(compare_sup_egal, >=)
DEFINI_FONCTION(octet_et, &)
DEFINI_FONCTION(octet_ou, |)
DEFINI_FONCTION(octet_oux, ^)

#define DEFINI_CAS_SIMPLE(__id, __fonction) \
	case __id: \
		if (s1.identifiant == IDENTIFIANT_NOMBRE) { \
			return __fonction<int>(s1, s2); \
		} \
		if (s1.identifiant == IDENTIFIANT_NOMBRE_DECIMAL) { \
			return __fonction<float>(s1, s2); \
		} \
		if (s1.identifiant == IDENTIFIANT_BOOL) { \
			return __fonction<bool>(s1, s2); \
		} \
		break;

#define DEFINI_CAS_DOUBLE(__id, __fonction) \
	case __id: \
		if (s1.identifiant == IDENTIFIANT_NOMBRE && s2.identifiant == IDENTIFIANT_NOMBRE_DECIMAL) { \
			return __fonction<int, float>(s1, s2); \
		} \
		if (s1.identifiant == IDENTIFIANT_NOMBRE_DECIMAL && s2.identifiant == IDENTIFIANT_NOMBRE) { \
			return __fonction<float, int>(s1, s2); \
		} \
		break;

Symbole evalue_operation(const Symbole &s1, const Symbole &s2, int operation)
{
	if (s1.identifiant == s2.identifiant) {
		switch (operation) {
			DEFINI_CAS_SIMPLE(IDENTIFIANT_PLUS, additionne);
			DEFINI_CAS_SIMPLE(IDENTIFIANT_MOINS, soustrait);
			DEFINI_CAS_SIMPLE(IDENTIFIANT_DIVISE, divise);
			DEFINI_CAS_SIMPLE(IDENTIFIANT_FOIS, multiplie);
			DEFINI_CAS_SIMPLE(IDENTIFIANT_EGALITE, compare_egalite);
			DEFINI_CAS_SIMPLE(IDENTIFIANT_DIFFERENCE, compare_difference);
			DEFINI_CAS_SIMPLE(IDENTIFIANT_INFERIEUR, compare_inferiorite);
			DEFINI_CAS_SIMPLE(IDENTIFIANT_SUPERIEUR, compare_superiorite);
			DEFINI_CAS_SIMPLE(IDENTIFIANT_INFERIEUR_EGAL, compare_inf_egal);
			DEFINI_CAS_SIMPLE(IDENTIFIANT_SUPERIEUR_EGAL, compare_sup_egal);
		}
	}
	else {
		switch (operation) {
			DEFINI_CAS_DOUBLE(IDENTIFIANT_PLUS, additionne);
			DEFINI_CAS_DOUBLE(IDENTIFIANT_MOINS, soustrait);
			DEFINI_CAS_DOUBLE(IDENTIFIANT_DIVISE, divise);
			DEFINI_CAS_DOUBLE(IDENTIFIANT_FOIS, multiplie);
			DEFINI_CAS_DOUBLE(IDENTIFIANT_EGALITE, compare_egalite);
			DEFINI_CAS_DOUBLE(IDENTIFIANT_DIFFERENCE, compare_difference);
			DEFINI_CAS_DOUBLE(IDENTIFIANT_INFERIEUR, compare_inferiorite);
			DEFINI_CAS_DOUBLE(IDENTIFIANT_SUPERIEUR, compare_superiorite);
			DEFINI_CAS_DOUBLE(IDENTIFIANT_INFERIEUR_EGAL, compare_inf_egal);
			DEFINI_CAS_DOUBLE(IDENTIFIANT_SUPERIEUR_EGAL, compare_sup_egal);
		}
	}

	return {};
}

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
				case IDENTIFIANT_CHAINE_LITTERALE:
				case IDENTIFIANT_COULEUR:
				case IDENTIFIANT_VECTEUR:
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
							tmp.valeur = manipulable->evalue_bool(nom);
							tmp.identifiant = IDENTIFIANT_BOOL;
							break;
						}
						case TypePropriete::COULEUR:
						{
							tmp.valeur = manipulable->evalue_couleur(nom);
							tmp.identifiant = IDENTIFIANT_COULEUR;
							break;
						}
						case TypePropriete::VECTEUR:
						{
							tmp.valeur = manipulable->evalue_vecteur(nom);
							tmp.identifiant = IDENTIFIANT_VECTEUR;
							break;
						}
						case TypePropriete::ENUM:
						case TypePropriete::FICHIER_ENTREE:
						case TypePropriete::FICHIER_SORTIE:
						case TypePropriete::CHAINE_CARACTERE:
						{
							tmp.valeur = manipulable->evalue_chaine(nom);
							tmp.identifiant = IDENTIFIANT_CHAINE_LITTERALE;
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
