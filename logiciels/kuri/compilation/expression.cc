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

#include "expression.h"

#include "biblinternes/langage/nombres.hh"
#include "biblinternes/outils/conditions.h"

#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "outils_lexemes.hh"

using denombreuse = lng::decoupeuse_nombre<TypeLexeme>;

enum class dir_associativite : int {
	GAUCHE,
	DROITE,
};

struct DonneesPrecedence {
	dir_associativite direction;
	int priorite;
};

static DonneesPrecedence associativite(TypeLexeme identifiant)
{
	switch (identifiant) {
		case TypeLexeme::TROIS_POINTS:
			return { dir_associativite::GAUCHE, 0 };
		case TypeLexeme::EGAL:
		case TypeLexeme::DECLARATION_VARIABLE:
		case TypeLexeme::PLUS_EGAL:
		case TypeLexeme::MOINS_EGAL:
		case TypeLexeme::DIVISE_EGAL:
		case TypeLexeme::MULTIPLIE_EGAL:
		case TypeLexeme::MODULO_EGAL:
		case TypeLexeme::ET_EGAL:
		case TypeLexeme::OU_EGAL:
		case TypeLexeme::OUX_EGAL:
		case TypeLexeme::DEC_DROITE_EGAL:
		case TypeLexeme::DEC_GAUCHE_EGAL:
			return { dir_associativite::GAUCHE, 1 };
		case TypeLexeme::VIRGULE:
			return { dir_associativite::GAUCHE, 2 };
		case TypeLexeme::BARRE_BARRE:
			return { dir_associativite::GAUCHE, 3 };
		case TypeLexeme::ESP_ESP:
			return { dir_associativite::GAUCHE, 4 };
		case TypeLexeme::BARRE:
			return { dir_associativite::GAUCHE, 5 };
		case TypeLexeme::CHAPEAU:
			return { dir_associativite::GAUCHE, 6 };
		case TypeLexeme::ESPERLUETTE:
			return { dir_associativite::GAUCHE, 7 };
		case TypeLexeme::DIFFERENCE:
		case TypeLexeme::EGALITE:
			return { dir_associativite::GAUCHE, 8 };
		case TypeLexeme::INFERIEUR:
		case TypeLexeme::INFERIEUR_EGAL:
		case TypeLexeme::SUPERIEUR:
		case TypeLexeme::SUPERIEUR_EGAL:
			return { dir_associativite::GAUCHE, 9 };
		case TypeLexeme::DECALAGE_GAUCHE:
		case TypeLexeme::DECALAGE_DROITE:
			return { dir_associativite::GAUCHE, 10 };
		case TypeLexeme::PLUS:
		case TypeLexeme::MOINS:
			return { dir_associativite::GAUCHE, 11 };
		case TypeLexeme::FOIS:
		case TypeLexeme::DIVISE:
		case TypeLexeme::POURCENT:
			return { dir_associativite::GAUCHE, 12 };
		case TypeLexeme::EXCLAMATION:
		case TypeLexeme::TILDE:
		case TypeLexeme::AROBASE:
		case TypeLexeme::PLUS_UNAIRE:
		case TypeLexeme::MOINS_UNAIRE:
			return { dir_associativite::DROITE, 13 };
		case TypeLexeme::POINT:
		case TypeLexeme::CROCHET_OUVRANT:
			return { dir_associativite::GAUCHE, 14 };
		default:
			assert(false);
			return { static_cast<dir_associativite>(-1), -1 };
	}
}

bool precedence_faible(TypeLexeme identifiant1, TypeLexeme identifiant2)
{
	auto p1 = associativite(identifiant1);
	auto p2 = associativite(identifiant2);

	return (p1.direction == dir_associativite::GAUCHE && p1.priorite <= p2.priorite)
			|| ((p2.direction == dir_associativite::DROITE) && (p1.priorite < p2.priorite));
}

/* ************************************************************************** */

template <typename T>
static auto applique_operateur_unaire(TypeLexeme id, T &a)
{
	switch (id) {
		case TypeLexeme::EXCLAMATION:
		{
			a = !a;
			break;
		}
		case TypeLexeme::TILDE:
		{
			a = ~a;
			break;
		}
		case TypeLexeme::PLUS_UNAIRE:
		{
			break;
		}
		case TypeLexeme::MOINS_UNAIRE:
		{
			a = -a;
			break;
		}
		default:
		{
			a = 0;
			break;
		}
	}
}

