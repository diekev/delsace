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

#include "contexte_generation_code.h"
#include "outils_lexemes.hh"
#include "portee.hh"

using denombreuse = lng::decoupeuse_nombre<GenreLexeme>;

enum class dir_associativite : int {
	GAUCHE,
	DROITE,
};

struct DonneesPrecedence {
	dir_associativite direction;
	int priorite;
};

static DonneesPrecedence associativite(GenreLexeme identifiant)
{
	switch (identifiant) {
		case GenreLexeme::VIRGULE:
			return { dir_associativite::GAUCHE, 0 };
		case GenreLexeme::TROIS_POINTS:
			return { dir_associativite::GAUCHE, 1 };
		case GenreLexeme::EGAL:
		case GenreLexeme::DECLARATION_VARIABLE:
		case GenreLexeme::DECLARATION_CONSTANTE:
		case GenreLexeme::PLUS_EGAL:
		case GenreLexeme::MOINS_EGAL:
		case GenreLexeme::DIVISE_EGAL:
		case GenreLexeme::MULTIPLIE_EGAL:
		case GenreLexeme::MODULO_EGAL:
		case GenreLexeme::ET_EGAL:
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::OUX_EGAL:
		case GenreLexeme::DEC_DROITE_EGAL:
		case GenreLexeme::DEC_GAUCHE_EGAL:
			return { dir_associativite::GAUCHE, 2 };
		case GenreLexeme::BARRE_BARRE:
			return { dir_associativite::GAUCHE, 3 };
		case GenreLexeme::ESP_ESP:
			return { dir_associativite::GAUCHE, 4 };
		case GenreLexeme::BARRE:
			return { dir_associativite::GAUCHE, 5 };
		case GenreLexeme::CHAPEAU:
			return { dir_associativite::GAUCHE, 6 };
		case GenreLexeme::ESPERLUETTE:
			return { dir_associativite::GAUCHE, 7 };
		case GenreLexeme::DIFFERENCE:
		case GenreLexeme::EGALITE:
			return { dir_associativite::GAUCHE, 8 };
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
			return { dir_associativite::GAUCHE, 9 };
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DECALAGE_DROITE:
			return { dir_associativite::GAUCHE, 10 };
		case GenreLexeme::PLUS:
		case GenreLexeme::MOINS:
			return { dir_associativite::GAUCHE, 11 };
		case GenreLexeme::FOIS:
		case GenreLexeme::DIVISE:
		case GenreLexeme::POURCENT:
			return { dir_associativite::GAUCHE, 12 };
		case GenreLexeme::EXCLAMATION:
		case GenreLexeme::TILDE:
		case GenreLexeme::AROBASE:
		case GenreLexeme::PLUS_UNAIRE:
		case GenreLexeme::MOINS_UNAIRE:
			return { dir_associativite::DROITE, 13 };
		case GenreLexeme::POINT:
		case GenreLexeme::CROCHET_OUVRANT:
			return { dir_associativite::GAUCHE, 14 };
		default:
			assert(false);
			return { static_cast<dir_associativite>(-1), -1 };
	}
}

bool precedence_faible(GenreLexeme identifiant1, GenreLexeme identifiant2)
{
	auto p1 = associativite(identifiant1);
	auto p2 = associativite(identifiant2);

	return (p1.direction == dir_associativite::GAUCHE && p1.priorite <= p2.priorite)
			|| ((p2.direction == dir_associativite::DROITE) && (p1.priorite < p2.priorite));
}

/* ************************************************************************** */

