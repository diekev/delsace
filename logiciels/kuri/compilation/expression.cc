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

#include "biblinternes/outils/conditions.h"

#include "compilatrice.hh"
#include "outils_lexemes.hh"
#include "portee.hh"

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
		EspaceDeTravail *espace,
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

			auto fichier = espace->fichier(b->lexeme->fichier);
			auto decl = trouve_dans_bloc_ou_module(*espace, bloc, b->ident, fichier);

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
				if (decl_var->type->est_enum()) {
					auto type_enum = decl_var->type->comme_enum();

					POUR (type_enum->membres) {
						if (it.nom == decl_var->ident->nom) {
							res.entier = it.valeur;
							res.type = TypeExpression::ENTIER;
							return res;
						}
					}
				}

				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "La déclaration de la variable n'a pas d'expression !";

				return res;
			}

			// À FAIRE : stockage de la valeur
			return evalue_expression(espace, decl->bloc_parent, decl_var->expression);
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto expr_taille_de = static_cast<NoeudExpressionUnaire *>(b);
			auto type = expr_taille_de->expr->type;

			auto res = ResultatExpression();
			res.type = TypeExpression::ENTIER;
			res.entier = type->taille_octet;

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			auto res = ResultatExpression();
			res.type = TypeExpression::ENTIER;
			res.entier = b->lexeme->chaine == "vrai";

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			auto res = ResultatExpression();
			res.type = TypeExpression::ENTIER;
			res.entier = static_cast<long>(b->lexeme->valeur_entiere);

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			auto res = ResultatExpression();
			res.type = TypeExpression::ENTIER;
			res.entier = static_cast<long>(b->lexeme->valeur_entiere);

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			auto res = ResultatExpression();
			res.type = TypeExpression::REEL;
			res.reel = b->lexeme->valeur_reelle;

			return res;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			auto inst = static_cast<NoeudSi *>(b);

			auto res = evalue_expression(espace, bloc, inst->condition);

			if (res.est_errone) {
				return res;
			}

			if (res.type != TypeExpression::ENTIER) {
				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "L'expression n'est pas de type booléen !";
				return res;
			}

			if (res.condition == (b->genre == GenreNoeud::INSTRUCTION_SI)) {
				res = evalue_expression(espace, bloc, inst->bloc_si_vrai);
			}
			else {
				if (inst->bloc_si_faux) {
					res = evalue_expression(espace, bloc, inst->bloc_si_faux);
				}
			}

			return res;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(b);
			auto res = evalue_expression(espace, bloc, inst->expr);

			if (res.est_errone) {
				return res;
			}

			if (res.type == TypeExpression::REEL) {
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
			auto res1 = evalue_expression(espace, bloc, inst->expr1);

			if (res1.est_errone) {
				return res1;
			}

			auto res2 = evalue_expression(espace, bloc, inst->expr2);

			if (res2.est_errone) {
				return res2;
			}

			auto res = ResultatExpression();
			res.type = res1.type;

			if (est_operateur_bool(inst->lexeme->genre)) {
				if (res.type == TypeExpression::REEL) {
					res.condition = applique_operateur_binaire_comp(inst->lexeme->genre, res1.reel, res2.reel);
				}
				else {
					res.condition = applique_operateur_binaire_comp(inst->lexeme->genre, res1.entier, res2.entier);
				}
			}
			else {
				if (res.type == TypeExpression::REEL) {
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
			return evalue_expression(espace, bloc, inst->expr);
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			/* À FAIRE : transtypage de l'expression constante */
			auto inst = static_cast<NoeudExpressionBinaire *>(b);
			return evalue_expression(espace, bloc, inst->expr1);
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto ref_membre = b->comme_ref_membre();
			auto type_accede = ref_membre->accede->type;

			if (type_accede->genre == GenreType::ENUM || type_accede->genre == GenreType::ERREUR) {
				auto type_enum = type_accede->comme_enum();
				auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;
				auto res = ResultatExpression();
				res.est_errone = false;
				res.entier = valeur_enum;
				res.type = TypeExpression::ENTIER;
				return res;
			}

			auto res = ResultatExpression();
			res.est_errone = true;
			res.noeud_erreur = b;
			res.message_erreur = "L'expression n'est pas constante et ne peut être calculée !";

			return res;
		}
	}
}