static auto applique_operateur_unaire(TypeLexeme id, double &a)
{
	switch (id) {
		case TypeLexeme::PLUS_UNAIRE:
		{
			break;
		}
		case TypeLexeme::MOINS_UNAIRE:
		{
			a = -a;
			break;
		}
		default:
		{
			a = 0;
			break;
		}
	}
}

template <typename T>
static auto applique_operateur_binaire(TypeLexeme id, T a, T b)
{
	switch (id) {
		case TypeLexeme::PLUS:
		case TypeLexeme::PLUS_EGAL:
		{
			return a + b;
		}
		case TypeLexeme::MOINS:
		case TypeLexeme::MOINS_EGAL:
		{
			return a - b;
		}
		case TypeLexeme::FOIS:
		case TypeLexeme::MULTIPLIE_EGAL:
		{
			return a * b;
		}
		case TypeLexeme::DIVISE:
		case TypeLexeme::DIVISE_EGAL:
		{
			return a / b;
		}
		case TypeLexeme::POURCENT:
		case TypeLexeme::MODULO_EGAL:
		{
			return a % b;
		}
		case TypeLexeme::ESPERLUETTE:
		case TypeLexeme::ET_EGAL:
		{
			return a & b;
		}
		case TypeLexeme::OU_EGAL:
		case TypeLexeme::BARRE:
		{
			return a | b;
		}
		case TypeLexeme::CHAPEAU:
		case TypeLexeme::OUX_EGAL:
		{
			return a ^ b;
		}
		case TypeLexeme::DECALAGE_DROITE:
		case TypeLexeme::DEC_DROITE_EGAL:
		{
			return a >> b;
		}
		case TypeLexeme::DECALAGE_GAUCHE:
		case TypeLexeme::DEC_GAUCHE_EGAL:
		{
			return a << b;
		}
		default:
		{
			return T(0);
		}
	}
}

static auto applique_operateur_binaire(TypeLexeme id, double a, double b)
{
	switch (id) {
		case TypeLexeme::PLUS:
		case TypeLexeme::PLUS_EGAL:
		{
			return a + b;
		}
		case TypeLexeme::MOINS:
		case TypeLexeme::MOINS_EGAL:
		{
			return a - b;
		}
		case TypeLexeme::FOIS:
		case TypeLexeme::MULTIPLIE_EGAL:
		{
			return a * b;
		}
		case TypeLexeme::DIVISE:
		case TypeLexeme::DIVISE_EGAL:
		{
			return a / b;
		}
		default:
		{
			return 0.0;
		}
	}
}

template <typename T>
static auto applique_operateur_binaire_comp(TypeLexeme id, T a, T b)
{
	switch (id) {
		case TypeLexeme::INFERIEUR:
		case TypeLexeme::INFERIEUR_EGAL:
		{
			return a < b;
		}
		case TypeLexeme::SUPERIEUR:
		case TypeLexeme::SUPERIEUR_EGAL:
		{
			return a > b;
		}
		case TypeLexeme::DIFFERENCE:
		{
			return a != b;
		}
		case TypeLexeme::ESP_ESP:
		{
			return a && b;
		}
		case TypeLexeme::EGALITE:
		{
			return a == b;
		}
		case TypeLexeme::BARRE_BARRE:
		{
			return a || b;
		}
		default:
		{
			return false;
		}
	}
}

/**
 * Évalue l'expression dont « b » est la racine. L'expression doit être
 * constante, c'est à dire ne contenir que des noeuds dont la valeur est connue
 * lors de la compilation.
 *
 * Dans le future, ce sera sans doute la base d'un interpreteur pour exécuter de
 * manière arbitraire du code lors de la compilation. Pour cela, la prochaine
 * étape sera de pouvoir évaluer des fonctions entières.
 */
