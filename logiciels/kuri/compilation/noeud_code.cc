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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "noeud_code.hh"

#include "arbre_syntaxique.hh"

NoeudCode *ConvertisseuseNoeudCode::converti_noeud_syntaxique(NoeudExpression *noeud_expression)
{
	auto noeud_code = static_cast<NoeudCode *>(nullptr);

	switch (noeud_expression->genre) {
		case GenreNoeud::DECLARATION_FONCTION:
		case GenreNoeud::DECLARATION_COROUTINE:
		case GenreNoeud::DECLARATION_OPERATEUR:
		{
			auto decl = static_cast<NoeudDeclarationFonction *>(noeud_expression);

			auto n = noeuds_fonctions.ajoute_element();
			n->params_entree.reserve(decl->params.taille);

			n->nom.pointeur = const_cast<char *>(decl->lexeme->chaine.pointeur());
			n->nom.taille = decl->lexeme->chaine.taille();

			n->bloc = static_cast<NoeudCodeBloc *>(converti_noeud_syntaxique(decl->bloc));

			POUR (decl->params) {
				auto n_param = converti_noeud_syntaxique(it);
				n->params_entree.pousse(static_cast<NoeudCodeDeclaration *>(n_param));
			}

			noeud_code = n;
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto n = noeuds_assignations.ajoute_element();
			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto noeud_bloc = static_cast<NoeudBloc *>(noeud_expression);

			auto expressions = noeud_bloc->expressions.verrou_lecture();

			auto n = noeuds_blocs.ajoute_element();
			n->expressions.reserve(expressions->taille);

			POUR (*expressions) {
				n->expressions.pousse(converti_noeud_syntaxique(it));
			}

			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		case GenreNoeud::EXPRESSION_INDEXAGE:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::OPERATEUR_BINAIRE:
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto noeud_op_bin = static_cast<NoeudExpressionBinaire *>(noeud_expression);

			auto n = noeuds_operations_binaire.ajoute_element();
			n->operande_gauche = converti_noeud_syntaxique(noeud_op_bin->expr1);
			n->operande_droite = converti_noeud_syntaxique(noeud_op_bin->expr2);

			noeud_code = n;
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		case GenreNoeud::EXPRESSION_DELOGE:
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			break;
		}
		case GenreNoeud::DIRECTIVE_EXECUTION:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		case GenreNoeud::EXPRESSION_INFO_DE:
		case GenreNoeud::EXPRESSION_MEMOIRE:
		case GenreNoeud::OPERATEUR_UNAIRE:
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		case GenreNoeud::INSTRUCTION_RETOUR:
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		case GenreNoeud::INSTRUCTION_RETIENS:
		case GenreNoeud::EXPANSION_VARIADIQUE:
		case GenreNoeud::EXPRESSION_TYPE_DE:
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud_expression);

			auto n = noeuds_operations_unaire.ajoute_element();
			n->operande = converti_noeud_syntaxique(expr->expr);

			noeud_code = n;
			break;
		}
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::DECLARATION_ENUM:
		case GenreNoeud::DECLARATION_STRUCTURE:
		case GenreNoeud::EXPRESSION_INIT_DE:
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		case GenreNoeud::INSTRUCTION_BOUCLE:
		case GenreNoeud::INSTRUCTION_REPETE:
		case GenreNoeud::INSTRUCTION_TANTQUE:
		case GenreNoeud::INSTRUCTION_POUR:
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		case GenreNoeud::INSTRUCTION_TENTE:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			break;
		}
	}

	// À FAIRE : supprime cela quand nous gérerons tous les cas
	if (noeud_code) {
		noeud_code->genre = static_cast<int>(noeud_expression->genre);
	}

	noeud_expression->noeud_code = noeud_code;

	return noeud_code;
}