template <typename T>
static auto applique_operateur_unaire(GenreLexeme id, T &a)
{
	switch (id) {
		case GenreLexeme::EXCLAMATION:
		{
			a = !a;
			break;
		}
		case GenreLexeme::TILDE:
		{
			a = ~a;
			break;
		}
		case GenreLexeme::PLUS_UNAIRE:
		{
			break;
		}
		case GenreLexeme::MOINS_UNAIRE:
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

static auto applique_operateur_unaire(GenreLexeme id, double &a)
{
	switch (id) {
		case GenreLexeme::PLUS_UNAIRE:
		{
			break;
		}
		case GenreLexeme::MOINS_UNAIRE:
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
static auto applique_operateur_binaire(GenreLexeme id, T a, T b)
{
	switch (id) {
		case GenreLexeme::PLUS:
		case GenreLexeme::PLUS_EGAL:
		{
			return a + b;
		}
		case GenreLexeme::MOINS:
		case GenreLexeme::MOINS_EGAL:
		{
			return a - b;
		}
		case GenreLexeme::FOIS:
		case GenreLexeme::MULTIPLIE_EGAL:
		{
			return a * b;
		}
		case GenreLexeme::DIVISE:
		case GenreLexeme::DIVISE_EGAL:
		{
			return a / b;
		}
		case GenreLexeme::POURCENT:
		case GenreLexeme::MODULO_EGAL:
		{
			return a % b;
		}
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::ET_EGAL:
		{
			return a & b;
		}
		case GenreLexeme::OU_EGAL:
		case GenreLexeme::BARRE:
		{
			return a | b;
		}
		case GenreLexeme::CHAPEAU:
		case GenreLexeme::OUX_EGAL:
		{
			return a ^ b;
		}
		case GenreLexeme::DECALAGE_DROITE:
		case GenreLexeme::DEC_DROITE_EGAL:
		{
			return a >> b;
		}
		case GenreLexeme::DECALAGE_GAUCHE:
		case GenreLexeme::DEC_GAUCHE_EGAL:
		{
			return a << b;
		}
		default:
		{
			return T(0);
		}
	}
}

static auto applique_operateur_binaire(GenreLexeme id, double a, double b)
{
	switch (id) {
		case GenreLexeme::PLUS:
		case GenreLexeme::PLUS_EGAL:
		{
			return a + b;
		}
		case GenreLexeme::MOINS:
		case GenreLexeme::MOINS_EGAL:
		{
			return a - b;
		}
		case GenreLexeme::FOIS:
		case GenreLexeme::MULTIPLIE_EGAL:
		{
			return a * b;
		}
		case GenreLexeme::DIVISE:
		case GenreLexeme::DIVISE_EGAL:
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
static auto applique_operateur_binaire_comp(GenreLexeme id, T a, T b)
{
	switch (id) {
		case GenreLexeme::INFERIEUR:
		case GenreLexeme::INFERIEUR_EGAL:
		{
			return a < b;
		}
		case GenreLexeme::SUPERIEUR:
		case GenreLexeme::SUPERIEUR_EGAL:
		{
			return a > b;
		}
		case GenreLexeme::DIFFERENCE:
		{
			return a != b;
		}
		case GenreLexeme::ESP_ESP:
		{
			return a && b;
		}
		case GenreLexeme::EGALITE:
		{
			return a == b;
		}
		case GenreLexeme::BARRE_BARRE:
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
ResultatExpression evalue_expression(
		ContexteGenerationCode &contexte,
		NoeudBloc *bloc,
		NoeudExpression *b)
{
	switch (b->genre) {
		default:
		{
			auto res = ResultatExpression();
			res.est_errone = true;
			res.noeud_erreur = b;
			res.message_erreur = "L'expression n'est pas constante et ne peut être calculée !";

			return res;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto res = ResultatExpression();

			auto decl = trouve_dans_bloc(bloc, b->ident);

			if (decl == nullptr) {
				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "La variable n'existe pas !";

				return res;
			}

			if (decl->genre != GenreNoeud::DECLARATION_VARIABLE) {
				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "La référence n'est pas celle d'une variable !";

				return res;
			}

			auto decl_var = static_cast<NoeudDeclarationVariable *>(decl);

			if (decl_var->expression == nullptr) {
				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "La déclaration de la variable n'a pas d'expression !";

				return res;
			}

			// À FAIRE : stockage de la valeur
			return evalue_expression(contexte, decl->bloc_parent, decl_var->expression);
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto type = std::any_cast<Type *>(b->valeur_calculee);

			auto res = ResultatExpression();
			res.type = type_expression::ENTIER;
			res.entier = type->taille_octet;

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			auto res = ResultatExpression();
			res.type = type_expression::ENTIER;
			res.entier = b->lexeme->chaine == "vrai";

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			auto res = ResultatExpression();
			res.type = type_expression::ENTIER;
			res.entier = lng::decoupeuse_nombre<GenreLexeme>::converti_chaine_nombre_entier(b->lexeme->chaine, b->lexeme->genre);

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			auto res = ResultatExpression();
			res.type = type_expression::REEL;
			res.reel = lng::converti_nombre_reel(dls::vue_chaine(b->lexeme->chaine.pointeur(), b->lexeme->chaine.taille()));

			return res;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto inst = static_cast<NoeudSi *>(b);

			auto res = evalue_expression(contexte, bloc, inst->condition);

			if (res.est_errone) {
				return res;
			}

			if (res.type != type_expression::ENTIER) {
				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "L'expression n'est pas de type booléen !";
				return res;
			}

			if (res.condition == (b->genre == GenreNoeud::INSTRUCTION_SI)) {
				res = evalue_expression(contexte, bloc, inst->bloc_si_vrai);
			}
			else {
				if (inst->bloc_si_faux) {
					res = evalue_expression(contexte, bloc, inst->bloc_si_faux);
				}
			}

			return res;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto res = evalue_expression(contexte, bloc, inst->expr);

			if (res.est_errone) {
				return res;
			}

			if (res.type == type_expression::REEL) {
				applique_operateur_unaire(inst->lexeme->genre, res.reel);
			}
			else {
				applique_operateur_unaire(inst->lexeme->genre, res.entier);
			}

			return res;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			auto res1 = evalue_expression(contexte, bloc, inst->expr1);

			if (res1.est_errone) {
				return res1;
			}

			auto res2 = evalue_expression(contexte, bloc, inst->expr2);

			if (res2.est_errone) {
				return res2;
			}

			auto res = ResultatExpression();
			res.type = res1.type;

			if (est_operateur_bool(inst->lexeme->genre)) {
				if (res.type == type_expression::REEL) {
					res.condition = applique_operateur_binaire_comp(inst->lexeme->genre, res1.reel, res2.reel);
				}
				else {
					res.condition = applique_operateur_binaire_comp(inst->lexeme->genre, res1.entier, res2.entier);
				}
			}
			else {
				if (res.type == type_expression::REEL) {
					res.reel = applique_operateur_binaire(inst->lexeme->genre, res1.reel, res2.reel);
				}
				else {
					res.entier = applique_operateur_binaire(inst->lexeme->genre, res1.entier, res2.entier);
				}
			}

			return res;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			return evalue_expression(contexte, bloc, inst->expr);
		}
		case GenreNoeud::EXPRESSION_TRANSTYPE:
		{
			/* À FAIRE : transtypage de l'expression constante */
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			return evalue_expression(contexte, bloc, inst->expr);
		}
	}
}