ResultatExpression evalue_expression(ContexteGenerationCode &contexte, noeud::base *b)
{
	switch (b->type) {
		default:
		{
			auto res = ResultatExpression();
			res.est_errone = true;
			res.noeud_erreur = b;
			res.message_erreur = "L'expression n'est pas constante et ne peut être calculée !";

			return res;
		}
		case type_noeud::VARIABLE:
		{
			auto res = ResultatExpression();

			if (!contexte.locale_existe(b->chaine())) {
				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "La variable n'existe pas !";

				return res;
			}

			auto &donnees = contexte.donnees_variable(b->chaine());

			res.type = donnees.resultat_expression.type;
			res.entier = donnees.resultat_expression.entier;

			return res;
		}
		case type_noeud::TAILLE_DE:
		{
			auto index_dt = std::any_cast<long>(b->valeur_calculee);
			auto const &donnees = contexte.typeuse[index_dt];

			auto res = ResultatExpression();
			res.type = type_expression::ENTIER;
			res.entier = taille_octet_type(contexte, donnees);

			return res;
		}
		case type_noeud::BOOLEEN:
		{
			auto res = ResultatExpression();
			res.type = type_expression::ENTIER;
			res.entier = b->chaine() == "vrai";

			return res;
		}
		case type_noeud::NOMBRE_ENTIER:
		{
			auto res = ResultatExpression();
			res.type = type_expression::ENTIER;

			auto chaine_chiffre = dls::vue_chaine(b->chaine().pointeur(), b->chaine().taille());

			switch (b->morceau.identifiant) {
				case TypeLexeme::NOMBRE_ENTIER:
				{
					res.entier = lng::converti_nombre_entier(chaine_chiffre);
					break;
				}
				case TypeLexeme::NOMBRE_HEXADECIMAL:
				{
					res.entier = lng::converti_chaine_nombre_hexadecimal(chaine_chiffre);
					break;
				}
				case TypeLexeme::NOMBRE_OCTAL:
				{
					res.entier = lng::converti_chaine_nombre_octal(chaine_chiffre);
					break;
				}
				case TypeLexeme::NOMBRE_BINAIRE:
				{
					res.entier = lng::converti_chaine_nombre_binaire(chaine_chiffre);
					break;
				}
				default:
				{
					break;
				}
			}

			return res;
		}
		case type_noeud::NOMBRE_REEL:
		{
			auto res = ResultatExpression();
			res.type = type_expression::REEL;
			res.reel = lng::converti_nombre_reel(dls::vue_chaine(b->chaine().pointeur(), b->chaine().taille()));

			return res;
		}
		case type_noeud::SAUFSI:
		case type_noeud::SI:
		{
			auto const nombre_enfants = b->enfants.taille();
			auto iter_enfant = b->enfants.debut();

			auto enfant1 = *iter_enfant++;

			auto res = evalue_expression(contexte, enfant1);

			if (res.est_errone) {
				return res;
			}

			if (res.type != type_expression::ENTIER) {
				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "L'expression n'est pas de type booléen !";
				return res;
			}

			auto enfant2 = *iter_enfant++;

			if (res.condition == (b->type == type_noeud::SI)) {
				res = evalue_expression(contexte, enfant2);
			}
			else {
				if (nombre_enfants == 3) {
					auto enfant3 = *iter_enfant++;
					res = evalue_expression(contexte, enfant3);
				}
			}

			return res;
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			auto res = evalue_expression(contexte, b->enfants.front());

			if (res.est_errone) {
				return res;
			}

			if (res.type == type_expression::REEL) {
				applique_operateur_unaire(b->identifiant(), res.reel);
			}
			else {
				applique_operateur_unaire(b->identifiant(), res.entier);
			}

			return res;
		}
		case type_noeud::OPERATION_BINAIRE:
		{
			auto res1 = evalue_expression(contexte, b->enfants.front());

			if (res1.est_errone) {
				return res1;
			}

			auto res2 = evalue_expression(contexte, b->enfants.back());

			if (res2.est_errone) {
				return res2;
			}

			auto res = ResultatExpression();
			res.type = res1.type;

			if (est_operateur_bool(b->identifiant())) {
				if (res.type == type_expression::REEL) {
					res.condition = applique_operateur_binaire_comp(b->identifiant(), res1.reel, res2.reel);
				}
				else {
					res.condition = applique_operateur_binaire_comp(b->identifiant(), res1.entier, res2.entier);
				}
			}
			else {
				if (res.type == type_expression::REEL) {
					res.reel = applique_operateur_binaire(b->identifiant(), res1.reel, res2.reel);
				}
				else {
					res.entier = applique_operateur_binaire(b->identifiant(), res1.entier, res2.entier);
				}
			}

			return res;
		}
	}
}
