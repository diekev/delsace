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

#include "expression.hh"

#include "biblinternes/outils/conditions.h"

#include "parsage/identifiant.hh"
#include "parsage/outils_lexemes.hh"

#include "compilation/espace_de_travail.hh"
#include "compilation/portee.hh"

#include "noeud_expression.hh"

/* À FAIRE: les expressions littérales des énumérations ne sont pas validées donc les valeurs sont toujours surl les lexèmes */

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
			auto decl = trouve_dans_bloc_ou_module(bloc, b->ident, fichier);

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

			if (decl_var->valeur_expression.type != TypeExpression::INVALIDE) {
				res.est_errone = false;
				res.valeur = decl_var->valeur_expression;
				return res;
			}

			if (decl_var->expression == nullptr) {
				if (decl_var->type->est_enum()) {
					auto type_enum = decl_var->type->comme_enum();

					POUR (type_enum->membres) {
						if (it.nom == decl_var->ident) {
							res.valeur.entier = it.valeur;
							res.valeur.type = TypeExpression::ENTIER;
							res.est_errone = false;
							return res;
						}
					}
				}

				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "La déclaration de la variable n'a pas d'expression !";

				return res;
			}

			return evalue_expression(espace, decl->bloc_parent, decl_var->expression);
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto expr_taille_de = b->comme_taille_de();
			auto type = expr_taille_de->expression->type;

			auto res = ResultatExpression();
			res.valeur.type = TypeExpression::ENTIER;
			res.valeur.entier = type->taille_octet;
			res.est_errone = false;

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			auto res = ResultatExpression();
			res.valeur.type = TypeExpression::ENTIER;
			res.valeur.entier = b->lexeme->chaine == "vrai";
			res.est_errone = false;

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			auto res = ResultatExpression();
			res.valeur.type = TypeExpression::ENTIER;
			res.valeur.entier = static_cast<long>(b->lexeme->valeur_entiere);
			res.est_errone = false;

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			auto res = ResultatExpression();
			res.valeur.type = TypeExpression::ENTIER;
			res.valeur.entier = static_cast<long>(b->lexeme->valeur_entiere);
			res.est_errone = false;

			return res;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			auto res = ResultatExpression();
			res.valeur.type = TypeExpression::REEL;
			res.valeur.reel = b->lexeme->valeur_reelle;
			res.est_errone = false;

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

			if (res.valeur.type != TypeExpression::ENTIER) {
				res.est_errone = true;
				res.noeud_erreur = b;
				res.message_erreur = "L'expression n'est pas de type booléen !";
				return res;
			}

			if (res.valeur.condition == (b->genre == GenreNoeud::INSTRUCTION_SI)) {
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
			auto inst = b->comme_expression_unaire();
			auto res = evalue_expression(espace, bloc, inst->operande);

			if (res.est_errone) {
				return res;
			}

			if (res.valeur.type == TypeExpression::REEL) {
				applique_operateur_unaire(inst->lexeme->genre, res.valeur.reel);
			}
			else {
				applique_operateur_unaire(inst->lexeme->genre, res.valeur.entier);
			}

			return res;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto inst = b->comme_expression_binaire();
			auto res1 = evalue_expression(espace, bloc, inst->operande_gauche);

			if (res1.est_errone) {
				return res1;
			}

			auto res2 = evalue_expression(espace, bloc, inst->operande_droite);

			if (res2.est_errone) {
				return res2;
			}

			auto res = ResultatExpression();
			res.valeur.type = res1.valeur.type;
			res.est_errone = false;

			if (est_operateur_bool(inst->lexeme->genre)) {
				if (res.valeur.type == TypeExpression::REEL) {
					res.valeur.condition = applique_operateur_binaire_comp(inst->lexeme->genre, res1.valeur.reel, res2.valeur.reel);
				}
				else {
					res.valeur.condition = applique_operateur_binaire_comp(inst->lexeme->genre, res1.valeur.entier, res2.valeur.entier);
				}
			}
			else {
				if (res.valeur.type == TypeExpression::REEL) {
					res.valeur.reel = applique_operateur_binaire(inst->lexeme->genre, res1.valeur.reel, res2.valeur.reel);
				}
				else {
					res.valeur.entier = applique_operateur_binaire(inst->lexeme->genre, res1.valeur.entier, res2.valeur.entier);
				}
			}

			return res;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto inst = b->comme_parenthese();
			return evalue_expression(espace, bloc, inst->expression);
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			/* À FAIRE : transtypage de l'expression constante */
			auto inst = b->comme_comme();
			return evalue_expression(espace, bloc, inst->expression);
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto ref_membre = b->comme_reference_membre();
			auto type_accede = ref_membre->accedee->type;

			if (type_accede->genre == GenreType::ENUM || type_accede->genre == GenreType::ERREUR) {
				auto type_enum = type_accede->comme_enum();
				auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;
				auto res = ResultatExpression();
				res.est_errone = false;
				res.valeur.entier = valeur_enum;
				res.valeur.type = TypeExpression::ENTIER;
				return res;
			}

			if (type_accede->est_tableau_fixe()) {
				if (!ref_membre->membre->est_reference_declaration()) {
					auto res = ResultatExpression();
					res.est_errone = true;
					res.noeud_erreur = b;
					res.message_erreur = "L'expression n'est pas constante et ne peut être calculée !";

					return res;
				}

				auto ref_decl_membre = ref_membre->membre->comme_reference_declaration();

				if (ref_decl_membre->ident->nom == "taille") {
					auto res = ResultatExpression();
					res.est_errone = false;
					res.valeur.entier = type_accede->comme_tableau_fixe()->taille;
					res.valeur.type = TypeExpression::ENTIER;
					return res;
				}
			}

			auto res = ResultatExpression();
			res.est_errone = true;
			res.noeud_erreur = b;
			res.message_erreur = "L'expression n'est pas constante et ne peut être calculée !";

			return res;
		}
	}
}
